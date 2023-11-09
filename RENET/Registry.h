#pragma once

START_NAMESPACE

class RENET_API CRegistry  
{
public:
					CRegistry();
	virtual			~CRegistry();
			long	Open( LPCTSTR szReg );
			long	Create( LPCTSTR szReg, DWORD& dwStatus );
			void	Close();
			long	Get( TCHAR *szKey, int &nValue);
			long	Get( TCHAR *szKey, TCHAR *szValue);
			long	Set( const TCHAR *szKey, int nValue );
			long	Set( const TCHAR *szKey, const TCHAR *szValue );
private:
			HKEY	m_hKey;
};

END_NAMESPACE