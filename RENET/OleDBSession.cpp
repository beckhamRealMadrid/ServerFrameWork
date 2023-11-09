#include "stdafx.h"
#include "OleDBSession.h"
#include "OleDBSource.h"
#include "ThreadSafeStaticMemPool.h"
#include "Service.h"

START_NAMESPACE

const GUID CLSID_SQLOLEDB = {0xc7ff16cL,0x38e3,0x11d0,{0x97,0xab,0x0,0xc0,0x4f,0xc2,0xad,0x98}};

COleDBSession::COleDBSession( DB_INITIALIZE_DESC* desc, COleDBSource* pParent )
{
	m_pIDBInitialize	= NULL;
	m_pIDBProperties	= NULL;
	m_pIDBCreateSession	= NULL;
	m_pIDBCreateCommand	= NULL;
	m_pICommandText		= NULL;
	m_pIAccessor		= NULL;

	m_dwCommandTimeOut	= 0;
	m_bCommandTimeOut	= FALSE;
	m_byTimeOut			= 0;

	m_hExcuteEvent		= INVALID_HANDLE_VALUE;
	m_hDBExcuteThread	= INVALID_HANDLE_VALUE;

	m_pHrowPool			= NULL;
	m_pOledbSource		= pParent;

	if ( m_pOledbSource->GetDesc().bUsingThread )
	{
		m_pHrowPool = new CThreadSafeStaticMemPool( sizeof(HROW) * m_pOledbSource->GetDesc().wMaxRowPerRead, 1, 2 );
		if( !m_pHrowPool )
		{
			OutputDebugStringW( L"InitializeStaticMemoryPool failed for HROW pool" );
			__asm int 3
		}
	}
}

COleDBSession::~COleDBSession()
{
	SAFE_RELEASE(m_pICommandText);
	SAFE_RELEASE(m_pIAccessor);
	SAFE_RELEASE(m_pIDBCreateCommand);
	SAFE_RELEASE(m_pIDBProperties);
	SAFE_RELEASE(m_pIDBCreateSession);

	if( m_pIDBInitialize )
	{
		HRESULT hr = m_pIDBInitialize->Uninitialize();
		if( FAILED(hr) )
		{
			ErrorDisplay(hr);
			MessageBoxW( NULL, L"Uninitialize failed.", L"Warning!", MB_OK );
		}

		m_pIDBInitialize->Release();
	}

	if( m_pHrowPool )			
		delete m_pHrowPool;

	::SetEvent( GetExcuteEndEvent() );

	if( m_bCommandTimeOut )
	{
		::WaitForSingleObject( m_hDBExcuteThread, INFINITE );
		SAFE_CLOSEHANDLE( m_hDBExcuteThread );		
	}
}

int COleDBSession::Create()
{
	WCHAR wszDataSource[256] = {0,};
	WCHAR wszDefaultDb[256] = {0,};
	WCHAR wszUserId[256] = {0,};
	WCHAR wszPassword[256] = {0,};	

	KSCToUnicode( m_pOledbSource->GetIP(), wszDataSource );			
	KSCToUnicode( m_pOledbSource->GetCatalog(), wszDefaultDb );			
	KSCToUnicode( m_pOledbSource->GetUserID(), wszUserId );			
	KSCToUnicode( m_pOledbSource->GetPasswd(), wszPassword );			

	// COM Library 초기화...
	int			nRet = 1;
	HRESULT		hr = NULL;	
	DBPROP		InitProperties[6];
	DBPROPSET	rgInitPropSet[1];

	// SQLOLEDB Provider 
	hr = ::CoCreateInstance( CLSID_SQLOLEDB, NULL, CLSCTX_INPROC_SERVER, IID_IDBInitialize, (void**) &m_pIDBInitialize );
	if( FAILED(hr) )    
		return -1000;

	__try
	{
		for( int i = 0; i < 6; i++ ) 
            VariantInit(&InitProperties[i].vValue);

		// Server name.
		InitProperties[0].dwPropertyID		= DBPROP_INIT_DATASOURCE;
		InitProperties[0].vValue.vt			= VT_BSTR;
		InitProperties[0].vValue.bstrVal	= SysAllocString(wszDataSource);
		InitProperties[0].dwOptions			= DBPROPOPTIONS_REQUIRED;
		InitProperties[0].colid				= DB_NULLID;

		// Database.
		InitProperties[1].dwPropertyID		= DBPROP_INIT_CATALOG;
		InitProperties[1].vValue.vt			= VT_BSTR;
		InitProperties[1].vValue.bstrVal	= SysAllocString(wszDefaultDb);
		InitProperties[1].dwOptions			= DBPROPOPTIONS_REQUIRED;
		InitProperties[1].colid				= DB_NULLID;

		// Username (login).
		InitProperties[2].dwPropertyID		= DBPROP_AUTH_USERID; 
		InitProperties[2].vValue.vt			= VT_BSTR;
		InitProperties[2].vValue.bstrVal	= SysAllocString(wszUserId);
		InitProperties[2].dwOptions			= DBPROPOPTIONS_REQUIRED;
		InitProperties[2].colid				= DB_NULLID;

		// Password.
		InitProperties[3].dwPropertyID		= DBPROP_AUTH_PASSWORD;
		InitProperties[3].vValue.vt			= VT_BSTR;
		InitProperties[3].vValue.bstrVal	= SysAllocString(wszPassword);
		InitProperties[3].dwOptions			= DBPROPOPTIONS_REQUIRED;
		InitProperties[3].colid				= DB_NULLID;

		rgInitPropSet[0].guidPropertySet	= DBPROPSET_DBINIT;
		rgInitPropSet[0].cProperties		= 4;
		rgInitPropSet[0].rgProperties		= InitProperties;

		hr = m_pIDBInitialize->QueryInterface(IID_IDBProperties, (void **)&m_pIDBProperties);
		FAIL_CHECK(nRet, hr, -1100);

		hr = m_pIDBProperties->SetProperties(1, rgInitPropSet); 
		FAIL_CHECK(nRet, hr, -1200);

		// 초기화..
		hr = m_pIDBInitialize->Initialize();
		FAIL_CHECK(nRet, hr, -1300);

		// Create a session object.
		hr = m_pIDBInitialize->QueryInterface( IID_IDBCreateSession, (void**) &m_pIDBCreateSession);
		FAIL_CHECK(nRet, hr, -1400);

		// 세션을 만들고...  요청한 인터페이스 포인터를 받는다.
		// Creates a new session from the data source object and returns the requested interface on the newly created session.
		hr = m_pIDBCreateSession->CreateSession(NULL, IID_IDBCreateCommand, (IUnknown**) &m_pIDBCreateCommand);
		FAIL_CHECK(nRet, hr, -1500);

		// Access the ICommandText interface.
		// Creates a new command.
		hr = m_pIDBCreateCommand->CreateCommand(NULL, IID_ICommandText, (IUnknown**) &m_pICommandText);
		FAIL_CHECK(nRet, hr, -1600);
	}
	__finally
	{
		if(nRet != 1)
			ErrorDisplay(hr);

		SysFreeString( InitProperties[0].vValue.bstrVal );
		SysFreeString( InitProperties[1].vValue.bstrVal );
		SysFreeString( InitProperties[2].vValue.bstrVal );
		SysFreeString( InitProperties[3].vValue.bstrVal );		
	}

	return nRet;
}

