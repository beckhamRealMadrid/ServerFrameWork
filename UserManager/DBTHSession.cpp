#include "StdAfx.h"
#include "DBTHSession.h"

CDBTHSession::CDBTHSession(void)
{
	m_pTHOleDBSource = NULL;
	m_hThread = INVALID_HANDLE_VALUE;
	m_hEvent[0] = INVALID_HANDLE_VALUE;
	m_hEvent[1] = INVALID_HANDLE_VALUE;	
}

CDBTHSession::~CDBTHSession(void)
{
	SAFE_DELETE( m_pTHOleDBSource );
	
	::SetEvent( m_hEvent[CLOSE_THREAD] );

	DWORD dwWait = ::WaitForSingleObject( m_hThread, INFINITE );
	if( dwWait == WAIT_TIMEOUT )
	{
		DWORD dwExitCode;

		::GetExitCodeThread( m_hThread, &dwExitCode );
		if ( dwExitCode == STILL_ACTIVE )
			::TerminateThread( m_hThread, 0 );
	}

	SAFE_CLOSEHANDLE( m_hThread );
	SAFE_CLOSEHANDLE( m_hEvent[CLOSE_THREAD] );
	SAFE_CLOSEHANDLE( m_hEvent[DB_RESULT] );
}

bool CDBTHSession::Create( LPCSTR szIP, LPCSTR szCatalog, LPCSTR szUserID, LPCSTR szPasswd )
{
	m_hEvent[0] = ::CreateEvent( NULL, FALSE, FALSE, NULL );
	m_hEvent[1] = ::CreateEvent( NULL, FALSE, FALSE, NULL );

	unsigned dwID = 0;
	m_hThread = BEGINTHREADEX( NULL, 0, WorkerThread, this, 0, &dwID );
	if ( m_hThread == (HANDLE)-1L )
		return false;

	m_pTHOleDBSource = new CTHOleDBSource();
	if ( NULL == m_pTHOleDBSource )
		return false;

	m_pTHOleDBSource->Create( szIP, szCatalog, szUserID, szPasswd );

	DB_INITIALIZE_DESC desc;
	ZeroMemory( &desc, sizeof(DB_INITIALIZE_DESC) );

	desc.bUsingThread		= TRUE;					
	desc.bUsingEventObject	= TRUE;					
	desc.hWndToPostMessage	= NULL;					
	desc.uMessage			= NULL;							
#ifdef _DEBUG
	desc.wMaxNumOfProcessMessage_Input	= 30;		
	desc.wMaxNumOfProcessMessage_Output = 30;		
#else
	desc.wMaxNumOfProcessMessage_Input	= 1000;		
	desc.wMaxNumOfProcessMessage_Output = 1000;		
#endif
	desc.wMaxRowPerRead		= 50;					
	desc.wMaxReturnedRowNum = 50;					
	desc.wMaxReturnedColNum = 128;					
	desc.dwMaxRowSize		= 1024;					
	desc.bMaxParamNum		= 100;	
	desc.ReceiveFunc		= ReceivedFromDB;		
	desc.OutputMessageFunc	= DisplayDBMessage;		
	desc.ReportFunc			= DisplayDBReport;	

	m_pTHOleDBSource->InitDBModule( &desc );	
	m_pTHOleDBSource->SetDBResultEvent( m_hEvent[DB_RESULT] );
	m_pTHOleDBSource->SetPerformanceReport( TRUE );

	return true;
}

unsigned __stdcall CDBTHSession::WorkerThread( void *lpParamaeter )
{
	CDBTHSession* pDBTHSession = (CDBTHSession*)lpParamaeter;

	DWORD dwIndex = 0;

	while ( true )
	{
		dwIndex = ::WaitForMultipleObjects( 2, pDBTHSession->GetEventHandle(), FALSE, INFINITE );
		if ( CLOSE_THREAD == dwIndex )
		{
			break;
		}
		else if ( DB_RESULT == dwIndex )
		{
			if ( pDBTHSession->GetTHOleDBSource() )
                pDBTHSession->GetTHOleDBSource()->GetDBResult();
		}
	}

	return 0;
}

void CDBTHSession::ReceivedFromDB( DBRECEIVEDATA* pResult )
{
	switch( pResult->bQueryType )
	{
		case QUERY_TYPE_SELECT:	// THOpenRecord 
		{
			if( !pResult->Query.select.pResult || !pResult->Query.select.dwRowCount )
			{
				Log( _T("pResult->Query.select.pResult == NULL") );
				return;
			}
			else
			{
				if( pResult->Query.select.dwRowCount == 0 )
					Log( _T("pResult->Query.select.pResult != NULL, dwRowCount == 0") );
			}

			switch( pResult->dwQueryUID )
			{
				case 0:	break;
				case 1: break;
			}
		}
		break;

		case QUERY_TYPE_EXECUTE: // THExecuteSQL
		{
			if( pResult->nError < 0 )
				break;
				
			switch( pResult->dwQueryUID )
			{
				case 0:	break;
				case 1: break;
			}			
		}
		break;

		case QUERY_TYPE_EXECUTE_BY_PARAM:	// THExecuteSQLByParam
		{
			switch( pResult->dwQueryUID )
			{
				case 0:	break;
				case 1: break;	
			}
		}
		break;
	}	
}

void CDBTHSession::DisplayDBMessage( char* szMsg )
{
	
}

void CDBTHSession::DisplayDBReport( char* szMsg )
{
	
}