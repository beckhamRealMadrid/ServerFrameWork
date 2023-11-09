#include "stdafx.h"
#include "IOCPManager.h"
#include "Synchronized.h"
#include "Netmsg.h"
#include "BaseSession.h"

START_NAMESPACE

CIOCPManager::CIOCPManager()
{
	m_nWorkerThreadCount = 0;
	m_arrWorkerThread = NULL;
	m_hIOCP = NULL;
	m_hKillManager = NULL;
}

CIOCPManager::~CIOCPManager()
{
	
}

bool CIOCPManager::Create( int nWorkerThreadCount )
{
	if ( m_hIOCP == NULL )
	{
		if ( ( m_hIOCP = ::CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, nWorkerThreadCount ) ) == NULL )
		{
			return false;
		}
	}

	if ( nWorkerThreadCount == 0 )
	{
		SYSTEM_INFO sysInfo;

		::GetSystemInfo(&sysInfo);
		nWorkerThreadCount = sysInfo.dwNumberOfProcessors * 2 - 1;
#ifdef _DEBUG
		nWorkerThreadCount = 4;
#endif
	}

	m_nWorkerThreadCount = nWorkerThreadCount;
	m_arrWorkerThread = new HANDLE[nWorkerThreadCount];
	m_hKillManager = ::CreateEvent( NULL, TRUE, FALSE, NULL );

	unsigned dwID = 0;
	for ( int i = 0; i < nWorkerThreadCount ; i++ )
	{
		m_arrWorkerThread[i] = BEGINTHREADEX( NULL, 0, WorkerThread, this, 0, &dwID );			
	}
		
	return true;
}

void CIOCPManager::Destroy()
{
	if ( m_hKillManager != NULL && m_hIOCP != NULL )
	{
		::SetEvent( m_hKillManager );
		
		if ( m_nWorkerThreadCount )
		{
			int i;
			for ( i = 0; i < m_nWorkerThreadCount * 2; i++ )
			{
				// just send waste signal to GetQueuedCompletionStatus for wake up
				::PostQueuedCompletionStatus( m_hIOCP, 0, 0, NULL ); 
			}

			::WaitForMultipleObjects( m_nWorkerThreadCount, m_arrWorkerThread, TRUE, INFINITE );
			
			for ( i = 0 ; i < m_nWorkerThreadCount; i++ )
			{
				if ( m_arrWorkerThread[i] != NULL )
				{
					DWORD dwExitCode;

					::GetExitCodeThread( m_arrWorkerThread[i], &dwExitCode );
					
					if ( dwExitCode == STILL_ACTIVE )
						::TerminateThread( m_arrWorkerThread[i], 0 );

					SAFE_CLOSEHANDLE( m_arrWorkerThread[i] );
				}
			}

			delete [] m_arrWorkerThread;
		}

		SAFE_CLOSEHANDLE( m_hKillManager );
		SAFE_CLOSEHANDLE( m_hIOCP );		
	}
}

unsigned __stdcall CIOCPManager::WorkerThread( void* lpParam )
{
	CIOCPManager* pIOCP = (CIOCPManager*)lpParam;

	while ( true )
	{
		if ( ::WaitForSingleObject( pIOCP->GetKillHandle(), 0 ) == WAIT_OBJECT_0 )
		{
			_endthreadex(0);
			return 0;
		}

		pIOCP->WaitForSignal();
	}

	assert( false );
	_endthreadex(0);

	return 0;
}

bool CIOCPManager::AddIOPort( HANDLE IOHandle, DWORD dwKeyValue )
{
	if ( ( ::CreateIoCompletionPort( IOHandle, m_hIOCP, dwKeyValue, 0 ) ) == NULL )
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool CIOCPManager::PostCompletion( DWORD dwCompletionKey, OVERLAPPED_BASE* pContext )
{
	if ( m_hIOCP == NULL )
		return FALSE;

	if ( ::PostQueuedCompletionStatus( m_hIOCP, NULL, dwCompletionKey, (LPOVERLAPPED)pContext ) == FALSE )
	{
		return FALSE;
	}

	return TRUE;
}

DWORD CIOCPManager::WaitForSignal()
{
	DWORD			dwTransSize = 0;
	LPOVERLAPPED	lpov		= NULL;
	DWORD			dwKeyValue	= 0;
	DWORD			dwErrorCode = 0;
	DWORD			dwLastError = 0;
	BOOL			bStatus		= false;

	bStatus = ::GetQueuedCompletionStatus( m_hIOCP, &dwTransSize, &dwKeyValue, &lpov, INFINITE );
	
	if ( bStatus && dwKeyValue && lpov )
	{
		CBaseSession* pBaseSess = reinterpret_cast<CBaseSession*>(dwKeyValue);

		dwErrorCode = pBaseSess->Dispatch( dwTransSize, (LPOVERLAPPED_BASE)lpov );

		switch ( dwErrorCode )
		{
			case ERROR_NONE :	break;
			default :
				{			
					dwLastError = WSAGetLastError();
					pBaseSess->OnErrorSession( dwErrorCode, dwLastError );                    
				}				
				break;
		}
	}
	else
	{
		dwLastError = WSAGetLastError();

		if ( dwKeyValue )
		{
			dwLastError = WSAGetLastError();
			CBaseSession* pBaseSess = (CBaseSession*)dwKeyValue;

			if ( dwLastError != ERROR_OPERATION_ABORTED )
			{
				pBaseSess->OnErrorSession( ERROR_INVALIDSOCKET, dwLastError ); 				
			}
		}
		else
		{
			Log( _T("GQCS Failed %d, %d, %d, %x"), bStatus, (int)dwTransSize, dwKeyValue, lpov );
		}
	}

	return 0;
}

END_NAMESPACE