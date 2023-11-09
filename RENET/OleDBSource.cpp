#include "stdafx.h"
#include "OleDBSource.h"
#include "OleDBSession.h"
#include "ThreadSafeStaticMemPool.h"

COleDBSource::COleDBSource()
{
	CoInitialize(NULL);

	m_IDPool.Create();

	ZeroMemory( &m_InitDesc, sizeof(m_InitDesc) );
	m_dwOLEDBSize = 0;

	m_pInputMsgPool		= NULL;
	m_pOutputMsgPool	= NULL;
	m_pParamPool		= NULL;
	m_pResultPool		= NULL;		// 결과값 Buffer 생성용 
	m_pBindingPool		= NULL;		// DBBINDING 생성용
}

COleDBSource::~COleDBSource()
{
	Destroy();

	CoUninitialize();
}

bool COleDBSource::Create( LPCSTR szIP, LPCSTR szCatalog, LPCSTR szUserID, LPCSTR szPasswd )
{
	::StringCchCopyA( m_szIP, 256, szIP );
	::StringCchCopyA( m_szCatalog, 256, szCatalog );
	::StringCchCopyA( m_szUserID, 256, szUserID );
	::StringCchCopyA( m_szPasswd, 256, szPasswd );

	return true;
}

void COleDBSource::Destroy()
{
	Synchronized sa(&m_soOleDBSession);

	COleDBSession* pDB = NULL;
	for ( imapOleDBSession itor = m_mapOleDBSession.begin(); itor != m_mapOleDBSession.end(); ++itor )
	{
		pDB = (*itor).second;
		delete pDB;		// FOR OLE DB
	}

	m_mapOleDBSession.clear();

	SAFE_DELETE( m_pInputMsgPool );
	SAFE_DELETE( m_pOutputMsgPool );		
	SAFE_DELETE( m_pParamPool );
	SAFE_DELETE( m_pResultPool );
	SAFE_DELETE( m_pBindingPool );	
}

COleDBSession* COleDBSource::GetOleDBSession()
{
	Synchronized so(&m_soOleDBSession);

	imapOleDBSession itor = m_mapOleDBSession.find( GetCurrentThreadId() );
	if ( itor != m_mapOleDBSession.end() )
		return (*itor).second;
	else
		return ReserveNewOleDBSession( GetCurrentThreadId() );

/*
	DWORD dwAllocID = m_IDPool.AllocID();
	imapOleDBSession itor = m_mapOleDBSession.find( dwAllocID );
	if ( itor != m_mapOleDBSession.end() )
		return (*itor).second;
	else
		return ReserveNewOleDBSession( dwAllocID );
*/
}

COleDBSession* COleDBSource::GetOleDBSession( DWORD dwThreadID )
{
	Synchronized so(&m_soOleDBSession);

	imapOleDBSession itor = m_mapOleDBSession.find( dwThreadID );
	if ( itor != m_mapOleDBSession.end() )
		return (*itor).second;
	else
		return ReserveNewOleDBSession( dwThreadID );

/*
	DWORD dwAllocID = m_IDPool.AllocID();
	imapOleDBSession itor = m_mapOleDBSession.find( dwAllocID );
	if ( itor != m_mapOleDBSession.end() )
		return (*itor).second;
	else
		return ReserveNewOleDBSession( dwAllocID );
*/
}

COleDBSession* COleDBSource::ReserveNewOleDBSession( DWORD dwThreadID )
{
//	Log( _T("Reserve New DB for Thread ID : %d"), dwThreadID );

	COleDBSession *pOleDBSession = new COleDBSession( &m_InitDesc, this );

	int nRet = pOleDBSession->Create();
	if ( 1 == nRet )
	{
		Synchronized sa( &m_soOleDBSession );

		imapOleDBSession itor = m_mapOleDBSession.find( dwThreadID );
		if (itor != m_mapOleDBSession.end())
		{
			delete (*itor).second;
			m_mapOleDBSession.erase(itor);
		}

		m_mapOleDBSession.insert( mapOleDBSession::value_type( dwThreadID, pOleDBSession ) );

		return pOleDBSession;
	}
	else
	{
		delete pOleDBSession;
		pOleDBSession = NULL;
		return NULL;
	}
}