#include "stdafx.h"
#include "Service.h"
#include "Synchronized.h"
#include "IOCPManager.h"
#include "ExceptionFilter.h"
#include "Monitor.h"
#include "Netmsg.h"

/* 
	오늘 깨닫은 열라 중요한 뽀인트
	C****Service::DestroyNetwork()에서
	Session들을 delete한다음에 막바로 CService::DestroyNetwork()이 호출됐더니..
	CSession::~CSession()에서 Close()를 호출하쟎냐... 그것들이 IOCP쪽에 Signal을 발생시키는 겄이었다.
	그러니까... Session들이 delete됐으니 포인터들은 무효화됐는데... 이전에 IOCP에 post됐던 것들이 IOCP쪽으로 통해서 오기 때문에...
	무효화됀 포인터를 참조하게 되고.... _Crt에서는 지랄을 하게 됐다.
	결국... IOCP를 먼저 죽인다음에 세션들을 delete해줬다..
	그런데 참.. 세션들에 걸려있던 netmsg들은 따로 처리해줘야 겠네...
*/
START_NAMESPACE

CService*	g_pServiceModule = NULL;
CMonitor	g_Monitor;

DWORD DecodeCDBData(char* szLoadFile,  void* pReturnValue, char* szDecodeKey, int nDecodeSubKey)
{
	int nKeyLen = lstrlen(szDecodeKey);

	FILE* fp;	
	DWORD	dwCur = 0;
	DWORD   dwTotalLen = 0;
	BOOL bRet = TRUE;

	fp = fopen( szLoadFile, "rb" );
	if(!fp)
		return FALSE;

	fread(&dwTotalLen, sizeof(DWORD), 1, fp );

	char* szBuffer = new char[ dwTotalLen ];

	int nRemain;
	while( bRet )
	{
		if(!fread(szBuffer + dwCur, nKeyLen, 1, fp ))
			bRet = FALSE;

		nRemain = nKeyLen;
		if(!bRet)
			nRemain = dwTotalLen - dwCur;

		for(int k=0; k<nRemain; k++ )
			szBuffer[ dwCur + k ] ^= (szDecodeKey[k] + nDecodeSubKey);

		dwCur += nRemain;
	}

	memcpy(pReturnValue, szBuffer, dwTotalLen);
	delete [] szBuffer;
	fclose(fp);

	return dwCur;	//총 읽어드린 바이트수를 리
}

LONG WINAPI UnhandledExceptionHandler( _EXCEPTION_POINTERS* pExceptionInfo )
{
	Log( _T("!!!!! Unhandled Exception Occured !!!!!\n\n\n\n\n\n\n\n") );

	TCHAR strFileName[_MAX_PATH];

	struct tm when;
	__time64_t now;

	_time64( &now );
	when = *_localtime64( &now );

	::StringCbPrintf( strFileName, 256, _T("c:\\%s %4d%-02d-%02d %02d-%02d-%02d.DMP"),
        g_pServiceModule->GetServiceName(), when.tm_year + 1900, when.tm_mon + 1, when.tm_mday, when.tm_hour, when.tm_min, when.tm_sec);

	LONG lRetVal = EXCEPTION_CONTINUE_SEARCH;

	HANDLE hFile = ::CreateFile( strFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		_MINIDUMP_EXCEPTION_INFORMATION exInfo;

		exInfo.ThreadId = ::GetCurrentThreadId();
		exInfo.ExceptionPointers = pExceptionInfo;
		exInfo.ClientPointers = NULL;

		if ( MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &exInfo, NULL, NULL ) )
			lRetVal = EXCEPTION_EXECUTE_HANDLER;

		SAFE_CLOSEHANDLE( hFile );
	}

	return lRetVal;
}

void WINAPI CService::ServiceHandler( unsigned long dwControl )
{
	switch ( dwControl )
	{
		case SERVICE_CONTROL_STOP :
		case SERVICE_CONTROL_SHUTDOWN :
			Log( _T("Stop Service for Service Control") );
			g_pServiceModule->OnStopService();
			break;
	}
}

void WINAPI CService::ServiceMain( unsigned long dwArgc, TCHAR **lpszServiceName )
{
	assert( g_pServiceModule );
	::StringCchCopy( g_pServiceModule->m_strServiceName, 256, lpszServiceName[0] );

//	Log("RCNET Last Build at %s, %s", __DATE__, __TIME__);

	if ( g_pServiceModule->StartService() == 0 )
	{
		g_pServiceModule->WaitForTerminateSignal();
	}

	g_pServiceModule->StopService();
}

