#include "StdAfx.h"
#include "UserGuideManager.h"

CUserGuideManager::CUserGuideManager(void)
{
	
}

CUserGuideManager::~CUserGuideManager(void)
{
	
}

SUserDirection* CUserGuideManager::GetUserDirection( LPCTSTR strUserID )
{
	SUserDirection* pDir = NULL;

	Synchronized so( &m_soUserDirection );

	imapUserDir itor = m_mapUserDirection.find( strUserID );
	if ( itor != m_mapUserDirection.end() )
	{
		pDir = itor->second;
		m_mapUserDirection.erase( itor );
	}

	return pDir;
}

void CUserGuideManager::AddUserDirection( LPCTSTR strUserID, LPCTSTR strServerAddr, int nPort )
{
	SUserDirection* pDir = NULL;

	pDir = GetUserDirection( strUserID );
	if ( pDir )
		m_UserDirectionPool.FreeItem( pDir );

	pDir = m_UserDirectionPool.NewItem();
	pDir->m_dwTickCreated = GetTickCount();
	pDir->m_strServerAddr = strServerAddr;
	pDir->m_nServerPort = nPort;

	Synchronized so( &m_soUserDirection );
	m_mapUserDirection.insert( mapUserDir::value_type( strUserID, pDir ) );
}

void CUserGuideManager::RemoveTimedOutUser()
{
	SUserDirection* pDir = NULL;

	Synchronized so( &m_soUserDirection );

	for ( imapUserDir itor = m_mapUserDirection.begin(); itor != m_mapUserDirection.end(); )
	{
		pDir = itor->second;

		if ( GetTickCount() > pDir->m_dwTickCreated + USER_DIR_TIMEOUT )
		{
			itor = m_mapUserDirection.erase( itor );
		}
		else
		{
			itor++;
		}
	}
}