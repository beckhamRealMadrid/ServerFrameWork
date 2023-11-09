#pragma once

#include "Synchronized.h"
#include "IDGenerator.h"
#include "DBStruct.h"

START_NAMESPACE

class COleDBSession;
class CThreadSafeStaticMemPool;

class RENET_API COleDBSource
{
public:
								COleDBSource();
	virtual						~COleDBSource();
	virtual	bool				Create( LPCSTR szIP, LPCSTR szCatalog, LPCSTR szUserID, LPCSTR szPasswd );
	virtual	BOOL				InitDBModule( DB_INITIALIZE_DESC* desc )	{ return FALSE; }
	virtual	COleDBSession*		ReserveNewOleDBSession( DWORD dwThreadID );
			COleDBSession*		GetOleDBSession();
			COleDBSession*		GetOleDBSession( DWORD dwThreadID );
			void				Destroy();
				
			char*				GetIP()			{ return m_szIP; }	
			char*				GetCatalog()	{ return m_szCatalog; }	
			char*				GetUserID()		{ return m_szUserID; }
			char*				GetPasswd()		{ return m_szPasswd; }
			DWORD				GetOLEDBSize()	{ return m_dwOLEDBSize; }
			DB_INITIALIZE_DESC& GetDesc()		{ return m_InitDesc; }	

			CThreadSafeStaticMemPool*	GetOutputMsgPool()	{ return m_pOutputMsgPool; }
			CThreadSafeStaticMemPool*	GetInputMsgPool()	{ return m_pInputMsgPool; }
			CThreadSafeStaticMemPool*	GetBindingPool()	{ return m_pBindingPool; }
			CThreadSafeStaticMemPool*	GetParamPool()		{ return m_pParamPool; }
			CThreadSafeStaticMemPool*	GetResultPool()		{ return m_pResultPool; }
protected:
	typedef std::map <DWORD, COleDBSession*> mapOleDBSession;
	typedef std::map <DWORD, COleDBSession*>::iterator imapOleDBSession;

			mapOleDBSession		m_mapOleDBSession;
			SectionObject		m_soOleDBSession;
			char				m_szIP[256];
			char				m_szCatalog[256];
			char				m_szUserID[256];
			char				m_szPasswd[256];
			CIDPool				m_IDPool;

			DWORD				m_dwOLEDBSize;
			DB_INITIALIZE_DESC	m_InitDesc;

			CThreadSafeStaticMemPool*	m_pInputMsgPool;
			CThreadSafeStaticMemPool*	m_pOutputMsgPool;
			CThreadSafeStaticMemPool*	m_pResultPool;
			CThreadSafeStaticMemPool*	m_pBindingPool;
			CThreadSafeStaticMemPool*	m_pParamPool;
};

END_NAMESPACE