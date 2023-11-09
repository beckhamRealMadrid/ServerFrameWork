#include "stdafx.h"
#include "Registry.h"

START_NAMESPACE

CRegistry::CRegistry( )
{
	m_hKey = NULL;
}

CRegistry::~CRegistry()
{
	Close();
}

long CRegistry::Open( LPCTSTR szReg )
{
	Close();
	return RegOpenKeyEx( HKEY_LOCAL_MACHINE, szReg, 0, KEY_ALL_ACCESS, &m_hKey );
}

long CRegistry::Create( LPCTSTR szReg, DWORD& dwStatus )
{
	Close();
	return RegCreateKeyEx( HKEY_LOCAL_MACHINE, szReg, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &m_hKey, &dwStatus );
}

void CRegistry::Close()
{
	if ( m_hKey != NULL )
		RegCloseKey( m_hKey );

	m_hKey = NULL;
}

long CRegistry::Get( TCHAR *szKey, int &nValue )
{
	DWORD dwType, dwLen;

	if ( RegQueryValueEx( m_hKey, szKey , NULL, &dwType, (LPBYTE)&nValue, &dwLen ) == ERROR_SUCCESS )
	{
		if ( dwType == REG_DWORD ) return ( 0 );
	}

	return ( -1 );
}

long CRegistry::Get( TCHAR *szKey, TCHAR *szValue )
{
	DWORD dwType, dwLen;

	if ( RegQueryValueEx( m_hKey, szKey , NULL, &dwType, (LPBYTE)szValue, &dwLen ) == ERROR_SUCCESS )
	{
		if ( dwType == REG_SZ ) return ( 0 );
	}

	return ( -1 );
}

long CRegistry::Set( const TCHAR *szKey, int nValue )
{
	if ( RegSetValueEx(m_hKey, szKey, NULL, REG_DWORD, (BYTE*)&nValue, sizeof( DWORD ) ) == ERROR_SUCCESS )
		return ( 0 );

	return ( -1 );
}

long CRegistry::Set( const TCHAR *szKey, const TCHAR *szValue )
{
	if ( RegSetValueEx(m_hKey, szKey, NULL, REG_SZ, (BYTE*)szValue, _tcslen( szValue ) ) == ERROR_SUCCESS )
		return ( 0 );

	return ( -1 );
}

END_NAMESPACE