CService::CService( bool bService )	: m_bService( bService )
{
//	::SetUnhandledExceptionFilter( UnhandledExceptionHandler );
	SET_GLOBAL_EXCEPTION_FILTER();

	assert( g_pServiceModule == NULL );
	
	g_pServiceModule = this;

	m_pIOCPManager = NULL;
	m_hInstance = NULL;

	m_hKillService = CreateEvent( NULL, TRUE, FALSE, NULL );

	ZeroMemory( m_strPath, sizeof(m_strPath) );
	
	m_pAdapterInfo = NULL;
	m_pInterfaceInfo = NULL;
}

CService::~CService()
{
	::WSACleanup();

	if ( m_pAdapterInfo )
		free( m_pAdapterInfo );

	if ( m_pInterfaceInfo )
		free( m_pInterfaceInfo );
}

int CService::Start( int argc, TCHAR* argv[] )
{
	if ( !m_bService )
	{
		::StringCchCopy( argv[0], 10, _T("LSGame") );
		ServiceMain( 0, argv );
		return 0;
	}
	else
	{
		if ( argc == 3 )
		{
			if ( !_tcsicmp( argv[2],  _T("install") ) )
				return ( CService::RegisterService( argv[1] ) );
			if ( !_tcsicmp( argv[2],  _T("uninstall") ) )
				return ( CService::RemoveService( argv[1] ) );
		}

		SERVICE_TABLE_ENTRY	ServiceTable[] = { { _T(""), ServiceMain }, { NULL, NULL } };
		StartServiceCtrlDispatcher(ServiceTable);

		return 0;
	}
}

int CService::Start( HINSTANCE hInstance, LPCTSTR strCmdLine )
{
	srand( ::GetTickCount() );

	m_hInstance = hInstance;

	// 일단 서비스모드로 작동 안하게
	m_bService = false;

	if ( !m_bService )
	{
		MONITOR->Initialize( strCmdLine );
		ServiceMain( 0, (TCHAR **)&strCmdLine );
		return 0;
	}
	else
	{
		TCHAR szServerName[256], szOption[32];
		_stscanf( strCmdLine, _T("%s %s"), szServerName, szOption );

		if ( !_tcsicmp( szOption, _T("install") ) )
			return ( CService::RegisterService( szServerName ) );
		else if ( !_tcsicmp( szOption,  _T("uninstall") ) )
			return ( CService::RemoveService( szServerName ) );
		else
		{
			SERVICE_TABLE_ENTRY	ServiceTable[] = { { _T(""), ServiceMain }, { NULL, NULL } };
			StartServiceCtrlDispatcher( ServiceTable );

			return 0;
		}
	}
}

void CService::OnStopService()
{
	SetEvent( m_hKillService );
}

int CService::StartService()
{
	Log( _T("---------- Start Service ----------") );

	CoInitializeEx( NULL, COINIT_MULTITHREADED );
	m_hProcessIdentify = ::CreateMutex( NULL, FALSE, m_strServiceName );
	
	if ( m_hProcessIdentify )
	{
		TCHAR szRegKey[1024];
		::StringCbPrintf( szRegKey, 1024, SERVICE_REGKEY_NAME, m_strServiceName );

		m_Registry.Open( szRegKey );

		if ( InitDatabase() )
		{
			SERVICE_STATUS ss;

			if ( m_bService )
			{
				m_hSS = RegisterServiceCtrlHandler( m_strServiceName, ServiceHandler );

				ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
				ss.dwCurrentState = SERVICE_START_PENDING;
				ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
				ss.dwWin32ExitCode = NO_ERROR;
				ss.dwServiceSpecificExitCode = 0;
				ss.dwCheckPoint = 1;
				ss.dwWaitHint = 1000;

				SetServiceStatus(m_hSS, &ss);
			}

			if ( InitNetwork() )
			{
				if (m_bService)
				{
					ss.dwCurrentState = SERVICE_RUNNING;
					ss.dwCheckPoint = 0;
					ss.dwWaitHint = 0;
					SetServiceStatus(m_hSS, &ss);
				}

				if ( InitApplication() )
				{
					return 0;
				}
				else
				{
					return -4;
				}
			}
			else
			{
				return -3;
			}
		}
		else
		{
			return -2;
		}
	}
	else
	{
		return -1;	
	}
}

