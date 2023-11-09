#pragma once

#include "Singleton.h"
#include "Session.h"

class CProxyManager : public CSingleton <CProxyManager>
{
public:
						CProxyManager(void);
	virtual				~CProxyManager(void);
			CSession*	PopProxy();
			void		PushProxy( CSession* pSess );

	template <typename S>
	void CreateProxys( int nCapacity )
	{
		S::InitMessageMap();

		for ( int i = 0; i < nCapacity; i++ )
			m_listInActive.push_back( new S( NULL ) );			
	};

	template <typename S>
	void RemoveProxys()
	{
		sesslistitor itor = NULL;
		for ( itor = m_listActive.begin(); itor != m_listActive.end(); ++itor )
			delete (S*)(*itor);
		
		for ( itor = m_listInActive.begin(); itor != m_listInActive.end(); ++itor )
			delete (S*)(*itor);
	};
protected:
	typedef std::list < CSession* > sesslist;
	typedef std::list < CSession* >::iterator sesslistitor;
			SectionObject		m_soList;
			sesslist			m_listInActive;
			sesslist			m_listActive;
};

#define PROXY_MANAGER CProxyManager::GetInstance()