int COleDBSession::Create( int nConnectTimeout, int nCommandTimeOut, BOOL bCommandTimeOut )
{
	WCHAR wszDataSource[256] = {0,};
	WCHAR wszDefaultDb[256] = {0,};
	WCHAR wszUserId[256] = {0,};
	WCHAR wszPassword[256] = {0,};	

	KSCToUnicode( m_pOledbSource->GetIP(), wszDataSource );			
	KSCToUnicode( m_pOledbSource->GetCatalog(), wszDefaultDb );			
	KSCToUnicode( m_pOledbSource->GetUserID(), wszUserId );			
	KSCToUnicode( m_pOledbSource->GetPasswd(), wszPassword );

	// COM Library 초기화...
	int			nRet = 1;
	HRESULT		hr = NULL;	
	DBPROP		InitProperties[6];
	DBPROPSET	rgInitPropSet[1];
	int			nInitProp	= 6;

    // SQLOLEDB Provider 
    hr = ::CoCreateInstance(CLSID_SQLOLEDB, NULL, CLSCTX_INPROC_SERVER, IID_IDBInitialize, (void **) &m_pIDBInitialize);
 	if(FAILED(hr))    
        return -1000;

	__try
	{
		for( int i = 0; i < 6; i++ ) 
			VariantInit( &InitProperties[i].vValue );
	
		// Server name.
		InitProperties[0].dwPropertyID  = DBPROP_INIT_DATASOURCE;
		InitProperties[0].vValue.vt     = VT_BSTR;
		InitProperties[0].vValue.bstrVal= SysAllocString(wszDataSource);
		InitProperties[0].dwOptions     = DBPROPOPTIONS_REQUIRED;
		InitProperties[0].colid         = DB_NULLID;

		// Database.
		InitProperties[1].dwPropertyID  = DBPROP_INIT_CATALOG;
		InitProperties[1].vValue.vt     = VT_BSTR;
		InitProperties[1].vValue.bstrVal= SysAllocString(wszDefaultDb);
		InitProperties[1].dwOptions     = DBPROPOPTIONS_REQUIRED;
		InitProperties[1].colid         = DB_NULLID;

		// Username (login).
		InitProperties[2].dwPropertyID  = DBPROP_AUTH_USERID; 
		InitProperties[2].vValue.vt     = VT_BSTR;
		InitProperties[2].vValue.bstrVal= SysAllocString(wszUserId);
		InitProperties[2].dwOptions     = DBPROPOPTIONS_REQUIRED;
		InitProperties[2].colid         = DB_NULLID;
		
		// Password.
		InitProperties[3].dwPropertyID  = DBPROP_AUTH_PASSWORD;
		InitProperties[3].vValue.vt     = VT_BSTR;
		InitProperties[3].vValue.bstrVal= SysAllocString(wszPassword);
		InitProperties[3].dwOptions     = DBPROPOPTIONS_REQUIRED;
		InitProperties[3].colid         = DB_NULLID;

		// Connection Timeout.
		InitProperties[4].dwPropertyID  = DBPROP_INIT_TIMEOUT;
		InitProperties[4].vValue.vt     = VT_I4;
		InitProperties[4].vValue.intVal = nConnectTimeout;
		InitProperties[4].dwOptions     = DBPROPOPTIONS_REQUIRED;
		InitProperties[4].colid         = DB_NULLID;

		// Command Timeout.
		if( bCommandTimeOut )
		{
			InitProperties[5].dwPropertyID  = DBPROP_COMMANDTIMEOUT;
			InitProperties[5].vValue.vt     = VT_I4;
			InitProperties[5].vValue.intVal = nCommandTimeOut;
			InitProperties[5].dwOptions     = DBPROPOPTIONS_REQUIRED;
			InitProperties[5].colid         = DB_NULLID;
		}
		else
		{
			nInitProp = 5;
		}
  
		rgInitPropSet[0].guidPropertySet	= DBPROPSET_DBINIT;
		rgInitPropSet[0].cProperties		= nInitProp;
		rgInitPropSet[0].rgProperties		= InitProperties;
  
		m_dwCommandTimeOut	= nCommandTimeOut * 1000;
		m_bCommandTimeOut	= bCommandTimeOut;	
		m_byTimeOut			= DEF_EXCUTE_START;
		m_hExcuteEvent		= ::CreateEvent( NULL, FALSE, FALSE, NULL );

		if( m_bCommandTimeOut )
		{
			unsigned dwID = 0;
			m_hDBExcuteThread = BEGINTHREADEX( NULL, 0, WorkerThread, this, 0, &dwID );			
		}

		hr = m_pIDBInitialize->QueryInterface(IID_IDBProperties, (void **)&m_pIDBProperties);
		FAIL_CHECK(nRet, hr, -1100);

		hr = m_pIDBProperties->SetProperties(1, rgInitPropSet); 
		FAIL_CHECK(nRet, hr, -1200);

		// 초기화..
		hr = m_pIDBInitialize->Initialize();
		FAIL_CHECK(nRet, hr, -1300);
		
		// Create a session object.
		hr = m_pIDBInitialize->QueryInterface( IID_IDBCreateSession, (void**) &m_pIDBCreateSession);
		FAIL_CHECK(nRet, hr, -1400);

		// 세션을 만들고...  요청한 인터페이스 포인터를 받는다.
		// Creates a new session from the data source object and returns the requested interface on the newly created session.
		hr = m_pIDBCreateSession->CreateSession(NULL, IID_IDBCreateCommand, (IUnknown**) &m_pIDBCreateCommand);
		FAIL_CHECK(nRet, hr, -1500);		

		// Access the ICommandText interface.
		// Creates a new command.
		hr = m_pIDBCreateCommand->CreateCommand(NULL, IID_ICommandText, (IUnknown**) &m_pICommandText);
		FAIL_CHECK(nRet, hr, -1600);				
	}
	__finally
	{
		if(nRet != 1)
			ErrorDisplay(hr);

		SysFreeString(InitProperties[0].vValue.bstrVal);
		SysFreeString(InitProperties[1].vValue.bstrVal);
		SysFreeString(InitProperties[2].vValue.bstrVal);
		SysFreeString(InitProperties[3].vValue.bstrVal);		
	}

	return nRet;
}