int CService::StopService()
{
	Log( _T("Stopping Service....") );
	SERVICE_STATUS ss;

	if ( m_bService )
	{
		ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
		ss.dwWin32ExitCode = NO_ERROR;
		ss.dwServiceSpecificExitCode = 0;

		ss.dwCurrentState = SERVICE_STOP_PENDING;
		ss.dwCheckPoint = 1;
		ss.dwWaitHint = 5000;
		::SetServiceStatus(m_hSS, &ss);
	}

	SAFE_CLOSEHANDLE( m_hKillService );
	
	DestroyNetwork();
	DestroyDatabase();
	DestroyApplication();
		
	Log( _T("Destroy all system objs") );
	
	SAFE_CLOSEHANDLE( m_hProcessIdentify );
	Log( _T("Destroy process identify mutex") );
	
	CoUninitialize();
	Log( _T("CoUninitialize") );

	Log( _T("NETMSG not released : %d"), CNetMsg::s_NetMsgManager.m_nMsgOut);

	Log( _T("---------- Service Stopped ----------\r\n") );
	if ( m_bService )
	{
		ss.dwCurrentState = SERVICE_STOPPED;
		ss.dwCheckPoint = 2;
		ss.dwWaitHint = 1000;
		::SetServiceStatus(m_hSS, &ss);
	}

	if ( !m_bService )
        MONITOR->Release();

	return ( 0 );
}

void CService::AddEventLog( const TCHAR* strFormat, ... )
{
	HANDLE hEventSource;
	const TCHAR *lpszString[2];
	va_list args;
	TCHAR strBuffer[1024], strTemp[1024];

	Synchronized sync ( &m_sectionEventLog );

	va_start( args, strFormat );
	::StringCbVPrintf( strBuffer, 1024, strFormat, args );
	va_end( args );

	hEventSource = RegisterEventSource( NULL, m_strServiceName );

	::StringCbPrintf( strTemp, 1024, _T("Last ErrorCode : %d"), GetLastError() );
	lpszString[0] = strTemp;
	lpszString[1] = strBuffer;

	if ( hEventSource )
	{
		ReportEvent( hEventSource, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, 2, 0, lpszString, NULL );
		DeregisterEventSource( hEventSource );
	}
}

int CService::SetRegistryValues( CRegistry& reg )
{
	long lRet;

	lRet = reg.Set((const TCHAR *)_T("Port"), 0);
	if (lRet != 0)
		return lRet;

	lRet = reg.Set((const TCHAR *)_T("Capacity"), 1000);
	if (lRet != 0)
		return lRet;

	lRet = reg.Set((const TCHAR *)_T("DBIP"), _T(" ") );
	if (lRet != 0)
		return lRet;

	lRet = reg.Set((const TCHAR *)_T("DBName"), _T(" "));
	if (lRet != 0)
		return lRet;

	lRet = reg.Set((const TCHAR *)_T("DBID"), _T(" ") );
	if (lRet != 0)
		return lRet;

	lRet = reg.Set((const TCHAR *)_T("DBPass"), _T(" ") );
	if (lRet != 0)
		return lRet;

	return 0;
}

int CService::RegisterService( const TCHAR *szServiceName )
{
	TCHAR szModuleFile[_MAX_PATH];
	memset(szModuleFile , 0x00 , _MAX_PATH );
	GetModuleFile( szModuleFile );
	
	SC_HANDLE hSCM = ::OpenSCManager( NULL, NULL, SC_MANAGER_CREATE_SERVICE );
	SC_HANDLE hService = ::CreateService(	hSCM,
											szServiceName,
											szServiceName,
											0,
											SERVICE_WIN32_OWN_PROCESS,
											SERVICE_DEMAND_START,
											SERVICE_ERROR_IGNORE,
                                            szModuleFile,
											NULL,
											NULL,
											NULL,
											NULL,
											NULL);	
	::CloseServiceHandle( hService );
	::CloseServiceHandle( hSCM );

	TCHAR szRegKey[1024];
	::StringCbPrintf( szRegKey, 1024, SERVICE_REGKEY_NAME, szServiceName );

	CRegistry reg;
	DWORD dwStatus;

	if ( reg.Create( szRegKey, dwStatus ) == ERROR_SUCCESS )
	{
		return SetRegistryValues( reg );
	}
	else
	{
		MessageBox( NULL, _T("등록 실패"), szServiceName, MB_OK);
		return -1;
	}	
}

int CService::RemoveService( const TCHAR *szServiceName )
{
	SC_HANDLE hSCM = ::OpenSCManager( NULL, NULL, SC_MANAGER_CREATE_SERVICE );
	SC_HANDLE hService = ::OpenService( hSCM, szServiceName, DELETE );
	
	::DeleteService( hService );
	::CloseServiceHandle( hService );
	::CloseServiceHandle( hSCM );

	return ( 0 );
}

