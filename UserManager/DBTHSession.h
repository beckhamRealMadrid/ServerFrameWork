#pragma once

#include "Singleton.h"
#include "THOleDBSource.h"

#define CLOSE_THREAD	0
#define DB_RESULT		1

class CDBTHSession : public CSingleton <CDBTHSession>  
{
public:
								CDBTHSession(void);
	virtual						~CDBTHSession(void);
	static	void				ReceivedFromDB( DBRECEIVEDATA* pResult );
	static	void				DisplayDBMessage( char* szMsg );
	static	void				DisplayDBReport( char* szMsg );
			bool				Create( LPCSTR szIP, LPCSTR szCatalog, LPCSTR szUserID, LPCSTR szPasswd );
			CTHOleDBSource*		GetTHOleDBSource()	{ return m_pTHOleDBSource; }
	const	PHANDLE				GetEventHandle()	{ return m_hEvent; }
protected:
	static	unsigned __stdcall	WorkerThread( void *lpParamaeter );
private:
			CTHOleDBSource*		m_pTHOleDBSource;
			HANDLE				m_hThread;
			HANDLE				m_hEvent[2];
};

#define DBTHSESSION		CDBTHSession::GetInstance()
#define TH_OLEDB		CDBTHSession::GetInstance()->GetTHOleDBSource()
	