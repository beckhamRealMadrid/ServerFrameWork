#include "StdAfx.h"
#include "CharacterService.h"
#include "User.h"
#include "PrivateProfile.h"

CCharacterService::CCharacterService( bool bService ) : CService( bService )
{
	m_nAcceptPort = 14201;
	
	m_pUserSessionManager = NULL;
	m_pSupervisorSession = NULL;	
}

CCharacterService::~CCharacterService(void)
{

}

bool CCharacterService::InitDatabase()
{
	return true;
}

void CCharacterService::DestroyDatabase()
{
	CService::DestroyDatabase();	
}

bool CCharacterService::InitNetwork()
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
	m_pUserSessionManager->CreateSessions <CUser> (nCapacity);

	CSupervisorSession::InitMessageMap();
	m_pSupervisorSession = new CSupervisorSession( NULL );

	m_pSupervisorSession->SetAutoConnInfo( GetServerIP(), 17200 );
	m_pSupervisorSession->StartAutoConnect();

	return true;
}

void CCharacterService::DestroyNetwork()
{
	CService::DestroyNetwork();

	if ( m_pUserSessionManager )
	{
		m_pUserSessionManager->RemoveSessions <CUser> ();
		SAFE_DELETE( m_pUserSessionManager );
	}

	SAFE_DELETE( m_pSupervisorSession );	
}

bool CCharacterService::InitApplication()
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

void CCharacterService::DestroyApplication()
{
	CService::DestroyApplication();
}

void CCharacterService::FuncForException( LPTSTR szFileName )
{
	
}

void CCharacterService::WaitForTerminateSignal()
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