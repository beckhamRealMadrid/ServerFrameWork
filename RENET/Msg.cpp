#include "stdafx.h"
#include "Msg.h"

START_NAMESPACE

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CCircularQueue
//
CCircularQueue::CCircularQueue( int nSize )
{
	m_pBuf = new BYTE[nSize];
	m_nBufSize = nSize;
	Clear();
}

CCircularQueue::~CCircularQueue()
{
	delete [] m_pBuf;
}

void CCircularQueue::Clear()
{
	m_nHead = 0;
	m_nTail = 0;

	m_nDataSize = 0;
}

LPBYTE CCircularQueue::GetBuf()
{
	return m_pBuf;
}

bool CCircularQueue::IsEmpty()
{ 
	return (m_nDataSize == 0);
}

bool CCircularQueue::IsFull()
{ 
	return (m_nDataSize == m_nBufSize);
}

int CCircularQueue::GetDataSize()
{ 
	return m_nDataSize;
}

int CCircularQueue::GetEmptySize()
{ 
	return m_nBufSize - m_nDataSize;
}

void CCircularQueue::IncHeadPos( int nAmount )
{
	assert( GetDataSize() >= nAmount );

	m_nHead = (m_nHead + nAmount) % m_nBufSize;
	m_nDataSize -= nAmount;
}

void CCircularQueue::IncTailPos( int nAmount )
{
	assert( GetEmptySize() >= nAmount );

	m_nTail = (m_nTail + nAmount) % m_nBufSize;
	m_nDataSize += nAmount;
}

LPWSABUF CCircularQueue::MakeWSABuf( int* pnBufCount )
{
	assert( !IsFull() );

	m_wsaBuf[0].buf = (char *)&m_pBuf[m_nTail];

	if (m_nHead > m_nTail || m_nTail == 0)
	{
		m_wsaBuf[0].len = GetEmptySize();

		*pnBufCount = 1;
	}
	else
	{
		m_wsaBuf[0].len = m_nBufSize - m_nTail;

		m_wsaBuf[1].buf = (char *)m_pBuf;
		m_wsaBuf[1].len = m_nHead;

		*pnBufCount = 2;
	}

	return &m_wsaBuf[0];
}

void CCircularQueue::Decrypt()
{
	WORD wCode[2] = {0, 0};
	WORD wMsgSize;

	GetDataAt( 0, wCode, 4 );
	wMsgSize = wCode[0];

	int nPos = (m_nHead + 4) % m_nBufSize;
	int nIndex = 0;

	while (wMsgSize--)
	{
		m_pBuf[nPos] = m_pBuf[nPos] ^ wCode[nIndex];
		nIndex ^= 1;
		nPos = (nPos + 1) % m_nBufSize;
	}
}

int CCircularQueue::GetStringLength( int nIndex )
{
	int nLength = 0;

	for (int i = (m_nHead + nIndex) % m_nBufSize; i != m_nTail; nLength++ )
	{
		if (m_pBuf[i] == 0)
			break;

		i = (i + 1) % m_nBufSize;
	}

	return nLength;
}