// 반환값이있는 SQL 쿼리문(주로 Select 구문)이나 저장프로시져를 실행하고 파라미터에 지정한 주소로 결과를 반환
int	COleDBSession::OpenRecord( char* szQuerySQL, void* pRecordSet, DWORD dwMaxNumRows )
{
	DWORD				 dwRowPerRead			= DEFAULT_ROWS_PER_READ;	
	HRESULT				 hr						= NULL;
	IRowset*			 pIRowset				= NULL;
	IColumnsInfo*        pIColumnsInfo			= NULL;
	DBCOLUMNINFO*        pDBColumnInfo			= NULL;
	ULONG                ConsumerBufColOffset	= 0;
	ULONG                lNumCols				= 0;			// 몇개 칼럼이 있는지 받아올 변수 
	WCHAR*               pStringsBuffer			= NULL;
	HACCESSOR            hAccessor				= 0;
	ULONG                lNumRowsRetrieved		= 0;			// 몇개의 결과가 나왔는지 저장할 변수 
	HROW                 hRows[10]				= {NULL,};
	HROW*                pRows					= &hRows[0];
	DBBINDING*           pBindings				= NULL;	

	WCHAR wszQuerySQL[ MAX_SQL_STRING_LENGTH + 1 ] = {0,};
	KSCToUnicode( szQuerySQL, wszQuerySQL );

	hr = m_pICommandText->SetCommandText( DBGUID_DBSQL, wszQuerySQL );
	FAIL_RETURN(hr, -3000);
 
	if( !SetCommandExecute( wszQuerySQL, DEF_IID_IROWSET, NULL, &pIRowset ) )
		return -3100;	

	// Obtain access to the IColumnInfo interface, from the Rowset  object.
    hr = pIRowset->QueryInterface( IID_IColumnsInfo, (void **)&pIColumnsInfo );
	FAIL_RETURN(hr, -3200);

    // 필드(컬럼)정보를 얻어온다.
    pIColumnsInfo->GetColumnInfo( &lNumCols, &pDBColumnInfo, &pStringsBuffer );	

    // 컬럼 정보를 얻어오는데 사용한 인터페이스를 Release한다.
    pIColumnsInfo->Release();

	// DBBINDING 배열을 생성한다
    pBindings = new DBBINDING[lNumCols];		
		
    // ColumnInfo 구조체를 이용하여 DBBINDING 배열에 값을 채운다. 
    for( ULONG j = 0; j < lNumCols; j++ )
	{
        pBindings[j].iOrdinal	= j+1;
        pBindings[j].obValue	= ConsumerBufColOffset;		//Buffer의 옵셋 
        pBindings[j].pTypeInfo	= NULL;		 				
        pBindings[j].pObject	= NULL;
        pBindings[j].pBindExt	= NULL;
        pBindings[j].dwPart		= DBPART_VALUE;
        pBindings[j].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
        pBindings[j].eParamIO	= DBPARAMIO_NOTPARAM;
        pBindings[j].wType		= pDBColumnInfo[j].wType;				//할당받은 메모리에 ColumnType 저장.. 
		pBindings[j].dwFlags	= 0;
        pBindings[j].bPrecision = pDBColumnInfo[j].bPrecision;		//정확도..
        pBindings[j].bScale		= pDBColumnInfo[j].bScale;
        
		if(pBindings[j].wType == DBTYPE_WSTR)	//Unicode String이면 
		{
			pBindings[j].cbMaxLen = pDBColumnInfo[j].ulColumnSize * 2;	
			ConsumerBufColOffset = ConsumerBufColOffset + (pDBColumnInfo[j].ulColumnSize * 2);
		}
		else
		{
			pBindings[j].cbMaxLen = pDBColumnInfo[j].ulColumnSize;	//할당받은 메모리에 ColumnSize 저장.. 
			
			// 다음 버퍼 설정.
			ConsumerBufColOffset = ConsumerBufColOffset + pDBColumnInfo[j].ulColumnSize;
		}
    }

    // IAccessor 인터페이스 포인터를 얻는다. .
    hr = pIRowset->QueryInterface(IID_IAccessor, (void **) &m_pIAccessor);
    if(FAILED(hr))
    {
		m_pIAccessor = NULL;
   		delete [] pBindings;
  		pIRowset->Release();
		ErrorDisplay(hr);
        return -3300;
    }
	
	// 바잉딩으로부터 액셀레이터를 생성한다.
    m_pIAccessor->CreateAccessor( DBACCESSOR_ROWDATA, lNumCols, pBindings, 0, &hAccessor, NULL );                                               
  	
    // Allocate space for the row buffer.
	DWORD dwOffset = 0;
	DWORD dwTotalReceivedRow = 0;

	// lNumRowsRetrieved 만큼의 레코드(로우)를 얻어온다. 
	pIRowset->GetNextRows( NULL, 0, dwRowPerRead, &lNumRowsRetrieved, &pRows );

	// 화면에 레코드(로우)를 출력한다. 
    while( lNumRowsRetrieved > 0 ) 
	{
		// 각각의 레코드(로우) 별로 화면에 출력
        for( ULONG j = 0; j < lNumRowsRetrieved; j++ ) 
		{
			// 버퍼를 초기화 한다.
            // memset(pBuffer, 0, ConsumerBufColOffset);

            if( dwTotalReceivedRow >= dwMaxNumRows )	// 파라미터로 지정해준 MAX 행까지 받았으면 Release 한후 끝낸다.
			{
				pIRowset->ReleaseRows(lNumRowsRetrieved, hRows, NULL, NULL, NULL);
				goto QUERYEND;
			}
			
			// 레코드(로우)를 얻는다.
			pIRowset->GetData(hRows[j], hAccessor, (BYTE*)pRecordSet + dwOffset);

			dwOffset += ConsumerBufColOffset;
			dwTotalReceivedRow++;	        
        }

        // 레코드셋(로우셋) 인터페이스를 Release한다. 
        pIRowset->ReleaseRows( lNumRowsRetrieved, hRows, NULL, NULL, NULL );
        
		// lNumRowsRetrieved 만큼의 레코드(로우)를 얻어온다. 
		pIRowset->GetNextRows( NULL, 0, dwRowPerRead, &lNumRowsRetrieved, &pRows );
    } 

QUERYEND:
    m_pIAccessor->ReleaseAccessor(hAccessor, NULL);
	m_pIAccessor->Release();
	m_pIAccessor = NULL;
    
	// 할당된 힙 메모리 영역을 해제한다. 
	delete [] pBindings;	
	
  	pIRowset->Release();	

	return (int)dwTotalReceivedRow;
}

