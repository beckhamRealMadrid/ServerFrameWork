#include "StdAfx.h"
#include "ProxyManager.h"
#include "IOCPManager.h"

CProxyManager::CProxyManager(void)
{
	m_listActive.clear();
	m_listInActive.clear();
}

CProxyManager::~CProxyManager(void)
{
	m_listActive.clear();
	m_listInActive.clear();
}

CSession* CProxyManager::PopProxy()
{
	CSession* pSession = NULL;

	Synchronized sync( &m_soList );

	if ( m_listInActive.size() > 0 )
	{
		pSession = m_listInActive.front();
		m_listInActive.pop_front();
	}
	else
	{
		Log( _T("!!!! Proxy Pool Underfloor !!!!") );
		return NULL;
	}

	m_listActive.push_back( pSession );

	return pSession;
}

void CProxyManager::PushProxy( CSession* pSess )
{
	Synchronized sync( &m_soList );

	sesslistitor itor = std::find( m_listActive.begin(), m_listActive.end(), pSess );
	if ( itor != m_listActive.end() )
	{
//		IOCP->PostCompletion( (DWORD)pSess, NULL );

		m_listInActive.push_back( *itor );
		m_listActive.erase( itor );
	}
}