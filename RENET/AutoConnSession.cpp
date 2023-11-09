#include "StdAfx.h"
#include "Autoconnsession.h"
#include "IOCPManager.h"

START_NAMESPACE

CAutoConnSession::CAutoConnSession( CSessionManager* pSM ) : CSession( pSM )
{
	m_dwDelay = 0;
	m_bReconnect = true;
	m_hConnector = ( HANDLE ) -1L;
	m_hKillConnector = ::CreateEvent( NULL, FALSE, FALSE, NULL );
	m_hConnectorKilled = ::CreateEvent( NULL, TRUE, TRUE, NULL );
}

CAutoConnSession::~CAutoConnSession()
{
	m_bReconnect = false;

	Close( 0, 0 );

	m_dwDelay = 100;
	DWORD dwResume = ::ResumeThread( m_hConnector );
	::SetEvent( m_hKillConnector );

	if ( m_hConnector != ( HANDLE ) -1L )
	{
		::WaitForSingleObject( m_hConnector, INFINITE );
		SAFE_CLOSEHANDLE( m_hConnector );
	}

	SAFE_CLOSEHANDLE( m_hKillConnector );
	SAFE_CLOSEHANDLE( m_hConnectorKilled );
}

void CAutoConnSession::SetAutoConnInfo( LPCTSTR szAddr, int nPort )
{
	::StringCchCopy( m_strAutoConnAddr, 50, szAddr );
	m_nAutoConnPort = nPort;
}

bool CAutoConnSession::StartAutoConnect()
{
	unsigned dwID = 0;
	m_hConnector = BEGINTHREADEX( NULL, 0, WorkerThread, this, 0, &dwID );
	if (m_hConnector == (HANDLE)-1L)
		return false;
	else
		return true;
}

bool CAutoConnSession::OnError( DWORD dwErrorCode, int nDetail )
{
	bool bRetVal = CSession::OnError( dwErrorCode, nDetail );

	DWORD dwResume = 0;
	if ( m_bReconnect )
	{
		// 다시 작업하세요..
		m_dwDelay = 100;
		dwResume = ::ResumeThread( m_hConnector );
	}

	return bRetVal;
}

unsigned __stdcall CAutoConnSession::WorkerThread( void* lpParam )
{
	CAutoConnSession* pAutoConnSession = (CAutoConnSession*)lpParam;

	pAutoConnSession->AutoConnector();
	
	_endthreadex(0);
	return 0;
}

void CAutoConnSession::AutoConnector()
{
	DWORD dwSupend = 0;

	::ResetEvent( m_hConnectorKilled );

	m_dwDelay = 100;

	while ( true )
	{
		if ( ::WaitForSingleObject( m_hKillConnector, m_dwDelay ) == WAIT_OBJECT_0 )
		{
			::SetEvent( m_hConnectorKilled );
			return;
		}
		else
		{
			if ( !Connect( GetAutoConnAddr(), GetAutoConnPort() ) )
			{
				m_dwDelay *= 2;
				if ( m_dwDelay > 10 * 1000 )	// 10 seconds
					m_dwDelay = 10 * 1000;

//				Log( "CAutoConnSession Connect Fail..." );
//				Log( "Retry AutoConnect Addr : %s Port : %d", GetAutoConnAddr(), GetAutoConnPort() );		
			}
			else
			{
				bool bAddIOPort = IOCP->AddIOPort( (HANDLE)GetSocket(), (DWORD)this );
				int nReceiveResult = PrepareReceive();
				
				if ( bAddIOPort && ( nReceiveResult == ERROR_NONE ) )
				{
					Log( _T("AutoConnect Success") );

					PostCreate();

					::SetEvent( m_hConnectorKilled );
					_sleep(10);

					// 잠시 꺼두세요..
					dwSupend = ::SuspendThread( m_hConnector );					
				}
				else
				{
					Close( 0, 0 );
				}
			}
		}
	}
}

/*
bool CAutoConnSession::StartAutoConnect()
{
	m_hConnector = (HANDLE)::_beginthreadex( NULL, 0, &CAutoConnSession::AutoConnector, this, 0, NULL );
	if ( m_hConnector == (HANDLE)-1L )
		return false;
	else
	{
		DWORD dwRet = ::WaitForSingleObject( m_hConnector, 1000 * 5 );

		if ( dwRet ==  WAIT_OBJECT_0 )
		{
			bool bAddIOPort = CIOCPManager::GetInstance()->AddIOPort( (HANDLE)m_hSocket, (DWORD)this );
			int nReceiveResult = PrepareReceive();

			if ( bAddIOPort && ( nReceiveResult == ERROR_NONE ) )
			{
				PostCreate();
				return true;
			}
			else
				Close( 0, 0 );						
		}
		else if ( dwRet == WAIT_TIMEOUT )
		{
			// 5초가 지났는데 아직 커넥트 안됬다. 
			return true;
		}

		return false;
	}
}
*/

/*
unsigned __stdcall CAutoConnSession::AutoConnector(LPVOID lpParam)
{
	CAutoConnSession* pSession = (CAutoConnSession*) lpParam;

	::ResetEvent( m_hConnectorKilled );

	DWORD dwDelay = 100;

	while ( true )
	{
		if ( ::WaitForSingleObject( m_hKillConnector, dwDelay ) == WAIT_OBJECT_0 )
		{
			::SetEvent( m_hConnectorKilled );
			_endthreadex(0);
			return 0;
		}
		else
		{
			if ( !Connect( GetAutoConnAddr(), GetAutoConnPort() ) )
			{
				dwDelay *= 2;
				if (dwDelay > 10 * 1000)	// 10 seconds
					dwDelay = 10 * 1000;

				Log( "CAutoConnSession Connect Fail..." );
				Log( "Retry AutoConnect Addr : %s Port : %d", GetAutoConnAddr(), GetAutoConnPort() );
			}
			else
			{
				bool bAddIOPort = CIOCPManager::GetInstance()->AddIOPort( (HANDLE)GetSocket(), (DWORD)pSession );
				int nReceiveResult = PrepareReceive();
				
				if ( bAddIOPort && ( nReceiveResult == ERROR_NONE ) )
				{
					Log( "AutoConnect Success" );

					PostCreate();

					::SetEvent( m_hConnectorKilled );
					_sleep(10);
					_endthreadex(0);

					return 0;
				}
				else
				{
					Close( 0, 0 );
				}
			}
		}
	}
}
*/

/*
CAutoConnSession::~CAutoConnSession()
{
	m_bReconnect = false;
	
	Close( 0, 0 );

	::SetEvent( m_hKillConnector );
	
	if ( m_hConnector != ( HANDLE ) -1L )
	{
		::SignalObjectAndWait( m_hKillConnector, m_hConnectorKilled, INFINITE, FALSE );
		::CloseHandle( m_hConnector );
	}

	::CloseHandle( m_hKillConnector );
	::CloseHandle( m_hConnectorKilled );
}
*/

/*
bool CAutoConnSession::OnError( DWORD dwErrorCode, int nDetail )
{
	bool bRetVal = CSession::OnError( dwErrorCode, nDetail );

	if ( m_bReconnect )
	{
		if ( m_hConnector )
			::CloseHandle( m_hConnector );

		StartAutoConnect();		
	}

	return bRetVal;
}
*/

END_NAMESPACE