// 반환값이있는 SQL 쿼리문(주로 Select 구문)이나 저장프로시져를 실행하고
// 특별한 레코드셋의 생성 없이 Column의 타입이나 칼럼값 반환순서에 따라 Data를 반환
DBRECEIVEDATA* COleDBSession::OpenRecordEx( char* szQuerySQL, DWORD dwMaxNumRows, DWORD dwRowPerRead )
{
	HRESULT				 hr						= NULL;
	IRowset*			 pIRowset				= NULL;
	IColumnsInfo*        pIColumnsInfo			= NULL;
	DBCOLUMNINFO*        pDBColumnInfo			= NULL;
	ULONG                ConsumerBufColOffset	= 0;
	ULONG                lNumCols				= 0;		// 몇개 칼럼이 있는지 받아올 변수 
	ULONG                lNumRowsRetrieved		= 0;		// 몇개의 결과가 나왔는지 저장할 변수 
	WCHAR*               pStringsBuffer			= NULL;
	HACCESSOR            hAccessor				= 0;
	HROW*                pRows					= NULL;		// = &hRows[0];
	DBRECEIVEDATA*		 pRData					= NULL;	

	WCHAR wszQuerySQL[ MAX_SQL_STRING_LENGTH + 1 ] = {0,};
	KSCToUnicode( szQuerySQL, wszQuerySQL );

	// Creates a new command.
    m_pICommandText->Release();

	hr = m_pIDBCreateCommand->CreateCommand(NULL, IID_ICommandText, (IUnknown**) &m_pICommandText);
	FAIL_RETURN(hr, NULL);

	hr = m_pICommandText->SetCommandText( DBGUID_DBSQL, wszQuerySQL );
	FAIL_RETURN(hr, NULL);

	if( !SetCommandExecute( wszQuerySQL, DEF_IID_IROWSET, NULL, &pIRowset ) )
		return NULL;

	// Obtain access to the IColumnInfo interface, from the Rowset  object.
    hr = pIRowset->QueryInterface( IID_IColumnsInfo, (void **)&pIColumnsInfo );
	FAIL_RETURN(hr, NULL);
    
    // Retrieve the column information.
    pIColumnsInfo->GetColumnInfo( &lNumCols, &pDBColumnInfo, &pStringsBuffer );

    // Free the column information interface.
    pIColumnsInfo->Release();

    pRData = new DBRECEIVEDATA;
	if(!pRData)
	{
		OutputDebugStringW(L"Faile to allocate DBRECEIVEDATA by new\n");
		return NULL;
	}
	
	pRData->Query.select.bColCount = (BYTE)lNumCols;
	
	// Create a DBBINDING array.
    pRData->Query.select.pBindings = new DBBINDING[lNumCols];
	
    // Using the ColumnInfo structure, fill out the pBindings array.
    for( ULONG j = 0; j < lNumCols; j++ )
	{
        pRData->Query.select.pBindings[j].iOrdinal		= j+1;
        pRData->Query.select.pBindings[j].obValue		= ConsumerBufColOffset;				//Buffer의 옵셋 
        pRData->Query.select.pBindings[j].pTypeInfo		= NULL;		 				
        pRData->Query.select.pBindings[j].pObject		= NULL;
        pRData->Query.select.pBindings[j].pBindExt		= NULL;
        pRData->Query.select.pBindings[j].dwPart		= DBPART_VALUE;
        pRData->Query.select.pBindings[j].dwMemOwner	= DBMEMOWNER_CLIENTOWNED;
        pRData->Query.select.pBindings[j].eParamIO		= DBPARAMIO_NOTPARAM;
        pRData->Query.select.pBindings[j].cbMaxLen		= pDBColumnInfo[j].ulColumnSize;	// 할당받은 메모리에 ColumnSize 저장.. 
        pRData->Query.select.pBindings[j].dwFlags		= 0;
        pRData->Query.select.pBindings[j].wType			= pDBColumnInfo[j].wType;			// 할당받은 메모리에 ColumnType 저장.. 
        pRData->Query.select.pBindings[j].bPrecision	= pDBColumnInfo[j].bPrecision;		// 정확도..
        pRData->Query.select.pBindings[j].bScale		= pDBColumnInfo[j].bScale;
        
		if( pRData->Query.select.pBindings[j].wType == DBTYPE_WSTR )	// Unicode String이면 
		{
			pRData->Query.select.pBindings[j].cbMaxLen = pDBColumnInfo[j].ulColumnSize * 2;	
			ConsumerBufColOffset = ConsumerBufColOffset + (pDBColumnInfo[j].ulColumnSize * 2);
		}
		else
		{
			pRData->Query.select.pBindings[j].cbMaxLen = pDBColumnInfo[j].ulColumnSize;		// 할당받은 메모리에 ColumnSize 저장.. 
			
			// Compute the next buffer offset.
			ConsumerBufColOffset = ConsumerBufColOffset + pDBColumnInfo[j].ulColumnSize;
		}
    }

	pRData->Query.select.dwRowSize = ConsumerBufColOffset;

    // Get the IAccessor interface.
    hr = pIRowset->QueryInterface( IID_IAccessor, (void **) &m_pIAccessor );
    FAIL_RETURN(hr, pRData);
	
	// Create an accessor from the set of bindings (pBindings).
    m_pIAccessor->CreateAccessor( DBACCESSOR_ROWDATA, lNumCols, pRData->Query.select.pBindings, 0, &hAccessor, NULL );
                                               
  	pRows = new HROW[dwRowPerRead];
	assert(pRows != NULL);

	// Get a set of m_NumPerReceive만큼의 rows.
	// lNumRowsRetrieved : 몇개의 결과가 나왔는지..
	// pRows : 받은 값의 포인터.
    pIRowset->GetNextRows( NULL, 0, dwRowPerRead, &lNumRowsRetrieved, &pRows );

    //Allocate space for the row buffer.
	DWORD dwOffset				= 0;
	DWORD dwTotalReceivedRow	= 0;
			
	char* pBuffer = new char[ConsumerBufColOffset * dwMaxNumRows];
	assert(pBuffer != NULL);

	memset(pBuffer, 0, ConsumerBufColOffset * dwMaxNumRows);
	
    // Display the rows.
    while( lNumRowsRetrieved > 0 ) 
	{
        // For each row, print the column data.
        for( ULONG j = 0; j < lNumRowsRetrieved; j++ ) 
		{
			if( dwTotalReceivedRow >= dwMaxNumRows )	// 파라미터로 지정해준 MAX 행까지 받았으면 Release 한후 끝낸다.
			{
				pIRowset->ReleaseRows( lNumRowsRetrieved, pRows, NULL, NULL, NULL );
				goto QUERYEND;
			}
    
            // Get the row data values.
            pIRowset->GetData(pRows[j], hAccessor, pBuffer+dwOffset);
			dwOffset += ConsumerBufColOffset;
			dwTotalReceivedRow++;	        
        } 

        // Release the rows retrieved.
        pIRowset->ReleaseRows( lNumRowsRetrieved, pRows, NULL, NULL, NULL );
        
		// Get the next set of m_NumPerReceive rows.
        pIRowset->GetNextRows( NULL, 0, dwRowPerRead, &lNumRowsRetrieved, &pRows );
    }

QUERYEND:

	char* pResult = new char[ConsumerBufColOffset * dwTotalReceivedRow];
	assert(pResult != NULL);
	memcpy(pResult,pBuffer,ConsumerBufColOffset * dwTotalReceivedRow);

	pRData->Query.select.dwRowCount = dwTotalReceivedRow;
	pRData->Query.select.pResult = pResult;
		
    // Free up all allocated memory.
    delete [] pRows;
	delete [] pBuffer;
	
    m_pIAccessor->ReleaseAccessor( hAccessor, NULL );
	m_pIAccessor->Release();
	m_pIAccessor = NULL;

	pIRowset->Release();

	return pRData;
}

DBRECEIVEDATA* COleDBSession::OpenRecordExForThread( wchar_t* szQuerySQL, DWORD dwMaxNumRows, DWORD dwRowPerRead )
{
	HRESULT				 hr						= NULL;
	IRowset*			 pIRowset				= NULL;
	IColumnsInfo*        pIColumnsInfo			= NULL;
	DBCOLUMNINFO*        pDBColumnInfo			= NULL;
	ULONG                ConsumerBufColOffset	= 0;
	ULONG                lNumCols				= 0;		//몇개 칼럼이 있는지 받아올 변수 
	WCHAR*               pStringsBuffer			= NULL;
	HACCESSOR            hAccessor				= 0;
	ULONG                lNumRowsRetrieved		= 0;		//몇개의 결과가 나왔는지 저장할 변수 
	HROW*                pRows					= NULL;		// = &hRows[0];
	DBRECEIVEDATA*		 pRData					= NULL;
	
	hr = m_pICommandText->SetCommandText(DBGUID_DBSQL, szQuerySQL);
	FAIL_RETURN(hr, NULL);

	if( !SetCommandExecute(szQuerySQL, DEF_IID_IROWSET, NULL, &pIRowset) )
		return NULL;
		
	if( pIRowset == NULL )
	{
		Log( _T("failed to acquire rowset!") );
		return NULL;
	}

	// Obtain access to the IColumnInfo interface, from the Rowset  object.
    hr = pIRowset->QueryInterface(IID_IColumnsInfo, (void **)&pIColumnsInfo);
	FAIL_RETURN(hr, NULL);

    // Retrieve the column information.
    pIColumnsInfo->GetColumnInfo(&lNumCols, &pDBColumnInfo, &pStringsBuffer);
    pIColumnsInfo->Release();

	pRData = (DBRECEIVEDATA*)m_pOledbSource->GetOutputMsgPool()->Alloc();
	if(!pRData)	
	{
		Log( _T("Failed to allocate OutputMessageData") );
		return NULL;
	}

	pRData->Query.select.bColCount = (BYTE)lNumCols;
	
	assert( m_pOledbSource != NULL );

	pRData->Query.select.pBindings = (DBBINDING*)m_pOledbSource->GetBindingPool()->Alloc();
	char* pBuffer				   = (char*)m_pOledbSource->GetResultPool()->Alloc();
	
	if(!pRData->Query.select.pBindings)	
	{
		Log( _T( "Binding Memory Pool is Full!") );
		return NULL;
	}

	if( !pBuffer )	
	{
		Log( _T("Result Memory Pool is Full!") );
		return NULL;
	}
	
    // Using the ColumnInfo structure, fill out the pBindings array.
    for( ULONG j=0; j < lNumCols; j++ )
	{
        pRData->Query.select.pBindings[j].iOrdinal		= j+1;
        pRData->Query.select.pBindings[j].obValue		= ConsumerBufColOffset;				// Buffer의 옵셋.
        pRData->Query.select.pBindings[j].pTypeInfo		= NULL;		 				
        pRData->Query.select.pBindings[j].pObject		= NULL;
        pRData->Query.select.pBindings[j].pBindExt		= NULL;
        pRData->Query.select.pBindings[j].dwPart		= DBPART_VALUE;
        pRData->Query.select.pBindings[j].dwMemOwner	= DBMEMOWNER_CLIENTOWNED;
        pRData->Query.select.pBindings[j].eParamIO		= DBPARAMIO_NOTPARAM;
        pRData->Query.select.pBindings[j].cbMaxLen		= pDBColumnInfo[j].ulColumnSize;	// 할당받은 메모리에 ColumnSize 저장.. 
        pRData->Query.select.pBindings[j].dwFlags		= 0;
        pRData->Query.select.pBindings[j].wType			= pDBColumnInfo[j].wType;			// 할당받은 메모리에 ColumnType 저장.. 
        pRData->Query.select.pBindings[j].bPrecision	= pDBColumnInfo[j].bPrecision;		// 정확도..
        pRData->Query.select.pBindings[j].bScale		= pDBColumnInfo[j].bScale;
        
        // Compute the next buffer offset.
        
		if( pRData->Query.select.pBindings[j].wType == DBTYPE_WSTR )	// Unicode String이면 
		{
			pRData->Query.select.pBindings[j].cbMaxLen = pDBColumnInfo[j].ulColumnSize * 2;	
			ConsumerBufColOffset = ConsumerBufColOffset + (pDBColumnInfo[j].ulColumnSize * 2);
		}
		else
		{
			pRData->Query.select.pBindings[j].cbMaxLen = pDBColumnInfo[j].ulColumnSize;	//할당받은 메모리에 ColumnSize 저장.. 
			
			// Compute the next buffer offset.
			ConsumerBufColOffset = ConsumerBufColOffset + pDBColumnInfo[j].ulColumnSize;
		}
    }

	pRData->Query.select.dwRowSize = ConsumerBufColOffset;

    //Get the IAccessor interface.
    hr = pIRowset->QueryInterface(IID_IAccessor, (void **) &m_pIAccessor);
    if( FAILED(hr) )
    {
		ErrorDisplay(hr);

		if(pRData->Query.select.pBindings)
			m_pOledbSource->GetBindingPool()->Free(pRData->Query.select.pBindings);

		if(pBuffer)
			m_pOledbSource->GetResultPool()->Free(pBuffer);

		if(pRData)
			m_pOledbSource->GetOutputMsgPool()->Free(pRData);	
		
        return NULL;
    }
	
	// Create an accessor from the set of bindings (pBindings).
    m_pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA, lNumCols, pRData->Query.select.pBindings, 0, &hAccessor, NULL);
                          
	pRows = (HROW*)m_pHrowPool->Alloc();
	if(!pRows)
	{
		Log( _T("Failed to allocate Row from pool") );
		return NULL;
	}
	
	// Get a set of m_NumPerReceive만큼의 rows.
	// lNumRowsRetrieved : 몇개의 결과가 나왔는지..
	// pRows : 받은 값의 포인터.
    // Allocate space for the row buffer.
	DWORD dwOffset = 0;
	DWORD dwTotalReceivedRow = 0;

	pIRowset->GetNextRows(NULL, 0, dwRowPerRead, &lNumRowsRetrieved, &pRows);				

    //Display the rows.
    while( lNumRowsRetrieved > 0 ) 
	{
        //For each row, print the column data.
        for( ULONG j = 0; j < lNumRowsRetrieved; j++ ) 
		{
			if( dwTotalReceivedRow >= dwMaxNumRows )	// 파라미터로 지정해준 MAX 행까지 받았으면 Release 한후 끝낸다.
			{
				pIRowset->ReleaseRows(lNumRowsRetrieved, pRows, NULL, NULL, NULL);
				goto QUERYEND;
			}
    
            // Get the row data values.
            pIRowset->GetData(pRows[j], hAccessor, pBuffer+dwOffset);
			dwOffset += ConsumerBufColOffset;
			dwTotalReceivedRow++;	        
        }

        // Release the rows retrieved.
        pIRowset->ReleaseRows(lNumRowsRetrieved, pRows, NULL, NULL, NULL);
        
		// Get the next set of m_NumPerReceive rows.
        pIRowset->GetNextRows(NULL, 0, dwRowPerRead, &lNumRowsRetrieved, &pRows);
    }