void CService::GetLocalIP( TCHAR *szIP )
{
	char name[512];
	PHOSTENT hostinfo;

	if ( gethostname( name, sizeof( name ) ) == 0 )
	{
		if ( ( hostinfo = gethostbyname( name ) ) != NULL )
		{
			::StringCchCopy( szIP, 256, CODE_CONVERT_WIDE( inet_ntoa( *( struct in_addr* )*hostinfo->h_addr_list ) ) );
		}
	}
}

void CService::GetModuleFile( TCHAR* szDir )
{
	HMODULE hModule;
	hModule = ::GetModuleHandle(NULL);
	if ( hModule == NULL ) return;

	::GetModuleFileName( hModule, szDir, _MAX_PATH );
}

void CService::WaitForTerminateSignal()
{
#ifdef _DEBUG
	while (true)
	{
		DWORD dwRet = ::WaitForSingleObject( m_hKillService, 1 );
		if (dwRet == WAIT_OBJECT_0)
			return;
		if ( GetAsyncKeyState('X') < 0 && GetAsyncKeyState(VK_SHIFT) < 0 && GetAsyncKeyState(VK_CONTROL) < 0 )
			return;
	}
#else
	WaitForSingleObject( m_hKillService, INFINITE );
#endif
}

bool CService::InitNetwork()
{
	int nPort = 0;
	int nCapacity = 0;
	TCHAR strAddr[256];

	WORD wVersionReq;
	WSADATA wsaData;

	GetNetworkInterfaceEntry();
	GetNetworkInterfaceInfo();
	GetNetworkAdaptersInfo();

	wVersionReq = MAKEWORD(2, 2);

	if ( WSAStartup( wVersionReq, &wsaData ) == 0 )
	{
		if (m_Registry.Get( _T("Addr"), strAddr ) != 0 )
		{
			GetLocalIP( m_strServerIP );
		}
		else
		{
			::StringCchCopy( m_strServerIP, 256, strAddr );
		}

		Log( _T("IP Addr = %s"), m_strServerIP );
		
		if ( m_Registry.Get( _T("Port"), nPort ) == 0 )
		{
			m_nAcceptPort = nPort;
		}

		CNetMsg::s_NetMsgManager.Initialize();

		m_pIOCPManager = new CIOCPManager;

		return m_pIOCPManager->Create(0);		
	}
	else
	{
		return false;
	}
}

void CService::DestroyNetwork()
{
	if ( m_pIOCPManager )
	{
		m_pIOCPManager->Destroy();
		delete m_pIOCPManager;
		m_pIOCPManager = NULL;
	}

	CNetMsg::s_NetMsgManager.Destroy();	
}

bool CService::InitApplication() 
{
	::GetModuleFileName( NULL, m_strPath, 256 );

	for ( int i = (int)_tcslen(m_strPath) - 1 ; i >= 0 ; i-- )
	{
		if ( m_strPath[i] == '\\' )
		{
			m_strPath[i] = '\0';
			break;
		}
	}

	m_Registry.Get( _T("ShardName"), m_strShardName );

	::StringCbPrintf( m_strExceptionStackDumpPath, _MAX_PATH, _T("%s\\ExceptionStackDump%d"), m_strPath, m_nAcceptPort );
	
	HANDLE hSrch;
	WIN32_FIND_DATA wfd;

	hSrch = FindFirstFile( m_strExceptionStackDumpPath, &wfd );
	if ( hSrch == INVALID_HANDLE_VALUE ) 	
		::CreateDirectory( m_strExceptionStackDumpPath, NULL );	

	return true;
};

