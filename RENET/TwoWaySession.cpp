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

//	Receive Buffer의 크기가 Send Buffer의 크기 보다 크기 때문에
//	받은 데이타가 Send Buffer크기보다 클경우 짤라 보내준다.
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