QUERYEND:
	
	pRData->Query.select.dwRowCount = dwTotalReceivedRow;
	pRData->Query.select.pResult	= pBuffer;
	
    // Free up all allocated memory.	
	if(pRows)
		m_pHrowPool->Free(pRows);

    m_pIAccessor->ReleaseAccessor(hAccessor, NULL);
	m_pIAccessor->Release();
	m_pIAccessor = NULL;

	pIRowset->Release();

	return pRData;
}

// Parameter 바인딩 정보와 함께 DBBINDING 객체를 생성하고 속성들을 셋팅하며, 생성된 객체의 주소값을 반환
DBBINDING* COleDBSession::CreateParamInfo(WORD wParamNum)
{
	DBBINDING* pDbBinding = new DBBINDING[wParamNum];
	if( !pDbBinding )
		return NULL;

	for( int i = 0; i < (int)wParamNum; i++ )
	{
		pDbBinding[i].obLength		= 0;
		pDbBinding[i].obStatus		= 0;
		pDbBinding[i].pTypeInfo		= NULL;
		pDbBinding[i].pObject		= NULL;
		pDbBinding[i].pBindExt		= NULL;
		pDbBinding[i].dwPart		= DBPART_VALUE;
		pDbBinding[i].dwMemOwner	= DBMEMOWNER_CLIENTOWNED;
		pDbBinding[i].dwFlags		= 0;
		pDbBinding[i].bScale		= 0;
		pDbBinding[i].iOrdinal		= i+1;
		pDbBinding[i].eParamIO		= DBPARAMIO_INPUT;
		pDbBinding[i].bPrecision	= 11;
	} 

	return pDbBinding;
}

// CreateParamInfo 메서드를 통해 생성된 바인딩 정보를 해제
BOOL COleDBSession::ReleaseParamInfo( DBBINDING* pBinding )
{
	if( pBinding )
		delete [] pBinding;

	return TRUE;
}

// 반환값이 없는 SQL 쿼리문이나 반환값이 없는 저장프로시져를 호출(Insert, Update, Delete 구문등...)
int	COleDBSession::ExecuteSQL( char* szQuerySQL )
{
	HRESULT	 hr = NULL;
	LONG     cNumRows = 0;

	WCHAR wszQuerySQL[ MAX_SQL_STRING_LENGTH + 1 ]  = {0,};
	KSCToUnicode( szQuerySQL, wszQuerySQL );

	hr = m_pICommandText->SetCommandText( DBGUID_DBSQL, wszQuerySQL );
	FAIL_RETURN(hr, -2000);

	if( !SetCommandExecute( wszQuerySQL, DEF_IID_NULL, NULL, NULL, NULL, FALSE, &cNumRows ) )
		return 0;

	return cNumRows;
}

// 반환값이 없는 SQL 쿼리문이나 반환값이 없는 저장프로시져를 파라미터에 의해 호출
// (Insert, Update, Delete 구문등의 인자값을 변수로 부터 셋팅할때나 binary 데이터를 셋팅할때 주로 사용
int	COleDBSession::ExecuteSQLByParam( char* szQuerySQL, DBBINDING* pBinding, void* pParamValue, BYTE bParamNum )
{
	int			i			= 0;
	HRESULT		hr			= NULL;
	DBPARAMS    Params;
	HACCESSOR   hAccessor	= 0;
	DWORD		dwParamSize = 0;
	LONG		cNumRows	= 0;

	WCHAR wszQuerySQL[ MAX_SQL_STRING_LENGTH + 1 ] = {0,};
	KSCToUnicode( szQuerySQL, wszQuerySQL );

	//Set the command text.
	hr = m_pICommandText->SetCommandText( DBGUID_DBSQL, wszQuerySQL );
    if(FAILED(hr))
    {
       	m_pIAccessor = NULL;
		ErrorDisplay(hr);
		return -4000;
    }
/*	
	[bPrecision(정확도) 설명]
	The maximum precision to use when getting data and 
	wType is DBTYPE_NUMERIC or DBTYPE_VARNUMERIC. 
	This is ignored in three cases: when setting data; 
	when wType is not DBTYPE_NUMERIC or DBTYPE_VARNUMERIC; 
	and when the DBPART_VALUE bit is not set in dwPart. 
	For more information, see "Conversions Involving DBTYPE_NUMERIC 
	or DBTYPE_DECIMAL" in Appendix A: Data Types.
*/
    
    // Create an accessor from the above set of bindings.
    hr = m_pICommandText->QueryInterface( IID_IAccessor, (void**)&m_pIAccessor );
    if (FAILED(hr))
    {
     	m_pIAccessor = NULL;
		ErrorDisplay(hr);
		return -4100;
    }
	
	// 총 파라미터의 사이즈를 구하고..
	for( i = 0; i < bParamNum ; i++ )
		dwParamSize += pBinding[i].cbMaxLen; 

	hr = m_pIAccessor->CreateAccessor( DBACCESSOR_PARAMETERDATA, bParamNum, pBinding, dwParamSize, &hAccessor, NULL );
    if (FAILED(hr))
    {
		ErrorDisplay(hr);
		m_pIAccessor = NULL;
		return -4000;
    }
    
    // Fill in DBPARAMS structure for the command execution. This structure
    // specifies the parameter values in the command and is then passed 
    // to Execute.
    Params.pData = pParamValue;
    Params.cParamSets = 1;
    Params.hAccessor = hAccessor;
	 
	if( !SetCommandExecute( wszQuerySQL, DEF_IID_NULL, NULL, NULL, &Params, TRUE, &cNumRows ) )
	{
		m_pIAccessor->ReleaseAccessor(hAccessor, NULL);
		m_pIAccessor->Release();
		m_pIAccessor = NULL;
		return -4000;
	}  

//	ReleaseParamInfo(pBinding);	
	
    // Free up memory.
    m_pIAccessor->ReleaseAccessor(hAccessor, NULL);
    m_pIAccessor->Release();
	m_pIAccessor = NULL;
    
	return cNumRows;
}

