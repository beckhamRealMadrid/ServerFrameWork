#include "StdAfx.h"
#include "SupervisorService.h"
#include "ExceptionFilter.h"
#include "ServerSession.h"
#include "UserGuideManager.h"
#include "PrivateProfile.h"

CSupervisorService::CSupervisorService( bool bService ) : CService( bService )
{
	m_nAcceptPort = 17200;
	
	m_pServerSessionManager = NULL;
}

CSupervisorService::~CSupervisorService(void)
{
	
}

bool CSupervisorService::InitDatabase()
{
	return true;
}

void CSupervisorService::DestroyDatabase()
{
	CService::DestroyDatabase();	
}

bool CSupervisorService::InitNetwork()
{
	CService::InitNetwork();

	int nCapacity;

	m_pServerSessionManager = new CSessionManager();
	if ( m_Registry.Get( "Capacity", nCapacity ) != 0 )
#ifdef _DEBUG
		nCapacity = 300;
#else
		nCapacity = MAX_CAPACITY;
#endif

	m_pServerSessionManager->CreateSessions <CServerSession> (nCapacity);

	return true;
}

void CSupervisorService::DestroyNetwork()
{
	CService::DestroyNetwork();	

	if ( m_pServerSessionManager )
	{
		m_pServerSessionManager->RemoveSessions <CServerSession> ();
		SAFE_DELETE( m_pServerSessionManager );
	}

	Log( _T("Destroy Server Session Manager") );
}

bool CSupervisorService::InitApplication()
{
	if ( CService::InitApplication() )
	{
		m_pUserGuideManager = new CUserGuideManager;
				
		m_pServerSessionManager->Create( m_strServerIP, m_nAcceptPort );

		SetFuncForException( FuncForException, m_nAcceptPort );

		return true;
	}
	else
	{
		return false;
	}	
}

void CSupervisorService::DestroyApplication()
{
	SAFE_DELETE( m_pUserGuideManager );

	CService::DestroyApplication();
}

void CSupervisorService::FuncForException( LPTSTR szFileName )
{
	
}

void CSupervisorService::WaitForTerminateSignal()
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

		dwCurrent = ::GetTickCount();
		dwDelta	  = dwCurrent - dwPrev;
		dwPrev	  = dwCurrent;		

		m_ScheduledJob.Update( dwDelta );
	}
}