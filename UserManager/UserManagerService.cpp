#include "StdAfx.h"
#include "UserManagerService.h"
#include "ServerSession.h"

CUserManagerService::CUserManagerService( bool bService ) : CService(bService)
{
	m_nAcceptPort = 6000;
	
	m_pUserSessionManager = NULL;
}

CUserManagerService::~CUserManagerService()
{

}

bool CUserManagerService::InitDatabase()
{	
	return true;
}

bool CUserManagerService::InitNetwork()
{
	CService::InitNetwork();

	int nCapacity;
	
	m_pUserSessionManager = new CSessionManager();
	if ( m_Registry.Get( "Capacity", nCapacity ) != 0 )
#ifdef _DEBUG
		nCapacity = 10;
#else
		nCapacity = MAX_CAPACITY;
#endif
	
	m_pUserSessionManager->Create( m_strServerIP, m_nAcceptPort );
	m_pUserSessionManager->CreateSessions <CServerSession> ( nCapacity );

	return true;
}

void CUserManagerService::DestroyDatabase()
{
	CService::DestroyDatabase();	
}

void CUserManagerService::DestroyNetwork()
{
	CService::DestroyNetwork();

	if ( m_pUserSessionManager )
	{
		m_pUserSessionManager->RemoveSessions <CServerSession> ();
		SAFE_DELETE( m_pUserSessionManager );
	}
}

void CUserManagerService::WaitForTerminateSignal()
{
	DWORD dwRet;

	while (true)
	{
		dwRet = WaitForSingleObject( m_hKillService, 1 );
		if (dwRet == WAIT_OBJECT_0)
			return;

		if ( MONITOR->WaitForConsoleKeyInput() )
			return;

		m_pUserSessionManager->RemoveClosedSession();
	}
}