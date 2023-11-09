#include "StdAfx.h"
#include "THOleDBSource.h"
#include "OleDBSession.h"
#include "ThreadSafeStaticMemPool.h"

START_NAMESPACE

VOID CALLBACK InformInputQueryThreadAPC( ULONG_PTR dwParam )
{
	dwParam;
}

CTHOleDBSource::CTHOleDBSource(void)
{
	m_pInputQueue = NULL;
	m_pOutputMsgPool = NULL;

	m_hDBThread = INVALID_HANDLE_VALUE;
	m_hDBKillEvent = INVALID_HANDLE_VALUE;
	m_hResultEvent = INVALID_HANDLE_VALUE;

	m_bCloseQueryForThread = FALSE;
	m_bEnableReport = FALSE;

	m_hDBKillEvent = ::CreateEvent( NULL, FALSE, FALSE, NULL );
}

CTHOleDBSource::~CTHOleDBSource(void)
{
	if( m_InitDesc.bUsingThread )
	{
		::SetEvent( m_hDBKillEvent );
		::WaitForSingleObject( m_hDBThread, INFINITE );

		SAFE_CLOSEHANDLE( m_hDBThread );
		SAFE_CLOSEHANDLE( m_hDBKillEvent );		
	}

	SAFE_DELETE( m_pInputQueue );
	SAFE_DELETE( m_pOutputQueue );	
}

COleDBSession* CTHOleDBSource::ReserveNewOleDBSession( DWORD dwThreadID )
{
//	Log( _T("Reserve New DB for Thread ID : %d"), dwThreadID );

	COleDBSession *pOleDBSession = new COleDBSession( &m_InitDesc, this );

	int nRet = pOleDBSession->Create( 20, 10, FALSE );
	if ( 1 == nRet )
	{
		Synchronized sa( &m_soOleDBSession );

		imapOleDBSession itor = m_mapOleDBSession.find( dwThreadID );
		if (itor != m_mapOleDBSession.end())
		{
			delete (*itor).second;
			m_mapOleDBSession.erase(itor);
		}

		m_mapOleDBSession.insert( mapOleDBSession::value_type( dwThreadID, pOleDBSession ) );

		return pOleDBSession;
	}
	else
	{
		delete pOleDBSession;
		pOleDBSession = NULL;
		return NULL;
	}
}

BOOL CTHOleDBSource::KSCToUnicode( char *pKsc, WCHAR *pUni )
{
	int nUniSize = 0;
	int nKscSize = strlen(pKsc);

	if(nKscSize <= 0)
		return FALSE;

	// 1. 먼저 유니코드로 변환하고자 하는 문자열의 크기를 알아낸다.
	nUniSize = MultiByteToWideChar( CP_ACP, 0, pKsc, nKscSize+1, NULL, 0 );

	// 2. 유니코드의 크기(uni_size)와 변환하고자하는 소스문자열(pViewString)과 목적문자열(string)을 이용한다.
	MultiByteToWideChar( CP_ACP, 0, pKsc, nKscSize, pUni, nUniSize*2+100 );

	pUni[nUniSize] = L'\0';

	return TRUE;
}

BOOL CTHOleDBSource::UnicodeToKSC( WCHAR *pUni, char *pKsc, int nKscSize )
{
	int nMultibyteSize = 0;
	nMultibyteSize = WideCharToMultiByte( CP_ACP, 0, pUni, wcslen(pUni)+1, pKsc, nKscSize, NULL, 0 );

	pKsc[ nMultibyteSize ] = '\0';
	return TRUE;
}

unsigned __stdcall CTHOleDBSource::WorkerThread( void* lpParam )
{
	CTHOleDBSource* pTHOleDBSource = (CTHOleDBSource*)lpParam;

	pTHOleDBSource->ProcessQuery();

	_endthreadex( 0 );
	return 0;
}

