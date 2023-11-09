#include "stdafx.h"
#include "SessionManager.h"
#include "IOCPManager.h"
#include "Heartbeat.h"

START_NAMESPACE

CSessionManager::CSessionManager()
{
	m_listActive.clear();
	m_listInActive.clear();
	m_pListener = NULL;
	m_pHeartbeat = NULL;
}

CSessionManager::~CSessionManager()
{
	if ( m_pListener )
	{
		m_pListener->Close( 0, 0 );
		delete m_pListener;
	}

	SAFE_DELETE( m_pHeartbeat );

	m_listActive.clear();
	m_listInActive.clear();
}

bool CSessionManager::Create( LPCTSTR szAddr, int nPort )
{
	m_pListener = new CListeningSession( this );

	bool bListenerCreate = m_pListener->Create( szAddr, nPort );
	bool bAddIOPort = IOCP->AddIOPort( (HANDLE)m_pListener->GetSocket(), (DWORD)m_pListener );	
	int nAcceptResult = m_pListener->PrepareAccept();

	if ( bListenerCreate && bAddIOPort && ( nAcceptResult == ERROR_NONE ) )
	{
		m_pHeartbeat = new CHeartbeat( this, CHECK_HEARTBEAT );
		if ( m_pHeartbeat->StartHeartbeat() )
			return true;
		else
			return false;
	}
	else
	{
		return false;
	}
}

int CSessionManager::AcceptSocket( SOCKET hSocket, LPCTSTR strIPAddr )
{
	CSession* pSession = NULL;
	
	Synchronized sync( &m_soList );

	if ( m_listInActive.size() > 0 )
	{
		pSession = m_listInActive.front();
		m_listInActive.pop_front();
	}
	else
	{
		Log( _T("!!!! Socket Pool Underfloor !!!!") );
		::closesocket(hSocket);
		return ERROR_NONE;
	}

	bool bSessionCreate = pSession->Create( hSocket );
	
	pSession->SetIPAddr( strIPAddr );
	pSession->PostCreate();

	bool bAddIOPort = IOCP->AddIOPort( (HANDLE)hSocket, (DWORD)pSession );
	int nReceiveResult = pSession->PrepareReceive();

	if ( bSessionCreate && bAddIOPort && ( nReceiveResult == ERROR_NONE ) )
	{
//		pSession->SetIPAddr( strIPAddr );
//		pSession->PostCreate();

		m_listActive.push_back( pSession );

//		Log( _T("[SM Accept] Active Session Count : %d"), m_listActive.size() );
	}
	else
	{
		Log( _T("SM Accept Fail") );
		pSession->Close( 0, 0 );
	}

	return ERROR_NONE;
}

bool CSessionManager::FindActiveSessionAndSend( LPCTSTR strUserID, CMsgSend& msgSend )
{
	Synchronized sync(&m_soList);

	CSession* pSess = NULL;
	for ( sesslistitor itor = m_listActive.begin(); itor != m_listActive.end(); ++itor )
	{
		pSess = *itor;
		if ( _tcsicmp( pSess->GetUserID(), strUserID ) == 0 )
		{
			pSess->SendMessage( msgSend );
			return true;
		}
	}

	return false;
}

void CSessionManager::Broadcast( CMsgSend& msg, bool bEncrypt )
{
	if (bEncrypt)
		msg.Encrypt();

	Synchronized sync(&m_soList);

	for ( sesslistitor itor = m_listActive.begin(); itor != m_listActive.end(); ++itor )
		(*itor)->SendMessage( msg, false );	
}

void CSessionManager::ErrorOnSession( CSession* pSession, DWORD dwError, int nDetail )
{
	if ( pSession->OnError( dwError, nDetail ) )
	{
		Synchronized sync(&m_soList);

		sesslistitor itor = std::find( m_listActive.begin(), m_listActive.end(), pSession );
		if ( itor != m_listActive.end() )
		{
			m_listActive.erase( itor );
			m_listInActive.push_back( pSession );

//			Log( _T("[SM ErrorOnSession] Active Session Count : %d"), m_listActive.size() );
		}
	}
}

void CSessionManager::CheckAndKillSession()
{
	CSession* pSess = NULL;

	Synchronized sync(&m_soList);
	
	for ( sesslistitor itor = m_listActive.begin(); itor != m_listActive.end(); )
	{
		pSess = *itor;

		if ( *(pSess->GetNotRecvedCount()) > CHECK_COUNT )
			pSess->SendAliveReq();

		if ( *(pSess->GetNotRecvedCount()) > NOT_ALIVE )
		{
			if ( pSess->OnError( ERROR_HEARTBEAT_TIMEOUT, 0 ) )
			{
				// 응답이 없는 세션 나가삼 ㅡ.-;;
				m_listInActive.push_back(pSess);
				itor = m_listActive.erase( itor );
			}
			else
			{
				itor++;
			}
		}
		else
		{
			InterlockedIncrement((LPLONG)pSess->GetNotRecvedCount());
			itor++;
		}
	}		
}

int CSessionManager::GetActiveSessionCount()
{
	Synchronized so( &m_soList );

	return (int)m_listActive.size();
};

void CSessionManager::AddSession( CSession* pSess )
{
	m_listInActive.push_back( pSess );
}

void CSessionManager::RemoveSession( CSession* pSess )
{
	Synchronized sync(&m_soList);

	sesslistitor itor = std::find( m_listActive.begin(), m_listActive.end(), pSess );
	if ( itor != m_listActive.end() )
	{
		m_listInActive.push_back( *itor );
		m_listActive.erase( itor );
	}
}

void CSessionManager::CloseAllSessions()
{
	sesslistitor itor;
	for ( itor = m_listActive.begin(); itor != m_listActive.end(); ++itor )
		(*itor)->Close( 0, 0 );	
}

void CSessionManager::RemoveClosedSession()
{
	CSession* pSess = NULL;

	Synchronized sync( &m_soList );

	for ( sesslistitor itor = m_listActive.begin(); itor != m_listActive.end(); )
	{
		pSess = *itor;

		if ( !pSess->IsActive() )
		{
			m_listInActive.push_back(pSess);
			itor = m_listActive.erase(itor);
		}
		else
		{
			itor++;
		}
	}
}

END_NAMESPACE