// 반환값이있는 SQL 쿼리문(주로 Select 구문)이나 저장프로시져를 실행하고 특별한 레코드셋의 생성 없이 Column의 타입이나 칼럼값 반환순서에 따라 Data를 반환
// dwRowNum와 wColumnNum 은 zero based Index이다..
bool COleDBSession::GetData( void* pReceiveData, DBRECEIVEDATA* pResultData, DWORD dwRowNum, WORD wColumnNum )
{
	if( !pReceiveData ) 
		return false;

	memcpy( pReceiveData,
        pResultData->Query.select.pResult + (pResultData->Query.select.dwRowSize * dwRowNum) + pResultData->Query.select.pBindings[ wColumnNum ].obValue,
		pResultData->Query.select.pBindings[wColumnNum].cbMaxLen );

	return true;
}

// OpenRecordEx 메서드로 부터 생성되어 받은 DBRECEIVEDATA 객체를 해제
bool COleDBSession::ReleaseRecordset( DBRECEIVEDATA* pResultData )
{
	if( !pResultData )
		return FALSE;

	delete [] pResultData->Query.select.pBindings; 
	delete [] pResultData->Query.select.pResult;

	delete pResultData;

	return true;
}

bool COleDBSession::KSCToUnicode( char *pKsc, WCHAR *pUni )
{
	int nUniSize = 0;
	int nKscSize = strlen(pKsc);

	if( nKscSize <= 0 )
		return FALSE;

	// 1. 먼저 유니코드로 변환하고자 하는 문자열의 크기를 알아낸다.
	nUniSize = MultiByteToWideChar( CP_ACP, 0, pKsc, nKscSize+1, NULL, 0 );

	// 2. 유니코드의 크기(uni_size)와 변환하고자하는 소스문자열(pViewString)과 목적문자열(string)을 이용한다.
	MultiByteToWideChar( CP_ACP, 0, pKsc, nKscSize, pUni, nUniSize*2+100 );

	pUni[nUniSize] = L'\0';

	return true;
}

BOOL COleDBSession::UnicodeToKSC( WCHAR *pUni, char *pKsc, int nKscSize )
{
	int nMultibyteSize = 0;
	nMultibyteSize = WideCharToMultiByte( CP_ACP, 0, pUni, wcslen(pUni)+1, pKsc, nKscSize, NULL, 0 );

	pKsc[ nMultibyteSize ] = '\0';
	return TRUE;
}

void COleDBSession::ErrorDisplay( HRESULT hrErr )
{
	// A locale is a collection of language-related
	DWORD MYLOCALEID = GetUserDefaultLCID();  

	IErrorInfo*          pErrorInfo			= NULL;
	IErrorInfo*          pErrorInfoRec		= NULL;
	IErrorRecords*       pErrorRecords		= NULL;
	ISupportErrorInfo*   pSupportErrorInfo	= NULL;
	HRESULT              hr					= NULL;
	ULONG                i = 0, ulNumErrorRecs = 0;
	ERRORINFO            ErrorInfo;

	if(!FAILED(hrErr))
		return;

	// Check that the current interface supports error objects.
	hr = m_pICommandText->QueryInterface( IID_ISupportErrorInfo, (void**) &pSupportErrorInfo );
	if(SUCCEEDED(hr)) 
	{
		GetErrorInfo( 0, &pErrorInfo );
		if(!pErrorInfo) 
		{
			if(pSupportErrorInfo)
				pSupportErrorInfo->Release();

			return;
		}

		pErrorInfo->QueryInterface( IID_IErrorRecords, (void**)&pErrorRecords );
		pErrorRecords->GetRecordCount( &ulNumErrorRecs );

		// Read through error records and display them.
		for ( i = 0; i < ulNumErrorRecs; i++ ) 
		{
			// Get basic error information.
			pErrorRecords->GetBasicErrorInfo( i, &ErrorInfo );

			// Get error description and source through the
			// IErrorInfo interface pointer on a particular record.
			hr = pErrorRecords->GetErrorInfo( i, MYLOCALEID, &pErrorInfoRec );
//			if(FAILED(hr))
//				int k=0;

			BSTR bstrDescriptionOfError = NULL;
			BSTR bstrSourceOfError = NULL;

			pErrorInfoRec->GetDescription( &bstrDescriptionOfError );
			pErrorInfoRec->GetSource( &bstrSourceOfError );

			// At this point, you could call GetCustomErrorObject
			// and query for additional interfaces to determine
			// what else happened.

			WCHAR wstr[2048] = {0, };
			swprintf( wstr, L"HRESULT: %lx, Minor Code: %lu, Source: %s, Description: %s",
                ErrorInfo.hrError,
				ErrorInfo.dwMinor,	// 프로바이더 제공 에러코드 
				bstrSourceOfError,	//
				bstrDescriptionOfError);

			char szStr[2048] = {0, };
			int nMultibyteSize = 0;
			nMultibyteSize = WideCharToMultiByte( CP_ACP, 0, wstr, wcslen(wstr), szStr, 2048, NULL, 0 );
			szStr[ nMultibyteSize ] = '\0';

#ifdef _UNICODE
			Log( wstr );
#else
			Log( szStr );
#endif
			// Free the resources.
			SysFreeString(bstrDescriptionOfError);
			SysFreeString(bstrSourceOfError);
			pErrorInfoRec->Release();
		}

		// Release the error object.
		pErrorRecords->Release();
		pErrorInfo->Release();

		if(pSupportErrorInfo)
			pSupportErrorInfo->Release();	
	}
}

