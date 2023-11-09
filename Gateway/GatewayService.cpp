#include "StdAfx.h"
#include "GatewayService.h"
#include "TWClient.h"
#include "ExceptionFilter.h"
#include "OleDBSession.h"
#include "PrivateProfile.h"
#include "EasyRandom.h"
#include "IOCPManager.h"

CGatewayService::CGatewayService( bool bService ) : CService( bService )
{
	m_nAcceptPort = 13200;	

	m_pUserSessionManager = NULL;
	m_pSupervisorSession = NULL;
	m_UserManagerSession = NULL;
}

CGatewayService::~CGatewayService(void)
{

}

bool CGatewayService::InitDatabase()
{
	return true;
}

void CGatewayService::DestroyDatabase()
{
	CService::DestroyDatabase();	
}

bool CGatewayService::InitNetwork()
{
	CService::InitNetwork();

	int nCapacity = 0;

	m_pUserSessionManager = new CSessionManager();
	if ( m_Registry.Get( "Capacity", nCapacity ) != 0 )
#ifdef _DEBUG
		nCapacity = 300;
#else
		nCapacity = MAX_CAPACITY;
#endif

	m_pUserSessionManager->Create( m_strServerIP, m_nAcceptPort );
	m_pUserSessionManager->CreateSessions <CTWClient> ( nCapacity );

	CSupervisorSession::InitMessageMap();
	m_pSupervisorSession = new CSupervisorSession( NULL );

	m_pSupervisorSession->SetAutoConnInfo( GetServerIP(), 17200 );
	m_pSupervisorSession->StartAutoConnect();	

	return true;
}

void CGatewayService::DestroyNetwork()
{
	CService::DestroyNetwork();

	if ( m_pUserSessionManager )
	{
		m_pUserSessionManager->RemoveSessions <CTWClient> ();
		SAFE_DELETE( m_pUserSessionManager );
	}

	SAFE_DELETE( m_pSupervisorSession );
	SAFE_DELETE( m_UserManagerSession );
}

bool CGatewayService::InitApplication()
{
	if ( CService::InitApplication() )
	{
		SetFuncForException( FuncForException, m_nAcceptPort );		

		return true;
	}
	else
	{
		return false;
	}	
}

void CGatewayService::DestroyApplication()
{
	CService::DestroyApplication();
}

void CGatewayService::FuncForException( LPTSTR szFileName )
{
	
}

void CGatewayService::WaitForTerminateSignal()
{
	DWORD dwPrev = ::GetTickCount();
	DWORD dwCurrent = dwPrev;
	DWORD dwDelta = 0;
	DWORD dwRet = 0;
	TCHAR* pstrTarget = NULL;
	CTWClient* pClient = NULL;

	while (true)
	{
		dwRet = WaitForSingleObject( m_hKillService, 1 );
		if ( dwRet == WAIT_OBJECT_0 )
			return;

		if ( MONITOR->WaitForConsoleKeyInput() )
			return;
	}
}