void CTHOleDBSource::ProcessQuery()
{
	DWORD		dwRet = 0;
	POSITION_	pos = NULL;
	LPDBCMDMSG	pMsg = NULL;
	DWORD		dwInputQCount = 0;
	DWORD		dwCurCount = 0;
	DWORD		dwTime = 0;

	for(;;)
	{	
		dwRet = ::WaitForSingleObjectEx( m_hDBKillEvent, INFINITE, TRUE );
		
		if ( dwRet == WAIT_OBJECT_0 )
		{
			return;	// 종료하라~!!!
		}
		else if ( dwRet == WAIT_IO_COMPLETION )	// 들어온 쿼리가 있다~ 처리하자.
		{
			// adding 큐와 process 큐를 교체한다.
			SwitchInputQueue();
			
			dwCurCount = 0;
			dwInputQCount = m_pInputQueue->GetCount(FALSE);				

			pos = m_pInputQueue->GetHeadPosition();
			while( pos )
			{				
				pMsg = (LPDBCMDMSG)m_pInputQueue->GetNext(pos);
				if( !pMsg )
				{
					Log( _T("### OLEDB Warning! : InputMsg Pointer in list is NULL") );
					continue;
				}

				dwCurCount++;

				if( m_bEnableReport )
				{
					dwTime = GetTickCount();

					switch( pMsg->bQueryType )
					{
						case QUERY_TYPE_SELECT:				ProcessSelectQuery(pMsg);			break;
						case QUERY_TYPE_EXECUTE:			ProcessExecuteQuery(pMsg);			break;
						case QUERY_TYPE_EXECUTE_BY_PARAM:	ProcessExecuteWithParamQuery(pMsg); break;
						case QUERY_TYPE_CHANGE_DB:			ProcessChangeDBQuery(pMsg);			break;
					}

					dwTime = GetTickCount() - dwTime;

					char szBuf[1024] = {0,};
					sprintf(szBuf, "[DB Report - ThreadID:%u, QueryID:%u] "
							       "Inner Input Queue: %u/%u, Outer Input Queue: %u, "
								   "Execute Time: %u\n", pMsg->dwThreadID, pMsg->dwQueryUID, dwCurCount, dwInputQCount, m_pInputQueue->GetCount(TRUE), dwTime);
					
					OutputReport( szBuf );
				}
				else
				{
					switch(pMsg->bQueryType)
					{
						case QUERY_TYPE_SELECT:				ProcessSelectQuery(pMsg);			break;
						case QUERY_TYPE_EXECUTE:			ProcessExecuteQuery(pMsg);			break;
						case QUERY_TYPE_EXECUTE_BY_PARAM:	ProcessExecuteWithParamQuery(pMsg); break;
						case QUERY_TYPE_CHANGE_DB:			ProcessChangeDBQuery(pMsg);			break;
					}
				}

				m_pInputMsgPool->Free( pMsg );
			}				
		}

		m_pInputQueue->RemoveAll();
		
		// 만약 현재 adding 용 큐가 비어있지 않다면, 
		// 다시 처리하도록 알려준다. (자기 자신에 apc한다.... 이상하지만서도..)
		if( m_pInputQueue->GetCount(TRUE) )
		{
			InformDBInput();
		}
	}
}

