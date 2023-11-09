#include "StdAfx.h"
#include "Monitor.h"

START_NAMESPACE

CMonitor::CMonitor(void)
{
	m_fpLog					= NULL;
	m_criLog;
	m_hIn					= NULL;
	m_hOut					= NULL;
	m_dwScreenBufferLastRow = 0;
	m_ConsoleLogLevel		= 2;
	m_FileLogLevel			= 7;
	m_bIsRun				= true;
	InitTarget();
}	

CMonitor::~CMonitor(void)
{
		
}

void CMonitor::Initialize( LPCTSTR szTitle )
{
	InitializeCriticalSectionAndSpinCount( &m_criLog, 1000 );

	AllocConsole();	
	SetConsoleTitle( szTitle );
	
	m_hIn  = GetStdHandle(STD_INPUT_HANDLE);
	m_hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	// To prevent mouse input buffer processing.
	DWORD dwMode = 0;
	GetConsoleMode( m_hIn, &dwMode );
	dwMode &= ~ENABLE_MOUSE_INPUT;
	SetConsoleMode( m_hIn, dwMode );

	HWND hWnd = GetConsoleWindow();
	if( hWnd )
	{
		RECT rt;
		::ShowWindow( hWnd, SW_MINIMIZE );
		::ShowWindow( hWnd, SW_SHOWNORMAL );
		::GetWindowRect( hWnd, &rt );
		::SetWindowPos( hWnd, HWND_TOP, 0, 0, rt.right - rt.left, rt.bottom - rt.top, SWP_NOSIZE | SWP_SHOWWINDOW );		
	}
	else
	{
		hWnd = ::FindWindow( NULL, szTitle );
				
		RECT rt;
		::ShowWindow( hWnd, SW_MINIMIZE );
		::ShowWindow( hWnd, SW_SHOWNORMAL );
		::GetWindowRect( hWnd, &rt );	
		::SetWindowPos( hWnd, HWND_TOP, 0, 0, rt.right - rt.left, rt.bottom - rt.top, SWP_NOSIZE | SWP_SHOWWINDOW );		
	}

//	LogToScreen( LOG_JUST_DISPLAY1, "########### Start Service %s %s ###########\n", __DATE__, __TIME__ );
//	LogToScreen( LOG_JUST_DISPLAY1, "0x%08X\n", hWnd );
}

void CMonitor::Release()
{
//	LogToScreen( LOG_JUST_DISPLAY1, "########### End Service %s %s ###########\n", __DATE__, __TIME__ );

	DeleteCriticalSection( &m_criLog );

	if( m_fpLog )
	{
		fclose( m_fpLog );
	}

	FreeConsole();	
}

void CMonitor::SetConsoleSize( DWORD col,DWORD row )
{
	SMALL_RECT srect;
	COORD	dwSize;

	CONSOLE_SCREEN_BUFFER_INFO info;

	dwSize.X = (short) col;
	dwSize.Y = (short) row;

	SetConsoleScreenBufferSize( m_hOut, dwSize );
	GetConsoleScreenBufferInfo( m_hOut, &info );

	m_dwScreenBufferLastRow = row - 1;
	srect.Top = 0;
	srect.Left = 0;
	srect.Right = SHORT(info.dwMaximumWindowSize.X-1);
	srect.Bottom = SHORT(info.dwMaximumWindowSize.Y-1);
	
	SetConsoleWindowInfo( m_hOut, TRUE, &srect );
}

void CMonitor::WaitForKeyInput()
{
	DWORD dwResult = 0;
	INPUT_RECORD irBuffer;
	memset( &irBuffer, 0, sizeof(INPUT_RECORD) );

	while ( m_bIsRun )
	{
		GetConsoleKeyInput( &irBuffer, &dwResult );

		if ( irBuffer.EventType == KEY_EVENT )
		{
			if ( irBuffer.Event.KeyEvent.bKeyDown )
			{
				switch( irBuffer.Event.KeyEvent.wVirtualKeyCode )
				{
					case VK_ESCAPE: 
						{
							if( MessageBox( NULL, _T("Are you sure to End Service?"), _T("End Service Question"), MB_YESNO | MB_ICONQUESTION ) == IDYES )
							{
								m_bIsRun = false; 
							}							
						}
						break;
					default: break;
				}
			}
		}
	}
}

bool CMonitor::WaitForConsoleKeyInput()
{
	DWORD dwResult = 0;
	INPUT_RECORD irBuffer;
	memset( &irBuffer, 0, sizeof(INPUT_RECORD) );

	::FlushConsoleInputBuffer( m_hIn );

	if( WAIT_TIMEOUT == ::WaitForSingleObject( m_hIn, 500 ) )
		return false;

	GetConsoleKeyInput( &irBuffer, &dwResult );

	if ( irBuffer.EventType == KEY_EVENT )
	{
		if ( irBuffer.Event.KeyEvent.bKeyDown )
		{
			switch( irBuffer.Event.KeyEvent.wVirtualKeyCode )
			{
				case VK_ESCAPE: 
					{
						if( MessageBox( NULL, _T("Are you sure to End Service?"), _T("End Service Question"), MB_YESNO | MB_ICONQUESTION ) == IDYES )
						{
							return true;
						}
					}
					break;

				case VK_DELETE:  // 涝仿等 ID狼 User 碍硼 - Ghost 力芭 格利
					GhostRemover();
					break;

				default: break;
			}
		}
	}

	return false;
}

