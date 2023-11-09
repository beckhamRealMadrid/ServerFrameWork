#include "StdAfx.h"
#include "GameService.h"
#include "User.h"
#include "Client.h"
#include "ServerSession.h"
#include "PrivateProfile.h"

CGameService::CGameService( bool bService ) : CService( bService )
{
	m_nAcceptPort = 15201;
	m_nAcceptServerPort = m_nAcceptPort + 4000;
	m_nAcceptUDPPort = 19200;
		
	m_pUserSessionManager = NULL;
	m_pConnectionManager = NULL;
	m_pSupervisorSession = NULL;
	m_pChannelSession = NULL;	
}

CGameService::~CGameService(void)
{
	
}

bool CGameService::InitDatabase()
{
	return true;
}

void CGameService::DestroyDatabase()
{
	CService::DestroyDatabase();	
}

bool CGameService::InitNetwork()
{
	CService::InitNetwork();

	int nCapacity = 0;

	m_pUserSessionManager = new CSessionManager();
	m_pConnectionManager = new CConnectionManager();

	if ( m_Registry.Get( "Capacity", nCapacity ) != 0 )
#ifdef _DEBUG
		nCapacity = 300;
#else
		nCapacity = MAX_CAPACITY;
#endif

	m_pUserSessionManager->Create( m_strServerIP, m_nAcceptPort );
	m_pUserSessionManager->CreateSessions <CUser> (nCapacity);

	m_pConnectionManager->Create( m_strServerIP, m_nAcceptUDPPort );
	m_pConnectionManager->CreateSessions <CClient> (nCapacity);

	CSupervisorSession::InitMessageMap();
	CChannelSession::InitMessageMap();

	m_pSupervisorSession = new CSupervisorSession( NULL );
	m_pChannelSession = new CChannelSession( NULL );

	m_pSupervisorSession->SetAutoConnInfo( GetServerIP(), 17200 );
	m_pSupervisorSession->StartAutoConnect();

	m_pChannelSession->SetAutoConnInfo( GetServerIP(), 14202 + 4000 );
	m_pChannelSession->StartAutoConnect();

	return true;
}

void CGameService::DestroyNetwork()
{
	CService::DestroyNetwork();

	if ( m_pUserSessionManager )
	{
		m_pUserSessionManager->RemoveSessions <CUser> ();
		SAFE_DELETE( m_pUserSessionManager );
	}

	if ( m_pConnectionManager )
	{
		m_pConnectionManager->Destroy();
		m_pConnectionManager->RemoveSessions <CClient> ();
		SAFE_DELETE( m_pConnectionManager );
	}

	SAFE_DELETE( m_pSupervisorSession );
	SAFE_DELETE( m_pChannelSession );

	Log( _T("Destroy User Session Manager") );
}

bool CGameService::InitApplication()
{
	if ( CService::InitApplication() )
	{
		return true;
	}
	else
	{
		return false;
	}	
}

void CGameService::DestroyApplication()
{
	CService::DestroyApplication();
}

void CGameService::WaitForTerminateSignal()
{
	DWORD dwPrev = ::GetTickCount();
	DWORD dwCurrent = dwPrev;
	DWORD dwDelta = 0;
	DWORD dwRet = 0;

	while (true)
	{
		dwRet = WaitForSingleObject( m_hKillService, 1 );
		if ( dwRet == WAIT_OBJECT_0 )
			return;

		if ( MONITOR->WaitForConsoleKeyInput() )
			return;
	}
}