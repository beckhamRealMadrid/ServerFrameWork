#pragma once

#define	MAX_ANSI_BUFF		2048
#define MAX_UNICODE_BUFF	2048

inline LPCSTR  Convert2Multi(LPCSTR szSource)	{return szSource;}
inline LPCWSTR Convert2Wide(LPCWSTR wszSource)	{return wszSource;}

inline LPCSTR Convert2Multi(LPCWSTR wszSource)
{
	static char szBuf[MAX_ANSI_BUFF];
	memset(szBuf, 0, sizeof(szBuf));
	
	WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK, wszSource, wcslen(wszSource), szBuf, MAX_ANSI_BUFF, NULL, NULL );
	return szBuf;
}

inline LPCWSTR Convert2Wide(LPCSTR szSource)
{
	static WCHAR wszBuf[MAX_UNICODE_BUFF];
	memset(wszBuf, 0, sizeof(wszBuf));

	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szSource, strlen(szSource), wszBuf, MAX_UNICODE_BUFF );
	return wszBuf;
}

#ifdef _UNICODE
#	define CODE_CONVERT_WIDE(x)	Convert2Wide(x)
#	define CODE_CONVERT_ANSI(x)	Convert2Multi(x)
#else
#	define CODE_CONVERT_WIDE(x)	x	
#	define CODE_CONVERT_ANSI(x)	x
#endif

// 다국어 지원을 위해 인라인 문자열 비교함수를 제공한다
inline int RENET_API __strcmp(char* szString1, char* szString2)
{
	return (CompareString(LOCALE_USER_DEFAULT, 0, szString1, lstrlen(szString1), szString2, lstrlen(szString2)) - 2);
}

inline int RENET_API __stricmp(char* szString1, char* szString2)
{
	return (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE|NORM_IGNOREKANATYPE|NORM_IGNOREWIDTH, szString1, lstrlen(szString1), szString2, lstrlen(szString2)) - 2);
}

inline int RENET_API __strncmp(char* szString1, char* szString2, int nLength)
{
	return (CompareString(LOCALE_USER_DEFAULT, 0, szString1, nLength, szString2, nLength) - 2);
}

inline int RENET_API __strnicmp(char* szString1, char* szString2, int nLength)
{
	return (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE|NORM_IGNOREKANATYPE|NORM_IGNOREWIDTH, szString1, nLength, szString2, nLength) - 2);
}

inline LPSTR RENET_API __lstrcpyn(LPSTR lpString1, LPSTR lpString2, int iMaxLength)
{
	return lstrcpyn(lpString1, lpString2, iMaxLength+1);
}

inline bool RENET_API IsEmptyString(char* szString)
{
	return (lstrlen(szString) == 0);
}

inline void RENET_API Convert16To2String( char *pch16 , char *pch2 )
{
	int ilen = strlen( pch16 );
	int iNum = 0;

	for( int i = 0 ; i < ilen ; i++ )
	{
		if( pch16[i] >= '0' && pch16[i] <= '9' )
		{
			iNum = iNum * 16 + ( pch16[i] - '0' );
		}
		else if( pch16[i] >= 'a' && pch16[i] <= 'f' )
		{
			iNum = iNum * 16 + ( pch16[i] - 'a' + 10 );
		}
		else if( pch16[i] >= 'A' && pch16[i] <= 'F' )
		{
			iNum = iNum * 16 + ( pch16[i] - 'A' + 10 );
		}
	}

	int idigit = 0;
	int ipch = 0;

	do
	{
		idigit = iNum % 2;
		iNum = iNum / 2;
		pch2[ipch++] = idigit  ? '1' : '0';
	} while( iNum );

	if( iNum )
	{
		pch2[ipch++] = '1';
	}

	pch2[ipch] = '\0';

	char chtmp;

	ilen = ipch >> 1;

	for( int i = 0 ; i < ilen ; i++ )
	{
		chtmp = pch2[i];
		pch2[i] = pch2[ipch-i-1];
		pch2[ipch-i-1] = chtmp;
	}
}