BOOL CTHOleDBSource::InitDBModule( DB_INITIALIZE_DESC* desc )
{
	m_dwOLEDBSize = 0;

	memcpy( &m_InitDesc, desc, sizeof(m_InitDesc) );

	if( m_InitDesc.bUsingThread )
	{
		CHECK_MEMORY();

		m_pInputMsgPool = new CThreadSafeStaticMemPool(sizeof(DBCommandMsg),
			m_InitDesc.wMaxNumOfProcessMessage_Input/2,
			m_InitDesc.wMaxNumOfProcessMessage_Input);

		if( !m_pInputMsgPool )
			__asm int 3

		CHECK_MEMORY();
		
		m_dwOLEDBSize += (sizeof(DBCommandMsg) * m_InitDesc.wMaxNumOfProcessMessage_Input);

		m_pResultPool = new CThreadSafeStaticMemPool(desc->dwMaxRowSize * desc->wMaxReturnedRowNum,
			desc->wMaxNumOfProcessMessage_Output/2,
			desc->wMaxNumOfProcessMessage_Output);
		
		if( !m_pResultPool )
			__asm int 3

		CHECK_MEMORY();

		m_dwOLEDBSize += (desc->dwMaxRowSize * desc->wMaxReturnedRowNum * desc->wMaxNumOfProcessMessage_Output);
		
		m_pBindingPool = new CThreadSafeStaticMemPool(sizeof(DBBINDING)*desc->wMaxReturnedColNum,
			desc->wMaxNumOfProcessMessage_Output/2,
			desc->wMaxNumOfProcessMessage_Output);
		
		if( !m_pBindingPool )
			__asm int 3		

		CHECK_MEMORY();

		m_dwOLEDBSize += (sizeof(DBBINDING)*desc->wMaxReturnedColNum * desc->wMaxNumOfProcessMessage_Output);

		m_pOutputMsgPool = new CThreadSafeStaticMemPool(sizeof(DBRECEIVEDATA),
			desc->wMaxNumOfProcessMessage_Output/2,
			desc->wMaxNumOfProcessMessage_Output);
		
		if( !m_pOutputMsgPool )
			__asm int 3

		CHECK_MEMORY();

		m_dwOLEDBSize += (sizeof(DBRECEIVEDATA) * desc->wMaxNumOfProcessMessage_Output);

		m_pParamPool = new CThreadSafeStaticMemPool(sizeof(DBBINDING) * desc->bMaxParamNum,
			desc->wMaxNumOfProcessMessage_Input/2,
			desc->wMaxNumOfProcessMessage_Input);
		
		if( !m_pParamPool )
			__asm int 3

		CHECK_MEMORY();

		m_dwOLEDBSize += (sizeof(DBRECEIVEDATA) * desc->bMaxParamNum * desc->wMaxNumOfProcessMessage_Input);

		// 명령을 받을 2개의 메세지 큐 생성 
		m_pInputQueue = new CMsgQueue(m_InitDesc.wMaxNumOfProcessMessage_Input);
		if( !m_pInputQueue )
			__asm int 3

		CHECK_MEMORY();

		if( desc->bUsingEventObject )
		{
			// 출력용 2개의 메세지 큐 생성 
			m_pOutputQueue = new CMsgQueue(m_InitDesc.wMaxNumOfProcessMessage_Output); 				
			if( !m_pOutputQueue )
				__asm int 3
		}
		
		CHECK_MEMORY();

		unsigned dwID = 0;
		m_hDBThread = BEGINTHREADEX( NULL, 0, WorkerThread, this, 0, &dwID );
	}

	return TRUE;
}

void CTHOleDBSource::THOpenRecord( char* szQuerySQL, DWORD dwQueryUID, void* pData, DWORD dwMaxNumRows )
{	
	LPDBCMDMSG pMsg = (LPDBCMDMSG)m_pInputMsgPool->Alloc();
	if ( !pMsg )
	{
		Log( _T("[OLEDB Fatal Error] InputMessagePool Overflow!(CTHOleDBSource::THOpenRecord, Query:%s"), szQuerySQL );
		return;
	}

	KSCToUnicode( szQuerySQL, pMsg->wszQuery );
	pMsg->bQueryType = QUERY_TYPE_SELECT;
	pMsg->dwThreadID = ::GetCurrentThreadId();	
	pMsg->AddInfo.select.dwMaxNumRows = dwMaxNumRows;			
	pMsg->AddInfo.select.dwRowPerRead = DEFAULT_ROWS_PER_READ;
	pMsg->dwQueryUID = dwQueryUID;
	pMsg->pData = pData;

	PostDBMessage( pMsg );	
}

void CTHOleDBSource::THExecuteSQL( char* szQuerySQL, BYTE bReturnResult, DWORD dwQueryUID, void* pData )
{
	LPDBCMDMSG pMsg = (LPDBCMDMSG)m_pInputMsgPool->Alloc();
	if( !pMsg )	
	{
		Log( _T("[OLEDB Fatal Error] InputMessagePool Overflow!(CTHOleDBSource::THExecuteSQL, Query:%s)"), szQuerySQL );
		return;
	}

	KSCToUnicode( szQuerySQL, pMsg->wszQuery );
	pMsg->bQueryType = QUERY_TYPE_EXECUTE;
	pMsg->dwThreadID = ::GetCurrentThreadId();
	pMsg->AddInfo.execute.bReturnResult = bReturnResult;	// 몇행영향을 받았나 결과값 받을것인지 안받을것인지 
	pMsg->dwQueryUID = dwQueryUID;
	pMsg->pData = pData;

	PostDBMessage( pMsg );
}

