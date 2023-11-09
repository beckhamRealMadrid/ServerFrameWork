#include "StdAfx.h"
#include "ChannelService.h"
#include "User.h"
#include "ServerSession.h"
#include "PrivateProfile.h"

CChannelService::CChannelService( bool bService ) : CService( bService )
{
	m_nAcceptPort = 14202;
	m_nAcceptServerPort = m_nAcceptPort + 4000;
	
	m_pUserSessionManager = NULL;
	m_pServerSessionManager = NULL;
	m_pSupervisorSession = NULL;	
}

CChannelService::~CChannelService(void)
{

}

bool CChannelService::InitDatabase()
{
	return true;
}

void CChannelService::DestroyDatabase()
{
	CService::DestroyDatabase();	
}

bool CChannelService::InitNetwork()
{
	CService::InitNetwork();

	int nCapacity = 0;

	m_pUserSessionManager = new CSessionManager();
	m_pServerSessionManager = new CSessionManager();

	if ( m_Registry.Get( "Capacity", nCapacity ) != 0 )
#ifdef _DEBUG
		nCapacity = 300;
#else
		nCapacity = MAX_CAPACITY;
#endif

	m_pUserSessionManager->Create( m_strServerIP, m_nAcceptPort );
	m_pUserSessionManager->CreateSessions <CUser> (nCapacity);

	m_pServerSessionManager->CreateSessions <CServerSession> (10);

	CSupervisorSession::InitMessageMap();
	m_pSupervisorSession = new CSupervisorSession( NULL );

	m_pSupervisorSession->SetAutoConnInfo( GetServerIP(), 17200 );
	m_pSupervisorSession->StartAutoConnect();	

	return true;
}

void CChannelService::DestroyNetwork()
{
	CService::DestroyNetwork();

	if ( m_pUserSessionManager )
	{
		m_pUserSessionManager->RemoveSessions <CUser> ();
		SAFE_DELETE( m_pUserSessionManager );
	}

	if ( m_pServerSessionManager )
	{
		m_pServerSessionManager->RemoveSessions <CServerSession> ();
		SAFE_DELETE( m_pServerSessionManager );
	}

	SAFE_DELETE( m_pSupervisorSession );

	Log( _T("Destroy User Session Manager") );
}

bool CChannelService::InitApplication()
{
	if ( CService::InitApplication() )
	{
		m_pServerSessionManager->Create( m_strServerIP, m_nAcceptServerPort );

		return true;
	}
	else
	{
		return false;
	}	
}

void CChannelService::DestroyApplication()
{
	CService::DestroyApplication();
}

void CChannelService::WaitForTerminateSignal()
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

#ifdef _DEBUG
		if ( MONITOR->WaitForConsoleKeyInput() )
			return;
#endif		
	}
}