#pragma once

#include "Singleton.h"
#include "SectionObject.h"
#include "Registry.h"
#include "SessionManager.h"
#include <Iphlpapi.h>

START_NAMESPACE

#define SERVICE_NAME_FILE	_T("Service.dat")
#define SERVICE_NAME_LENGTH	20
#define SERVICE_REGKEY_NAME _T("SYSTEM\\CurrentControlSet\\Services\\%s\\Rhoceo")
#define MAX_CAPACITY		1000

#ifndef DECODE_KEY
#	define DECODE_KEY		"ÎýÜµâ³ô¸î¦,ÍéëâØ¿Ø¿á¨"
#endif

#ifndef DECODE_SUBKEY
#	define DECODE_SUBKEY	2
#endif

class CIOCPManager;

class RENET_API CService
{
public:	
							CService( bool bService = true );
	virtual					~CService();
	virtual int				Start( int argc, TCHAR* argv[] );
	virtual int				Start( HINSTANCE hInstance, LPCTSTR strCmdLine );
	virtual int				SetRegistryValues( CRegistry& reg );
	virtual void			WaitForTerminateSignal();
	virtual void			AddEventLog( const TCHAR *strFormat, ... );
	virtual bool			InitDatabase()			{ return true; }
	virtual bool			InitNetwork();
	virtual bool			InitApplication(); 
	virtual void			OnStopService();
	virtual void			DestroyApplication()	{}
	virtual void			DestroyNetwork();
	virtual void			DestroyDatabase()		{}	
	static	void WINAPI		ServiceHandler( unsigned long dwControl );
	static	void WINAPI		ServiceMain( unsigned long dwArgc, TCHAR **lpszServiceName );
			int				RemoveService( const TCHAR* szServiceName );
			int				RegisterService( const TCHAR* szServiceName );
			int				StartService();
			int				StopService();	
	const	HINSTANCE		GetHInstance()				{ return m_hInstance; }
	const	LPCTSTR 		GetShardName()				{ return m_strShardName; }
	const	LPCTSTR 		GetServiceName()			{ return m_strServiceName; }
	const	LPCTSTR 		GetServerIP()				{ return m_strServerIP; }
	const	LPCTSTR 		GetPath()					{ return m_strPath; }
	const	LPCTSTR			GetExceptionStackDumpPath()	{ return m_strExceptionStackDumpPath; }
	const	int				GetServerPort()				{ return m_nAcceptPort; }
protected:
	static	void			GetLocalIP( TCHAR *ip );
	static	void			GetModuleFile( TCHAR *szDir );
			void			GetNetworkInterfaceEntry();
			void			GetNetworkInterfaceInfo();
			void			GetNetworkAdaptersInfo();
protected:
			SERVICE_STATUS_HANDLE m_hSS;
			TCHAR				m_strShardName[256];
			TCHAR				m_strServiceName[256];
			TCHAR				m_strServerIP[256];
			TCHAR				m_strMasterID[256];
			TCHAR				m_strPath[256];
			TCHAR				m_strExceptionStackDumpPath[256];
			int					m_nAcceptPort;	
			HANDLE				m_hProcessIdentify;
			HANDLE				m_hKillService;
			CRegistry			m_Registry;
			SectionObject		m_sectionEventLog;			
			HINSTANCE			m_hInstance;
			PIP_ADAPTER_INFO	m_pAdapterInfo;
			PIP_INTERFACE_INFO	m_pInterfaceInfo;
protected:
			bool				m_bService;
			CIOCPManager*		m_pIOCPManager;
};

extern RENET_API CService* g_pServiceModule;

DWORD RENET_API DecodeCDBData( char* szLoadFile, void* pReturnValue, char* szDecodeKey = DECODE_KEY, int nDecodeSubKey = DECODE_SUBKEY );
LONG WINAPI UnhandledExceptionHandler( _EXCEPTION_POINTERS* pExceptionInfo );

END_NAMESPACE