void CService::GetNetworkInterfaceEntry()
{
	// Declare and initialize variables.
	PMIB_IFTABLE ifTable;
	PMIB_IFROW pMibIfRow;
	DWORD dwSize = 0;
	DWORD dwRetVal = 0;

	// Get size information
	::GetIfTable(NULL, &dwSize, FALSE);

	// Allocate memory for our pointers.
	ifTable		= (MIB_IFTABLE*)malloc(dwSize);
	pMibIfRow	= (MIB_IFROW*)malloc(sizeof(MIB_IFROW));

	// Before calling GetIfEntry, we call GetIfTable to make
	// sure there are entries to get.

	// Make an initial call to GetIfTable to get the
	// necessary size into dwSize
	if( ::GetIfTable(ifTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER ) 
	{
		free( ifTable );
		ifTable = (MIB_IFTABLE*)malloc(dwSize);
	}

	// Make a second call to GetIfTable to get the actual
	// data we want.
	if ((dwRetVal = ::GetIfTable(ifTable, &dwSize, 0)) == NO_ERROR) 
	{
		for( DWORD dwIndex = 0 ; dwIndex < ifTable->dwNumEntries ; dwIndex++ )
		{
			// Get interface description
			*pMibIfRow = (ifTable->table[ dwIndex ]);

			// Is this a Ethernet interface?
			if ((pMibIfRow->dwPhysAddrLen == 6) && (pMibIfRow->dwType == MIB_IF_TYPE_ETHERNET))
			{
				//				char szBuf[ 8 ];
				char szBuf[0xFF] = {0,};

				// Format physical address
				DWORD i = 0; 
				for ( ; i < pMibIfRow->dwPhysAddrLen; ++i )
				{
					sprintf( &szBuf[i*3], "%02X-", pMibIfRow->bPhysAddr[i] );
				}

				szBuf[i*3-1] = '\0';
				Log( "PsyAddr: %s", szBuf );
			}
		}
	}
	else 
	{
		Log( "GetIfTable failed." );
	}

	free( ifTable );
	free( pMibIfRow );
}

void CService::GetNetworkInterfaceInfo()
{
	// Declare and initialize variables
	//	PIP_INTERFACE_INFO m_pInterfaceInfo;
	m_pInterfaceInfo = (IP_INTERFACE_INFO *)malloc( sizeof(IP_INTERFACE_INFO) );
	if( !m_pInterfaceInfo )
		return;

	DWORD dwRetVal		= 0;
	ULONG ulOutBufLen	= sizeof(IP_INTERFACE_INFO);

	// Make an initial call to GetInterfaceInfo to get
	// the necessary size in the ulOutBufLen variable
	switch( GetInterfaceInfo(m_pInterfaceInfo, &ulOutBufLen) )
	{
	case ERROR_INSUFFICIENT_BUFFER :
		{
			free(m_pInterfaceInfo);
			m_pInterfaceInfo = (IP_INTERFACE_INFO *)malloc(ulOutBufLen);
		}
		break;

	case ERROR_NOT_SUPPORTED :
		{
			free(m_pInterfaceInfo);
			return;
		}
		break;
	}

	// Make a second call to GetInterfaceInfo to get
	// the actual data we need
	if ((dwRetVal = GetInterfaceInfo(m_pInterfaceInfo, &ulOutBufLen)) == NO_ERROR ) 
	{
		Log( "Adapter Name: %ws", m_pInterfaceInfo->Adapter[0].Name );
		Log( "Adapter Index: %ld", m_pInterfaceInfo->Adapter[0].Index );
		Log( "Num Adapters: %ld", m_pInterfaceInfo->NumAdapters );
	}
	else 
	{
		Log( "GetInterfaceInfo failed." );
	}
}

void CService::GetNetworkAdaptersInfo()
{
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;

	m_pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if ( GetAdaptersInfo( m_pAdapterInfo, &ulOutBufLen ) == ERROR_BUFFER_OVERFLOW ) 
	{
		free( m_pAdapterInfo );
		m_pAdapterInfo = (IP_ADAPTER_INFO *) malloc ( ulOutBufLen ); 
	}

	if ( ( dwRetVal = GetAdaptersInfo( m_pAdapterInfo, &ulOutBufLen ) ) == NO_ERROR ) 
	{
		pAdapter = m_pAdapterInfo;
		while ( pAdapter ) 
		{
			Log( "Adapter Name: %s", pAdapter->AdapterName );
			Log( "Adapter Desc: %s", pAdapter->Description );
			Log( "Adapter Addr: %ld", pAdapter->Address );
			Log( "IP Address: %s", pAdapter->IpAddressList.IpAddress.String );
			Log( "IP Mask: %s", pAdapter->IpAddressList.IpMask.String );
			Log( "Gateway: %s", pAdapter->GatewayList.IpAddress.String );
//			Log( "***" );

			if ( pAdapter->DhcpEnabled ) 
			{
				Log( "DHCP Enabled: Yes" );
				Log( "DHCP Server: %s", pAdapter->DhcpServer.IpAddress.String );
				Log( "Lease Obtained: %ld", pAdapter->LeaseObtained );
			}
			else
			{
				Log("DHCP Enabled: No");
			}

			if ( pAdapter->HaveWins ) 
			{
				Log( "Have Wins: Yes" );
				Log( "Primary Wins Server: %s", pAdapter->PrimaryWinsServer.IpAddress.String );
				Log( "Secondary Wins Server: %s", pAdapter->SecondaryWinsServer.IpAddress.String );
			}
			else
			{
				Log("Have Wins: No");
			}

			pAdapter = pAdapter->Next;
		}
	}
	else 
	{
		Log( "Call to GetAdaptersInfo failed." );
	}
}

END_NAMESPACE