void CTHOleDBSource::THExecuteSQLByParam( char* szQuerySQL, DBBINDING* pBinding, void* pParamValue, DWORD dwParamValueSize, BYTE bParamNum,
                                         BYTE bReturnResult, DWORD dwQueryUID,  void* pData )
{
	if( dwParamValueSize > MAX_PARAM_VALUE_SIZE )
	{
		Log( _T("[OLEDB Fatal Error] Param value size overflow!(CTHOleDBSource::THExecuteSQLByParam)") );
		return;
	}

	LPDBCMDMSG pMsg = (LPDBCMDMSG)m_pInputMsgPool->Alloc();
	if( !pMsg )	
	{
		Log( _T("[OLEDB Fatal Error] InputMessagePool Overflow!(CTHOleDBSource::THExecuteSQLByParam, Query:%s)"), szQuerySQL );
		return;
	}

	KSCToUnicode( szQuerySQL, pMsg->wszQuery );	
	pMsg->bQueryType = QUERY_TYPE_EXECUTE_BY_PARAM;
	pMsg->dwThreadID = ::GetCurrentThreadId();
	pMsg->AddInfo.execute_by_param.bReturnResult = bReturnResult;
	pMsg->AddInfo.execute_by_param.pBinding = pBinding;
	pMsg->AddInfo.execute_by_param.bParamNum = bParamNum;
	memcpy( pMsg->AddInfo.execute_by_param.szParamValueBuf, pParamValue, dwParamValueSize );
	pMsg->dwQueryUID = dwQueryUID;
	pMsg->pData = pData;

	PostDBMessage( pMsg );
}

DBBINDING* CTHOleDBSource::THCreateParamInfo( WORD wParamNum )
{
	DBBINDING* pDbBinding = (DBBINDING*)m_pParamPool->Alloc();
	if( !pDbBinding )	
	{
		Log( _T("CTHOleDBSource Error! - Parameter Memory Pool is Full!") );
		return NULL;
	}

	for( int i = 0; i < (int)wParamNum; i++ )
	{
		pDbBinding[i].obLength	= 0;
		pDbBinding[i].obStatus	= 0;
		pDbBinding[i].pTypeInfo = NULL;
		pDbBinding[i].pObject	= NULL;
		pDbBinding[i].pBindExt	= NULL;
		pDbBinding[i].dwPart	= DBPART_VALUE;
		pDbBinding[i].dwMemOwner= DBMEMOWNER_CLIENTOWNED;
		pDbBinding[i].dwFlags	= 0;
		pDbBinding[i].bScale	= 0;
		pDbBinding[i].iOrdinal	= i+1;
		pDbBinding[i].eParamIO	= DBPARAMIO_INPUT;
		pDbBinding[i].bPrecision= 11;
	} 

	return pDbBinding;
}

void CTHOleDBSource::THReleaseParamInfo( DBBINDING* pBinding )
{
	if( !pBinding )
	{
		Log( _T("[OLEDB Fatal Error] pBinding is NULL Entered at THReleaseParamInfo Method!") );
		return;
	}

	m_pParamPool->Free( pBinding );
}

BOOL CTHOleDBSource::THReleaseRecordset( DBRECEIVEDATA* pResultData )
{
	if( !pResultData )
		return FALSE;

	if( pResultData->Query.select.pBindings )
		m_pBindingPool->Free( pResultData->Query.select.pBindings );

	if( pResultData->Query.select.pResult )
		m_pBindingPool->Free( pResultData->Query.select.pResult );

	return TRUE;
}

BOOL CTHOleDBSource::THChangeDB( char* szDbName, BYTE bReturnResult, DWORD dwQueryUID )
{
	LPDBCMDMSG pMsg = (LPDBCMDMSG)m_pInputMsgPool->Alloc();
	if( !pMsg )	
	{
		Log( _T("[OLEDB Fatal Error] InputMessagePool Overflow!(CTHOleDBSource::THChangeDB)") );
		return FALSE;
	}

	KSCToUnicode( szDbName, pMsg->wszQuery );
	pMsg->bQueryType = QUERY_TYPE_CHANGE_DB;
	pMsg->dwThreadID = ::GetCurrentThreadId();
	pMsg->AddInfo.change_db.bReturnResult = bReturnResult;
	pMsg->dwQueryUID = dwQueryUID;

	PostDBMessage( pMsg );

	return TRUE;
}

BOOL CTHOleDBSource::PostDBMessage( LPDBCMDMSG pMsg )
{
	if( !pMsg )	
	{
		Log( _T("[OLEDB Fatal Error] LPDBCMDMSG is NULL!(CTHOleDBSource::PostDBMessage)") );
		return FALSE;
	}

	if( m_bCloseQueryForThread )	// Input 막음 
		return FALSE;

	DWORD dwQCount = m_pInputQueue->GetCount( TRUE );
	if( m_InitDesc.wMaxNumOfProcessMessage_Input <= dwQCount )
	{
		Log( _T("[OLEDB Fatal Error] DBInput Message Queue Overflow!(CTHOleDBSource::PostDBMessage)") );
		return FALSE;
	}

	m_pInputQueue->AddTail( pMsg );

	InformDBInput();

	return TRUE;
}

