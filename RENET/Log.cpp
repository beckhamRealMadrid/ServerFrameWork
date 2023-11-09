#include "StdAfx.h"
#include "Log.h"
#include "Service.h"
#include "Monitor.h"

START_NAMESPACE

void Log( const TCHAR *strFormat, ... )
{
	va_list args;

	struct tm when;
	__time64_t now;

	_time64( &now );
	when = *_localtime64( &now );

#ifndef _DEBUG
	char strLogFile[256];
	::StringCbPrintf( strLogFile, 256, _T("c:\\%s %04d-%02d-%02d.log"), g_pServiceModule->GetServiceName(), when.tm_year + 1900, when.tm_mon + 1, when.tm_mday );

	DWORD dwWritten;
	HANDLE hFile = ::CreateFile( strLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL ); 
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		::SetFilePointer( hFile, 0, 0, SEEK_END );
#endif
		TCHAR strTemp[512], strTemp2[512];

		va_start( args, strFormat );
		::StringCbVPrintf( strTemp, 512, strFormat, args );
		va_end( args );

		::StringCbPrintf( strTemp2,	512, _T("%4d-%2d-%2d %2d:%2d:%2d : %s\r\n"), when.tm_year + 1900, when.tm_mon + 1, when.tm_mday, when.tm_hour, when.tm_min, when.tm_sec, strTemp );

#ifdef _DEBUG
		LOG_TO_SCREEN( LOG_JUST_DISPLAY1, strTemp2 );
#endif

#ifdef _DEBUG
		OutputDebugString(strTemp2);
#else
		::WriteFile(hFile, strTemp2, strlen(strTemp2), &dwWritten, NULL);
		SAFE_CLOSEHANDLE(hFile);
	}
#endif
}

DWORD g_dwThreadCount = 0;
LPCTSTR g_szThreadName[] =
{
	_T("John Depp"),
	_T("Daniel Henney"),
	_T("Vivianse"),
	_T("Meg Ryan"),
	_T("Nicolas Cage"),
	_T("Renee Zellweger"),
	_T("Demi Moore"),
    _T("Mel Gibson"),
	_T("Olivia Hussey"),
	_T("Brad Pitt"),
	_T("Leonardo DiCaprio"),
	_T("Dakota Fanning"),
	_T("Gwyneth Paltrow")
};

void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName )
{
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType;		// must be 0x1000
		LPCSTR szName;		// pointer to name (in user addr space)
		DWORD dwThreadID;	// thread ID (-1=caller thread)
		DWORD dwFlags;		// reserved for future use, must be zero
	} THREADNAME_INFO;

	THREADNAME_INFO info;
	{
		info.dwType		= 0x1000;
		info.szName		= szThreadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags	= 0;
	}
	__try
	{
		RaiseException( 0x406D1388, 0, sizeof(info) / sizeof(DWORD), (DWORD*)&info );
		g_dwThreadCount++;
	}
	__except ( EXCEPTION_CONTINUE_EXECUTION )
	{
		
	}
}

END_NAMESPACE