void COleDBSession::ErrorDisplayEx( HRESULT hrErr, wchar_t* szQuery )
{
	DWORD MYLOCALEID = GetUserDefaultLCID();  // A locale is a collection of language-related

	IErrorInfo*         pErrorInfo			= NULL;
	IErrorInfo*         pErrorInfoRec		= NULL;
	IErrorRecords*      pErrorRecords		= NULL;
	ISupportErrorInfo*	pSupportErrorInfo	= NULL;
	HRESULT             hr					= NULL;
	ULONG               i = 0, ulNumErrorRecs = 0;
	ERRORINFO           ErrorInfo;

	if (!FAILED(hrErr))      
		return;

	// Check that the current interface supports error objects.
	hr = m_pICommandText->QueryInterface(IID_ISupportErrorInfo, (void**) &pSupportErrorInfo);
	if(SUCCEEDED(hr)) 
	{
		GetErrorInfo(0,&pErrorInfo);         
		if(!pErrorInfo) 
		{
			if(pSupportErrorInfo)
				pSupportErrorInfo->Release();

			return;
		}

		pErrorInfo->QueryInterface(IID_IErrorRecords, (void**) &pErrorRecords);
		pErrorRecords->GetRecordCount(&ulNumErrorRecs);

		// Read through error records and display them.
		for (i = 0; i < ulNumErrorRecs; i++)
		{
			// Get basic error information.
			pErrorRecords->GetBasicErrorInfo(i, &ErrorInfo);

			// Get error description and source through the
			// IErrorInfo interface pointer on a particular record.
			hr = pErrorRecords->GetErrorInfo(i, MYLOCALEID, &pErrorInfoRec);

			BSTR bstrDescriptionOfError = NULL;
			BSTR bstrSourceOfError		= NULL;

			pErrorInfoRec->GetDescription(&bstrDescriptionOfError);
			pErrorInfoRec->GetSource(&bstrSourceOfError);

			WCHAR wstr[2048] = {0, };
			// ErrorInfo.dwMinor : 프로바이더 제공 에러코드 
			swprintf(wstr,L"HRESULT: %lx, Minor Code: %lu, Source: %s, Description: %s, Query : %s",
				ErrorInfo.hrError, ErrorInfo.dwMinor, bstrSourceOfError, bstrDescriptionOfError, szQuery);

			char szStr[2048] = {0, };
			int nMultibyteSize = 0;
			nMultibyteSize = WideCharToMultiByte(CP_ACP, 0, wstr, wcslen(wstr), szStr, 2048, NULL, 0);
			szStr[ nMultibyteSize ] = '\0';

#ifdef _UNICODE
			Log( wstr );
#else
			Log( szStr );
#endif

			// Free the resources.
			SysFreeString(bstrDescriptionOfError);
			SysFreeString(bstrSourceOfError);
			pErrorInfoRec->Release();
		}

		// Release the error object.         
		pErrorRecords->Release();
		pErrorInfo->Release();

		if(pSupportErrorInfo)
			pSupportErrorInfo->Release();
	}
}

BOOL COleDBSession::ChangeDB( char* szDbName )
{
	HRESULT		hr = NULL;
	DBPROP		InitProperties[1];
	DBPROPSET   rgInitPropSet[1];

	WCHAR wszDbName[256] = {0,};
	KSCToUnicode( szDbName, wszDbName );

	InitProperties[0].dwPropertyID		= DBPROP_CURRENTCATALOG;
	InitProperties[0].vValue.vt			= VT_BSTR;
	InitProperties[0].vValue.bstrVal	= SysAllocString(wszDbName);
	InitProperties[0].dwOptions			= DBPROPOPTIONS_REQUIRED;
	InitProperties[0].colid				= DB_NULLID;

	rgInitPropSet[0].guidPropertySet	= DBPROPSET_DATASOURCE;
	rgInitPropSet[0].cProperties		= 1;
	rgInitPropSet[0].rgProperties		= InitProperties;

	hr = m_pIDBProperties->SetProperties(1, rgInitPropSet); 

	SysFreeString(InitProperties[0].vValue.bstrVal);

	return SUCCEEDED(hr);
}

DBCOLUMNINFO* COleDBSession::GetColumnInfo( char* szQuerySQL, DWORD* pColnum )
{
	HRESULT				 hr					= NULL;
	IRowset*			 pIRowset			= NULL;
	IColumnsInfo*        pIColumnsInfo		= NULL;
	DBCOLUMNINFO*        pDBColumnInfo      = NULL;
	WCHAR*               pStringsBuffer		= NULL;

	WCHAR wszQuerySQL[ MAX_SQL_STRING_LENGTH + 1 ] = {0,};
	KSCToUnicode( szQuerySQL, wszQuerySQL );	

	hr = m_pICommandText->SetCommandText(DBGUID_DBSQL, wszQuerySQL);
	FAIL_RETURN(hr, NULL);

	if(!SetCommandExecute(wszQuerySQL, DEF_IID_IROWSET, NULL, &pIRowset))
		return NULL;

	// Obtain access to the IColumnInfo interface, from the Rowset  object.
	hr = pIRowset->QueryInterface(IID_IColumnsInfo, (void **)&pIColumnsInfo);
	FAIL_RETURN(hr, NULL);

	// Retrieve the column information.
	pIColumnsInfo->GetColumnInfo(pColnum, &pDBColumnInfo, &pStringsBuffer);

	// Free the column information interface.
	pIColumnsInfo->Release();
	pIRowset->Release();

	return pDBColumnInfo;
}

int	COleDBSession::QueryDBCatalog( DBSCHEMA* pSchemaBuf, DWORD dwMaxNumRows )
{	
	HRESULT						hr					= NULL;
	IDBSchemaRowset				*pIDBSchemaRowset	= NULL;
	IRowset*					pIRowset			= NULL;
	IAccessor*					pIAccessor			= NULL;
	HACCESSOR					hAccessor			= 0;
	ULONG						lret				= 0;
	DWORD						dwOffset			= 0;
	DWORD						dwTotalReceivedRow	= 0;
	WCHAR						wBuffer[ 0xff ]		= {0, };
	char						szBuffer[ 1024 ]	= {0, };
	HROW		                hRows[10]			= {NULL,};
	HROW*				        pRows				= &hRows[0];
	const static DBCOUNTITEM	cBindings			= 1;
	DBBINDING rgBindings[cBindings] = 
	{
			1,		// iOrinal
			0,		// pData = phChapter + 0 offset = phChapter
			0,
			0,	
			NULL,			
			NULL, 		
			NULL,		
			DBPART_VALUE,
			DBMEMOWNER_CLIENTOWNED,		
			DBPARAMIO_NOTPARAM, 
			MAX_SCHEMA_BUFFER_SIZE,
			0, 				
			DBTYPE_WSTR,
			0,	
			0, 				
	};

	// IDBSchemaRowset의 포인터 얻기 		
	hr = m_pIDBCreateCommand->QueryInterface(IID_IDBSchemaRowset, (void**)&pIDBSchemaRowset);
	FAIL_RETURN(hr, -7000);

	hr = pIDBSchemaRowset->GetRowset(NULL, DBSCHEMA_CATALOGS, 0, 
										NULL, IID_IRowset, 0, NULL, (IUnknown**)&pIRowset);
	FAIL_RETURN(hr, -7100);

    hr = pIRowset->QueryInterface(IID_IAccessor, (void **) &pIAccessor);
    FAIL_RETURN(hr, -7200);

	hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA, cBindings, rgBindings, 0, &hAccessor, NULL);
	FAIL_RETURN(hr, -7300);

	hr = pIRowset->GetNextRows(DB_NULL_HCHAPTER, 0, 10, &lret,	&pRows);
	FAIL_RETURN(hr, -7400);	

	while( lret > 0 ) 
	{
        //For each row, print the column data.
		for( ULONG j = 0; j < lret; j++ )
		{			
			if( dwTotalReceivedRow >= dwMaxNumRows )	// 파라미터로 지정해준 MAX 행까지 받았으면 Release 한후 끝낸다.
			{
				pIRowset->ReleaseRows(lret, hRows, NULL, NULL, NULL);
				goto QUERYEND;
			}
			
			wBuffer[0] = L'\0';
			pIRowset->GetData(hRows[j], hAccessor, wBuffer);
			
			wsprintf( (LPTSTR)szBuffer, (LPCTSTR)"%S", wBuffer );
			memcpy(pSchemaBuf->szSchemaBuffer + dwOffset, szBuffer, strlen(szBuffer));
			dwOffset += MAX_SCHEMA_BUFFER_SIZE;
			dwTotalReceivedRow++;
		}
		 
        pIRowset->ReleaseRows(lret, hRows, NULL, NULL, NULL);
        pIRowset->GetNextRows(NULL, 0, 10, &lret, &pRows);
	}