void CMonitor::GetConsoleKeyInput( PINPUT_RECORD pInput, LPDWORD pResult )
{
//	::ReadConsoleInput( m_hIn, pInput, 1, pResult );
	::PeekConsoleInput( m_hIn, pInput, 1, pResult );
}

void CMonitor::GhostRemover( void )
{
	BOOL bResult = FALSE;
	TCHAR* pstrQuery = "What is the ghost client ID for removal? ";
	DWORD dwWritten = 0;
	DWORD dwRead = 0;

	bResult = WriteFile( GetStdHandle( STD_OUTPUT_HANDLE ), pstrQuery, strlen( pstrQuery ), &dwWritten, NULL );
	if( bResult == FALSE )
	{
		Log( _T( "Failed to write a sentence" ) );
		return;
	}

	bResult = ReadFile( GetStdHandle( STD_INPUT_HANDLE ), m_strTarget, MAX_TEMPCOMMAND_SIZE, &dwRead, NULL );
	if( bResult == FALSE )
	{
		Log( _T( "Failed to read a ghost client ID." ) );
		InitTarget();
		return;
	}

	m_strTarget[dwRead - 2] = '\0';  // CR + LF 贸府
}

void CMonitor::WriteText( TCHAR* msg, bool type )
{
	DWORD lenout;

	::WriteConsole( m_hOut, msg, lstrlen(msg), &lenout, 0 );
	
	if( type == true )
		::WriteConsole( m_hOut, "\n", 1, &lenout,0 );

	return;
}

void CMonitor::LogToScreen( int type, TCHAR* logmsg, ... )
{
	va_list vargs;
	struct tm *now;
	time_t nowTime;

	int year, mon, day;
	static int log_year = 0, log_mon = 0, log_day = 0;
	int hour, min, sec;

	TCHAR LogIdentifier[NUM_OF_LOG_TYPE] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };
	TCHAR buf[(MAX_LOG_LENGTH*10)+1] = {0, };
	static TCHAR szLogFileName[80+1] = {0, };

	// Argument Processing
	va_start( vargs, logmsg );

	// Get nowtime
	time( &nowTime );
	now = localtime(&nowTime);

	// Make it usable.
	year = now->tm_year + 1900;
	mon  = now->tm_mon + 1;
	day  = now->tm_mday;
	hour = now->tm_hour;
	min  = now->tm_min;
	sec  = now->tm_sec;

	// Lock...
	EnterCriticalSection( &m_criLog );

	if( log_year && ( (log_year != year) || (log_mon != mon) || (log_day != day) ) )
	{
		// Close fpLog
		fclose( m_fpLog );
		m_fpLog = NULL;

		// Clear log_year
		log_year = 0;
	}

	if( log_year == 0 || !m_fpLog )
	{
		// Set log_year, log_mon, log_day.
		log_year = year;
		log_mon = mon;
		log_day = day;

		wsprintf( szLogFileName, _T(".\\%d-%02d-%02d.log"), year, mon, day );

		m_fpLog = _tfopen( szLogFileName, _T("a") );
		if( !m_fpLog )
		{
			// Notify ERROR
			wsprintf( buf, _T("FATAL ERROR at Log() :: Can't open LogFile('%s')"), szLogFileName );
			WriteText( buf );
			goto lb_Exit;
		}
	}

	// Write Log Type
	buf[0] = LogIdentifier[type];
	buf[1] = ' ';

	// Write Log rised time.
//	wsprintf( buf+2, "<%02d:%02d:%02d> ", hour, min, sec );

	// Write Log's Body.
	if( _tcslen( logmsg ) > (MAX_LOG_LENGTH-2-11) )
	{
		// Self-calling.
		Log( LOG_FATAL, "Map() Too long string - This log will be lost" );
		va_end( vargs );
		goto lb_Exit;
	}

//	vsprintf( buf+2+11, logmsg, (vargs) );
	_vstprintf( buf+2, logmsg, (vargs) );
	
	// Now Log it. To Screen
	switch( type )
	{
		case LOG_JUST_DISPLAY:	SetConsoleTextAttribute(m_hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE );	break;
		case LOG_JUST_DISPLAY1:	SetConsoleTextAttribute(m_hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE ); break;
		default:	SetConsoleTextAttribute(m_hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_INTENSITY);  break;
	}

	WriteText( buf, false );

	// To File
	if( m_fpLog && (LOG_JUST_DISPLAY > type) )
	{
		lstrcat( buf, _T("\n") );
		_fputts( buf, m_fpLog );
		
		// this can make server to slow.
		fflush( m_fpLog );
	}

lb_Exit:
	LeaveCriticalSection( &m_criLog );

	// Finish Func
	va_end( vargs );
	return;
}

END_NAMESPACE