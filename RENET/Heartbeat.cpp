#include "StdAfx.h"
#include "Heartbeat.h"
#include "SessionManager.h"

START_NAMESPACE

CHeartbeat::CHeartbeat( CSessionManager* pSM, UINT nTime ) : m_pSessionManager(pSM)
{
	m_hThread = INVALID_HANDLE_VALUE;
	m_hEventExit = INVALID_HANDLE_VALUE;
	m_nTime = nTime;
	m_hEventExit = ::CreateEvent( NULL, 0, 0, NULL );
	m_bInService = false;
}

CHeartbeat::~CHeartbeat(void)
{
	if( m_bInService )
		StopHeartbeat();

	SAFE_CLOSEHANDLE( m_hEventExit );
	SAFE_CLOSEHANDLE( m_hThread );	
}

bool CHeartbeat::StartHeartbeat()
{
	unsigned dwID = 0;
	m_hThread = BEGINTHREADEX( NULL, 0, WorkerThread, this, 0, &dwID );
	if ( !m_hThread )
		return false;

	m_bInService = true;

	return true;
}

void CHeartbeat::StopHeartbeat()
{
	::SetEvent( m_hEventExit );

	DWORD dwWait = ::WaitForSingleObject( m_hThread, 3000 );
	if( dwWait == WAIT_TIMEOUT )
	{
		DWORD dwExitCode;

		::GetExitCodeThread( m_hThread, &dwExitCode );
		if ( dwExitCode == STILL_ACTIVE )
			::TerminateThread( m_hThread, 0 );
	}

	m_hThread = INVALID_HANDLE_VALUE;
	m_bInService = false;
}

unsigned __stdcall CHeartbeat::WorkerThread( void* lpParam )
{
	CHeartbeat* pHeartbeat = (CHeartbeat*)lpParam;

	pHeartbeat->HeartbeatCheck();

	_endthreadex( 0 );
	return 0;
}

void CHeartbeat::HeartbeatCheck()
{
	for(;;)
	{
		DWORD dwWait = ::WaitForSingleObject( m_hEventExit, m_nTime );
		if ( dwWait == WAIT_OBJECT_0 )
			return;
		else if ( dwWait == WAIT_TIMEOUT )
			CheckAndDrop();		
	}
}

void CHeartbeat::CheckAndDrop()
{
//	m_pSessionManager->CheckAndKillSession();
}

END_NAMESPACE