bool CCircularQueue::GetDataAt(int nIndex, LPVOID pData, int nSize)
{
	if ( GetDataSize() >= nSize )
	{
		int nSrcHead = (m_nHead + nIndex) % m_nBufSize;
		int nSrcTail = (m_nHead + nIndex + nSize) % m_nBufSize;

		if ( nSrcHead > nSrcTail )
		{
			int nSizeForward = m_nBufSize - nSrcHead;
			LPBYTE pBufDest = (LPBYTE)pData;

			CopyMemory( pBufDest, &m_pBuf[nSrcHead], nSizeForward );
			CopyMemory( pBufDest + nSizeForward, m_pBuf, nSize - nSizeForward );
		}
		else
		{
			CopyMemory( pData, &m_pBuf[nSrcHead], nSize );
		}

		return true;
	}
	else
	{
//		assert( false );
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CMsgRecv
//
CMsgRecv::CMsgRecv( CCircularQueue* pQue ) : m_pCQue( pQue )
{
	Clear();
}

void CMsgRecv::Clear()
{
	m_nReadPos = 4;
}

bool CMsgRecv::Decrypt()
{
	WORD wSize = GetSize();
	WORD wCheck;

	m_pCQue->GetDataAt( wSize - 2, &wCheck, 2 );

	if ( wSize != wCheck )
	{
		// TODO: Server 실행 몇 시간후, wSize = 8, wCheck = 2 로 assert( false ) 진입
		//		 진입 당시 접근 IP는 211.47.193.130
		//		 이에 따라 주석 처리
//		assert( false );
		return false;
	}

	m_pCQue->Decrypt();

	return true;
}

WORD CMsgRecv::ID()
{
	WORD wID = 0;
	m_pCQue->GetDataAt(2, &wID, 2);

	return wID;
}

WORD CMsgRecv::GetSize()
{
	WORD wSize = 0;
	m_pCQue->GetDataAt(0, &wSize, 2);

	return wSize + 6; 
}

void CMsgRecv::ReadData( LPVOID pData, int n )
{
	m_pCQue->GetDataAt( m_nReadPos, pData, n );
	m_nReadPos += n;
}

void CMsgRecv::ReadString( LPSTR data, int nBufferSize )
{
	assert( data );

	int nLen = m_pCQue->GetStringLength( m_nReadPos ) + 1;
	if (nLen < nBufferSize)
	{
		ReadData( data, nLen );
	}
	else
	{
		ReadData( data, nBufferSize - 1 );
		data[nBufferSize - 1] = 0;
	}
}

CMsgRecv& CMsgRecv::operator >> (CMSGStreamObj& obj)
{
	obj.ReadObject(*this);
	return *this;
}

CMsgRecv& CMsgRecv::operator >> (CMSGStreamObj* pObj)
{
	pObj->ReadObject(*this);
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CMsgSend
//
CMsgSend::CMsgSend() : m_pNetMsg( NULL )
{

}

CMsgSend::CMsgSend( CMsgSend& msg )
{
	m_pNetMsg		= msg.m_pNetMsg;
	m_pNetMsg->AddRef();

	m_pSize			= msg.m_pSize;
	m_pID			= msg.m_pID;
	m_pWrite		= msg.m_pWrite;
	m_bEncrypted	= msg.m_bEncrypted;
}

CMsgSend::CMsgSend( CNetMsg* pNetMsg ) 
{
	assert(pNetMsg);
	Attach(pNetMsg);
}

CMsgSend::~CMsgSend()
{
	Detach();
}

bool CMsgSend::Attach( CNetMsg* pNetMsg )
{
	assert( pNetMsg );

	Detach();

	pNetMsg->AddRef();
	m_pNetMsg = pNetMsg;
	m_pBuf = m_pNetMsg->m_pBuf;

	Clear();

	return true;
}

CMsgSend& CMsgSend::ID(WORD wID)
{
	AllocBuf();

	*m_pID = wID;
	*m_pSize = 0;

	return *this;
}

void CMsgSend::AllocBuf()
{
	Detach();

	m_pNetMsg = CNetMsg::s_NetMsgManager.Alloc();
	m_pNetMsg->AddRef();
	m_pBuf = m_pNetMsg->m_pBuf;

	Clear();
}

void CMsgSend::Detach()
{
	if ( m_pNetMsg )
	{
		m_pNetMsg->Release();
		m_pNetMsg = NULL;
	}
}

void CMsgSend::Clear()
{
	m_pSize		= (LPWORD)&m_pBuf[0];
	m_pID		= (LPWORD)&m_pBuf[2];
	m_pWrite	= &m_pBuf[4];

	m_bEncrypted = false;
}

LPBYTE CMsgSend::GetBuffer()
{
	return m_pBuf;
}

WORD CMsgSend::GetSize()
{
	return *m_pSize + 6;
}

bool CMsgSend::Encrypt()
{
	if ( m_bEncrypted )
	{
		Log( _T("CMg::Encrypt called for the Encrypted MSG ID = %d"), *m_pID);
		assert(false);
		return false;
	}
	else
	{
		LPWORD pCheck = (LPWORD)( m_pBuf + *m_pSize + 4);
		*pCheck = GetSize();

		WORD nBufferSize = *m_pSize;
		LPBYTE pData = m_pBuf + 4;
		int nIndex = 0;

		for ( DWORD i = 0; i < nBufferSize; i++ )
		{
			pData[i] = pData[i] ^ ( *( m_pSize + nIndex ) );
			nIndex = (nIndex + 1) & 1;
		}

		m_pNetMsg->m_nBufCount = GetSize();
		m_bEncrypted = true;

		return true;
	}
}

void CMsgSend::WriteData( void* pData, int n )
{
	assert(m_pWrite + n < m_pBuf + NETWORK_SENDBUFFER_SIZE);

	if (m_pWrite + n > m_pBuf + NETWORK_SENDBUFFER_SIZE)
	{
		Log( _T("!!! BUFFERSIZE to small for MSG ID = %d"), *m_pID);
		assert( false );
		return;
	}

	memcpy(m_pWrite, pData, n);
	m_pWrite += n;
	*m_pSize += n;
}

CMsgSend& CMsgSend::operator << (LPCSTR data)
{
	if ( data == NULL )
	{
		*m_pWrite++ = 0;
		*m_pSize++;
	}
	else
	{
		WriteData( (void *)data, static_cast <int>(strlen(data) + 1) );
	}

	return *this;
}

CMsgSend& CMsgSend::operator << (LPCWSTR data)
{
	if ( data == NULL )
	{
		*m_pWrite++ = 0;
		*m_pSize++;
	}
	else
	{
		WriteData( (void *)data, static_cast <int>(wcslen(data) + 1) * sizeof(WCHAR));
	}

	return *this;
}

CMsgSend& CMsgSend::operator << (CMSGStreamObj& obj)
{
	obj.WriteObject(*this);
	return *this;
}

CMsgSend& CMsgSend::operator << (CMSGStreamObj* pObj)
{
	pObj->WriteObject(*this);
	return *this;
}

END_NAMESPACE