void CTHOleDBSource::InformDBInput()
{
	QueueUserAPC( InformInputQueryThreadAPC, m_hDBThread, (ULONG_PTR)NULL );		
}

void CTHOleDBSource::ProcessSelectQuery( LPDBCMDMSG pMsg )
{
	COleDBSession* pOleDBSession = GetOleDBSession( pMsg->dwThreadID );
	DBRECEIVEDATA* pResult = pOleDBSession->OpenRecordExForThread( pMsg->wszQuery, pMsg->AddInfo.select.dwMaxNumRows, pMsg->AddInfo.select.dwRowPerRead );				

	if( !pResult )	// Error
	{
		pResult = (DBRECEIVEDATA*)m_pOutputMsgPool->Alloc();
		if(pResult == NULL)
		{
			Log( _T("CTHOleDBSource Error! - Output MessagePool is Full!(OLEDBThread() case: QUERY_TYPE_SELECT)") );
			return;
		}

		pResult->nError = -1;

		// Log Error Message
		WCHAR wszStr[ 2048 ] = {0,};
		char  szStr[ 2048 ] = {0,};
		swprintf( wszStr, L"[OLEDB Query Error] '%s'( CTHOleDBSource(VOID) )", pMsg->wszQuery );
		UnicodeToKSC( wszStr, szStr, 2048 );
#ifdef _UNICODE
		Log( wszStr );
#else
		Log( szStr );
#endif
	}

	pResult->dwQueryUID = pMsg->dwQueryUID;
	pResult->pData		= pMsg->pData;
	pResult->bQueryType = pMsg->bQueryType;		

	InformDBOutput( pResult );
}

void CTHOleDBSource::ProcessExecuteQuery( LPDBCMDMSG pMsg )
{
	COleDBSession* pOleDBSession = GetOleDBSession( pMsg->dwThreadID );
	int nRet = pOleDBSession->ExecuteSQL( (char*)Convert2Multi(pMsg->wszQuery) );

	if( nRet < -1000 || pMsg->AddInfo.execute.bReturnResult )	// Error 발생시 또는 몇행이 영향을 받았는지 받기를 원할때 
	{
		DBRECEIVEDATA* pResult = (DBRECEIVEDATA*)m_pOutputMsgPool->Alloc();	
		if( !pResult )
		{									
			Log( _T("CTHOleDBSource Error! - Output MessagePool is Full!(OLEDBThread() case: QUERY_TYPE_EXECUTE)") );
			return;
		}

		if( nRet < -1000 )
		{
			pResult->nError = nRet;
			
			// Log Error Message
			WCHAR wszStr[ 2048 ] = {0,};
			char  szStr[ 2048 ] = {0,};
			swprintf( wszStr, L"[OLEDB Query Error] '%s'(code:%d)( CTHOleDBSource(VOID) )", pMsg->wszQuery, nRet );
			UnicodeToKSC( wszStr, szStr, 2048 );	
#ifdef _UNICODE
			Log( wszStr );
#else
			Log( szStr );
#endif
		}
		else
		{
			pResult->Query.execute.dwEffected = nRet;
		}

		pResult->dwQueryUID = pMsg->dwQueryUID;
		pResult->pData		= pMsg->pData;
		pResult->bQueryType = pMsg->bQueryType;	

		InformDBOutput( pResult );
	}
}

