#include "stdafx.h"
#include "PrivateProfile.h"

START_NAMESPACE

CPrivateProfile::CPrivateProfile( void )
{
}

CPrivateProfile::~CPrivateProfile( void )
{
}

bool CPrivateProfile::IsPrivateProfilePath( void )
{
	if( strcmp( m_szFilePath, "" ) == 0 )
	{
		char szBuffer[256];

		ZeroMemory( szBuffer, sizeof( szBuffer ) );
		sprintf( szBuffer, "'*.ini' file not found." );
		OutputDebugString( szBuffer );

		return false;
	}

	return true;
}

// '*.ini' file의 경로 설정
bool CPrivateProfile::SetPrivateProfilePath( LPCTSTR lpFileName )
{
	assert( lpFileName );

	DWORD nResult;
	char szBuffer[128],
		 szDrive[_MAX_DRIVE] = "",
		 szDirectory[_MAX_DIR] = "",
		 szPreFileName[_MAX_FNAME] = "",
		 szExtension[_MAX_EXT] = "";

	// File명 검사
	_splitpath( lpFileName, szDrive, szDirectory, szPreFileName, szExtension );
	if( strcmp( "", szPreFileName ) == 0 || strcmp( "", szExtension ) == 0 )  // 입력된 값이 있는가?
	{
		ZeroMemory( szBuffer, sizeof( szBuffer ) );
		sprintf( szBuffer, "[Error:] Invalid parameter in SetPrivateProfilePath( lpFileName )\n" );
		OutputDebugString( szBuffer );

		return false;
	}
	if( strcmp( ".ini", _strlwr( szExtension ) ) != 0 )  // *.ini인가?
	{
		ZeroMemory( szBuffer, sizeof( szBuffer ) );
		sprintf( szBuffer, "[Error:] Invalid file extension\n" );
		OutputDebugString( szBuffer );

		return false;
	}

	// File 경로 지정
	ZeroMemory( szDirectory, _MAX_DIR );
	nResult = GetCurrentDirectory( _MAX_DIR, szDirectory );
	if( nResult == 0 )
	{
		ZeroMemory( szBuffer, sizeof( szBuffer ) );
		sprintf( szBuffer,
				 "[Error:%d] Failed to GetCurrentDirectory( _MAX_DIR, szDirectory )\n",
				 GetLastError() );
		OutputDebugString( szBuffer );

		return false;
	}

	// File명 구성
	ZeroMemory( m_szFilePath, _MAX_PATH );
	sprintf( m_szFilePath, "%s\\%s%s", szDirectory, szPreFileName, szExtension );

	return true;
}

// 설정된 '*.ini' file에서 Section(lpAppName)내에 Key(lpKeyName)에 할당된 정수 값 추출
UINT CPrivateProfile::GetProfileInt( LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault )
{
	IsPrivateProfilePath();

	return GetPrivateProfileInt( lpAppName, lpKeyName, nDefault, m_szFilePath );;
}

// 설정된 '*.ini' file에서 Section(lpAppName)내에 Key(lpKeyName)에 할당된 문자열 값 추출
DWORD CPrivateProfile::GetProfileString( LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize )
{
	IsPrivateProfilePath();

	return GetPrivateProfileString( lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, m_szFilePath );
}

END_NAMESPACE
