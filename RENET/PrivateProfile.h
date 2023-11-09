#pragma once

START_NAMESPACE

class RENET_API CPrivateProfile
{
private:
	char m_szFilePath[MAX_PATH];		// '*.ini' file의 경로

	bool IsPrivateProfilePath( void );  // '*.ini' file의 경로 설정 여부

public:
	CPrivateProfile( void );
	~CPrivateProfile( void );

	bool SetPrivateProfilePath( LPCTSTR lpFileName );  
	UINT GetProfileInt( LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault );  
	DWORD GetProfileString( LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize );  
};

END_NAMESPACE
