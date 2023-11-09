#include "stdafx.h"
#include "NetMsg.h"

START_NAMESPACE

CMemoryBlockManager <CNetMsg> CNetMsg::s_NetMsgManager;

CNetMsg::CNetMsg()
{
	m_nRefCount = 0;
};

bool CNetMsg::Create()
{
	m_nBufCount = 0;
	assert( m_nRefCount == 0 );

	return true;
}

void CNetMsg::AddRef()
{
	InterlockedIncrement(&m_nRefCount);
};

int CNetMsg::GetRefCount()
{
	return m_nRefCount;
};

void CNetMsg::Release()
{
	if ( InterlockedDecrement( &m_nRefCount ) == 0 )
	{
		s_NetMsgManager.Release( this );
	}
}

void OVERLAPPED_SEND::SendClear()
{
	memset( &m_ov, 0, sizeof( OVERLAPPED ) );
	m_dwFlag	= IOCP_SEND;
	m_dwOffset	= 0;
	m_listMsgs.clear();
}

bool OVERLAPPED_SEND::AddNetMsg( CNetMsg* pNetMsg )
{
	if ( m_listMsgs.size() >= 64 )
		return false;

	m_wsaBuf[ m_listMsgs.size() ].buf = (char*)pNetMsg->m_pBuf;
	m_wsaBuf[ m_listMsgs.size() ].len = pNetMsg->m_nBufCount;	

	m_listMsgs.push_back( pNetMsg );

	return true;
}

void OVERLAPPED_SEND::ProcessSendedBytes( DWORD dwBytes )
{
	CNetMsg* pNetMsg;
	netmsglistitor itor;

	for ( itor = m_listMsgs.begin(); (itor != m_listMsgs.end()) && (dwBytes > 0); )
	{
		pNetMsg = *itor;

		if ( pNetMsg->m_nBufCount - m_dwOffset > dwBytes )
		{ 
			// 요 버퍼에서 뒤쪽 데이터는 못보냈다는 얘기
			m_dwOffset += dwBytes;
			dwBytes = 0;
			Log( _T("Unsent Data Remain") );
		}
		else
		{
			dwBytes -= pNetMsg->m_nBufCount - m_dwOffset;
			itor = m_listMsgs.erase( itor );
			pNetMsg->Release();
			m_dwOffset = 0;
		}
	}

	// 버퍼 재정리
	int nBufIndex = 0;

	itor = m_listMsgs.begin();
	if ( itor != m_listMsgs.end() && m_dwOffset != 0 )
	{
		pNetMsg = *itor;
		m_wsaBuf[nBufIndex].buf = (char*)(pNetMsg->m_pBuf + m_dwOffset);
		m_wsaBuf[nBufIndex].len = pNetMsg->m_nBufCount - m_dwOffset;
		itor++;
		nBufIndex++;
	}

	for ( ; ( itor != m_listMsgs.end() ); ++itor )
	{
		pNetMsg = *itor;
		m_wsaBuf[nBufIndex].buf = (char*)pNetMsg->m_pBuf;
		m_wsaBuf[nBufIndex].len = pNetMsg->m_nBufCount;
		nBufIndex++;
	}
}

int OVERLAPPED_SEND::GetMsgCount()
{
	return (int)m_listMsgs.size();
}

int OVERLAPPED_SEND::GetMsgSize()
{
	CNetMsg* pNetMsg;
	netmsglistitor itor;
	int nCount = 0;

	for ( itor = m_listMsgs.begin(); itor != m_listMsgs.end(); ++itor )
	{
		pNetMsg = *itor;
		nCount += pNetMsg->m_nBufCount;
	}

	nCount -= m_dwOffset;

	return nCount;
}

END_NAMESPACE