void CTHOleDBSource::ProcessExecuteWithParamQuery( LPDBCMDMSG pMsg )
{
	DBMSG_EXECUTESQL_BY_PARAM* pAdd = &pMsg->AddInfo.execute_by_param;

	COleDBSession* pOleDBSession = GetOleDBSession( pMsg->dwThreadID );
    int nRet = pOleDBSession->ExecuteSQLByParam( (char*)Convert2Multi(pMsg->wszQuery), pAdd->pBinding, pAdd->szParamValueBuf, pAdd->bParamNum );

	if( nRet < -1000 || pAdd->bReturnResult )	// Error 발생시 또는 몇행이 영향을 받았는지 받기를 원할때 
	{
		DBRECEIVEDATA* pResult = (DBRECEIVEDATA*)m_pOutputMsgPool->Alloc();
		if( !pResult )
		{									
			Log( _T("OLEDB Fatal Error! - Output MessagePool is Full!(CTHOleDBSource() case: QUERY_TYPE_EXECUTE_BY_PARAM)") );
			return;
		}

		if( nRet <-1000 )
		{
			pResult->nError = nRet;

			// Log Error Message
			WCHAR wszStr[ 2048 ] = {0,};
			char  szStr[ 2048 ] = {0,};
			swprintf( wszStr, L"[OLEDB Query Error] QUERY_TYPE_EXECUTE_BY_PARAM (QueryUID:%d, Query:%s)( CTHOleDBSource(VOID) )", pMsg->dwQueryUID, pMsg->wszQuery );
			UnicodeToKSC( wszStr, szStr, 2048 );			
#ifdef _UNICODE
			Log( wszStr );
#else
			Log( szStr );
#endif
		}
		else
		{
			pResult->Query.execute_by_param.dwEffected = nRet;
		}

		pResult->dwQueryUID = pMsg->dwQueryUID;
		pResult->pData		= pMsg->pData;
		pResult->bQueryType = pMsg->bQueryType;	

		InformDBOutput( pResult );
	}

	THReleaseParamInfo( pAdd->pBinding );
}

void CTHOleDBSource::ProcessChangeDBQuery( LPDBCMDMSG pMsg )
{
	COleDBSession* pOleDBSession = GetOleDBSession( pMsg->dwThreadID );
	BOOL bRet = pOleDBSession->ChangeDB( (char*)Convert2Multi(pMsg->wszQuery) );

	if( bRet == FALSE || pMsg->AddInfo.change_db.bReturnResult )
	{
		DBRECEIVEDATA* pResult = (DBRECEIVEDATA*)m_pOutputMsgPool->Alloc();
		if( !pResult )
		{									
			Log( _T("OLEDB Fatal Error! - Output MessagePool is Full!(OLEDBThread() case: QUERY_TYPE_CHANGE_DB)") );
			return;
		}

		if( bRet == FALSE )
		{
			pResult->nError = -1;

			// Log Error Message
			WCHAR wszStr[ 2048 ] = {0,};
			char  szStr[ 2048 ] = {0,};
			swprintf( wszStr, L"[OLEDB Query Error] '%s'( OLEDBThread(VOID) )", pMsg->wszQuery );
			UnicodeToKSC(wszStr, szStr, 2048);
#ifdef _UNICODE
			Log( wszStr );
#else
			Log( szStr );
#endif
		}
		else
		{
			pResult->nError = 0;
		}

		pResult->dwQueryUID = pMsg->dwQueryUID;
		pResult->bQueryType = pMsg->bQueryType;	

		InformDBOutput(pResult);
	}
}

void CTHOleDBSource::InformDBOutput( DBRECEIVEDATA* pResult )
{
	if( m_InitDesc.bUsingEventObject )
	{		
		// OutputQ에 Push							
		m_pOutputQueue->AddTail( pResult );	

		// 결과가 만들어졌음을 통보!!
		::SetEvent( m_hResultEvent ); 
	}
	else
	{
		::PostMessage( m_InitDesc.hWndToPostMessage, m_InitDesc.uMessage, (WPARAM)pResult, 0 );
	}
}

void CTHOleDBSource::GetDBResult()
{
	SwitchOutputQueue();

	POSITION_ pos = NULL;
	DBRECEIVEDATA* pResult = NULL;

	pos = m_pOutputQueue->GetHeadPosition();
	while( pos )
	{	
		pResult = (DBRECEIVEDATA*)m_pOutputQueue->GetNext(pos);

		if( pResult ) 
		{
			m_InitDesc.ReceiveFunc( pResult );	// 결과물을 외부함수로 반환 

			switch( pResult->bQueryType )
			{
				case QUERY_TYPE_SELECT:	THReleaseRecordset(	pResult );	break;	// 메모리 풀에서 결과값 Release
			}

			m_pOutputMsgPool->Free( pResult );
		}			
	}

	m_pOutputQueue->RemoveAll();

	SwitchOutputQueue();

	if ( m_pOutputQueue->IsEmpty(FALSE) == FALSE )
	{
		SwitchOutputQueue();
		::SetEvent( m_hResultEvent );
	}
}

END_NAMESPACE