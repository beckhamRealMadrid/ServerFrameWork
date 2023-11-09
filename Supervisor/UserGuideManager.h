#pragma once

#include "Singleton.h"
#include "Synchronized.h"
#include "ChunkAllocator.h"

#define USER_DIR_TIMEOUT	( 1000 * 30 )	// 30 seconds

class CServerSession;

struct SUserDirection
{
	std::string	m_strServerAddr;
	int			m_nServerPort;
	DWORD		m_dwTickCreated;
};

class CUserGuideManager : public CSingleton <CUserGuideManager>
{
public:
									CUserGuideManager(void);
	virtual							~CUserGuideManager(void);
			SUserDirection*			GetUserDirection( LPCTSTR strUserID );
			void					AddUserDirection( LPCTSTR strUserID, LPCTSTR strServerAddr, int nPort );
			void					RemoveTimedOutUser();
private:
	typedef std::map <std::string, SUserDirection*> mapUserDir;
	typedef mapUserDir::iterator					imapUserDir;
	typedef CChunkAllocatorMT<SUserDirection>		AllocatorUserDirection;

			mapUserDir				m_mapUserDirection;
			SectionObject			m_soUserDirection;
			AllocatorUserDirection	m_UserDirectionPool;
};

#define USER_GUIDE_MANAGER	CUserGuideManager::GetInstance()	