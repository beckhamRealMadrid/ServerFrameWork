#include "StdAfx.h"
#include "TwoWaySession.h"
#include "IOCPManager.h"

START_NAMESPACE

CTwoWaySession::CTwoWaySession( CSessionManager* pSM ) : CSession( pSM )
{
	m_pOtherSession = NULL;
}

bool CTwoWaySession::Connect( LPCTSTR strAddr, int nPort )
{
	if ( CSession::Connect( strAddr, nPort ) )
	{
		bool bAddIOPort = IOCP->AddIOPort( (HANDLE)GetSocket(), (DWORD)this );
		int nReceiveResult = PrepareReceive();

		if ( bAddIOPort && ( nReceiveResult == ERROR_NONE ) )
		{
//			Log( _T("CTwoWaySession::Connect Success Port : %d"), nPort );
			PostCreate();

			return true;
		}
	}

	return false;
}

int CTwoWaySession::DispatchRecv( DWORD dwBytes, LPOVERLAPPED_BASE lpov )
{
	if ( dwBytes == 0 )
		return ERROR_TRANS;
	
	int nError;
	int nIndex = 0;

//	Receive Buffer�� ũ�Ⱑ Send Buffer�� ũ�� ���� ũ�� ������
//	���� ����Ÿ�� Send Bufferũ�⺸�� Ŭ��� ©�� �����ش�.
//	while ( dwBytes != 0 )
//	{
		CNetMsg* pNetMsg = CNetMsg::s_NetMsgManager.Alloc();

		Synchronized so( &m_soRecv );

		m_pCQue->IncTailPos( dwBytes );
		m_pCQue->GetDataAt( 0, pNetMsg->m_pBuf, dwBytes );
		pNetMsg->m_nBufCount = dwBytes;
		m_pCQue->IncHeadPos( dwBytes );
		
/*
		if ( dwBytes > NETWORK_SENDBUFFER_SIZE )
		{
			memcpy( pNetMsg->m_pBuf, m_pCQue->GetBuf() + nIndex, NETWORK_SENDBUFFER_SIZE );
			pNetMsg->m_nBufCount = NETWORK_SENDBUFFER_SIZE;
			nIndex += NETWORK_SENDBUFFER_SIZE;
			dwBytes -= NETWORK_SENDBUFFER_SIZE;
		}
		else
		{
			memcpy( pNetMsg->m_pBuf, m_pCQue->GetBuf() + nIndex, dwBytes );
			pNetMsg->m_nBufCount = dwBytes;
			nIndex += dwBytes;
			dwBytes = 0;
		}
*/

		nError = m_pOtherSession->Send( pNetMsg );

		if ( nError != ERROR_NONE )
            return nError;		
//	}

	return PrepareReceive();
}

END_NAMESPACE