QUERYEND:

	pIDBSchemaRowset->Release();
	pIAccessor->ReleaseAccessor(hAccessor, NULL);
	pIAccessor->Release();
	pIRowset->Release();
	
	return (int)dwTotalReceivedRow;
}

int COleDBSession::QueryDBTable( DBSCHEMA *pSchemaBuf, DWORD dwMaxNumRows )
{	
	HRESULT						hr					= NULL;
	IDBSchemaRowset				*pIDBSchemaRowset	= NULL;
	IRowset*					pIRowset			= NULL;
	IAccessor*					pIAccessor			= NULL;
	HACCESSOR					hAccessor			= 0;
	ULONG						lret				= 0;
	DWORD						dwOffset			= 0;
	DWORD						dwTotalReceivedRow	= 0;
	WCHAR						wBuffer[ 0xff ]		= {0,};
	char						szBuffer[ 1024 ]	= {0,};
	HROW		                hRows[10]			= {NULL,};
	HROW*				        pRows				= &hRows[0];
	const static DBCOUNTITEM	cBindings			= 1;
	DBBINDING rgBindings[cBindings] = 
	{
			3,		// 테이블 이름만 받아오자..   테이블 이름이 3번째 칼럼...
			0,		// pData = phChapter + 0 offset = phChapter
			0,
			0,	
			NULL,			
			NULL, 		
			NULL,		
			DBPART_VALUE,
			DBMEMOWNER_CLIENTOWNED,		
			DBPARAMIO_NOTPARAM, 
			MAX_SCHEMA_BUFFER_SIZE,
			0, 				
			DBTYPE_WSTR,
			0,	
			0, 				
	};

	//IDBSchemaRowset의 포인터 얻기 		
	hr = m_pIDBCreateCommand->QueryInterface(IID_IDBSchemaRowset, (void**)&pIDBSchemaRowset);
	FAIL_RETURN(hr, -8000);

	//Restriction 값 주기...   UserTable만 뽑아내기 위해 	
	VARIANT		rgRestrictions[ 4 ];
	ULONG		cRestrictions = 4;
	
	for( ULONG j = 0; j < 4; j++ )
        VariantInit( &rgRestrictions[j] );
	
//	rgRestrictions[0].bstrVal = SysAllocString(L"dragon");
//	rgRestrictions[0].vt = VT_BSTR;
	
	rgRestrictions[3].bstrVal = SysAllocString(L"TABLE");
	rgRestrictions[3].vt = VT_BSTR;	

	hr = pIDBSchemaRowset->GetRowset(NULL, DBSCHEMA_TABLES, cRestrictions, 
										rgRestrictions, IID_IRowset, 0, NULL, (IUnknown**)&pIRowset);
	FAIL_RETURN(hr, -8100);

    hr = pIRowset->QueryInterface(IID_IAccessor, (void **) &pIAccessor);
    FAIL_RETURN(hr, -8200);

	hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA, cBindings, rgBindings, 0, &hAccessor, NULL);
	FAIL_RETURN(hr, -8300);

	hr = pIRowset->GetNextRows(DB_NULL_HCHAPTER, 0, 10, &lret,	&pRows);
	FAIL_RETURN(hr, -8400);	

	while(lret > 0) 
	{
        // For each row, print the column data.
		for( ULONG j = 0; j < lret; j++ )
		{			
			if( dwTotalReceivedRow >= dwMaxNumRows )	// 파라미터로 지정해준 MAX 행까지 받았으면 Release 한후 끝낸다.
			{
				pIRowset->ReleaseRows(lret, hRows, NULL, NULL, NULL);
				goto QUERYEND;
			}
			
			wBuffer[0] = L'\0';
			pIRowset->GetData(hRows[j], hAccessor, wBuffer);
			
			wsprintf( (LPTSTR)szBuffer, (LPCTSTR)"%S", wBuffer );
			memcpy(pSchemaBuf->szSchemaBuffer + dwOffset, szBuffer, strlen(szBuffer));		//Unicode -> Ansi   짱나..ㅡㅡ
			dwOffset += MAX_SCHEMA_BUFFER_SIZE;
			dwTotalReceivedRow++;
		}
		 
        pIRowset->ReleaseRows(lret, hRows, NULL, NULL, NULL);
        pIRowset->GetNextRows(NULL, 0, 10, &lret, &pRows);
	}

QUERYEND:

	pIDBSchemaRowset->Release();
	pIAccessor->ReleaseAccessor(hAccessor, NULL);
	pIAccessor->Release();
	pIRowset->Release();
	
	return (int)dwTotalReceivedRow;
}

LONG COleDBSession::SetCommandExecute( wchar_t* szQuerySQL, BYTE byType, IRowsetChange** pIRowsetChange,
									  IRowset** pIRowset, DBPARAMS* dbParams, BOOL bParam, LONG* cNumRows )
{
	// cNumRows : Update, Insert, Delete 시에 몇개의 행이 영향을 받았는지.

	HRESULT	hr = NULL;
	LONG lNumRows = 0;

	__try
	{
		::SetEvent( GetExcuteEvent() );	

		// Execute the command.	
		switch(byType)
		{
			case DEF_IID_NULL:		
				{
					hr = (bParam) ?	m_pICommandText->Execute(NULL, IID_NULL, dbParams, &lNumRows, NULL) :
									m_pICommandText->Execute(NULL, IID_NULL, NULL, &lNumRows, NULL);
				}
				break;

			case DEF_IID_IROWSETCHANGE:
				hr = m_pICommandText->Execute(NULL, IID_IRowsetChange, NULL, &lNumRows, (IUnknown**)pIRowsetChange);
				break;

			case DEF_IID_IROWSET:
				hr = m_pICommandText->Execute(NULL, IID_IRowset, NULL, &lNumRows, (IUnknown**)pIRowset);
				break;
		}

		if(cNumRows)
			*cNumRows = lNumRows;

		SetExcuteInit();

		if(FAILED(hr))
		{
			ErrorDisplayEx(hr, szQuerySQL);
			return 0;
		}

		return 1;
	}
	__except(UnhandledExceptionHandler(GetExceptionInformation()))
	{
		return 0;
	}
}

unsigned __stdcall COleDBSession::WorkerThread( void* lpParam )
{
	COleDBSession* pOleDBSession = (COleDBSession*)lpParam;

	pOleDBSession->ProcessTimeOut();

	_endthreadex( 0 );
	return 0;
}

void COleDBSession::ProcessTimeOut()
{
	for(;;)
	{
		if( WAIT_OBJECT_0 == ::WaitForSingleObject( GetExcuteEvent(), INFINITE ) )
		{
			if( m_byTimeOut == DEF_EXCUTE_END )
				return;
			
			DWORD dwPrevTime = ::timeGetTime();
			DWORD dwCurrTime = ::timeGetTime();

			while( dwPrevTime + m_dwCommandTimeOut > dwCurrTime )
			{
				dwCurrTime = ::timeGetTime();

				if( m_byTimeOut == DEF_EXCUTE_START )
					break;
			}

			// 타임아웃 되었을 경우... 로그 남기고 작업 취소한다.
			if( m_byTimeOut == DEF_EXCUTE_TIMEOUT )
			{
				WCHAR wszQuery[2048]={0,};
				struct _GUID DBGUID = DBGUID_DBSQL;
				m_pICommandText->GetCommandText(&DBGUID, (WCHAR**)&wszQuery);

				WCHAR wstr[2048] = {0, };
				swprintf(wstr,L"*** Query canceled by time out: %s", wszQuery);

				char szStr[2048] = {0, };
				int nMultibyteSize = 0;
				nMultibyteSize = WideCharToMultiByte(CP_ACP, 0, wstr, wcslen(wstr), szStr, 2048, NULL, 0);
				szStr[ nMultibyteSize ] = '\0';

				m_pOledbSource->GetDesc().OutputMessageFunc( szStr );

				m_pICommandText->Cancel();
			}
		}
	}	
}

END_NAMESPACE