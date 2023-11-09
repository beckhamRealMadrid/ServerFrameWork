#pragma once

#include "Singleton.h"
#include "atlaux.h"
#include "Msg.h"
#include "ObjMap.h"
#include "SectionObject.h"

#pragma pack(push,1)
class CChannelInfo
{
public:
	CChannelInfo(void)	{}
	~CChannelInfo(void)	{}

	DWORD	m_dwChannelID;
	TCHAR	m_szChannelName[50];
	DWORD	m_dwServerID;
	BYTE	m_byChannelType;
	BYTE	m_byZoneType;
	DWORD	m_dwMaxUser;
	BYTE	m_byMinLevel;
	BYTE	m_byMaxLevel;
};

class CChannelInfoEx : public CChannelInfo
{
public:
	CChannelInfoEx(void)	{}
	~CChannelInfoEx(void)	{}

	DWORD	m_dwCurUserNum;
};
#pragma pack(pop)

class CChannelTable : public CSingleton <CChannelTable>, public CAuxStdThunk<CChannelTable> 
{
public:
								CChannelTable(void);
	virtual						~CChannelTable(void);
			bool				RegisterChannelInfo( DWORD dwID, CChannelInfoEx* pChannelInfoEx );
			CChannelInfoEx*		GetObjMapChannelInfo( DWORD dwID )	{ return m_ObjMapChannelInfos.GetObj( dwID ); }
			void				MakeChannelState( CMsgSend& msg );
private:
	typedef CObjMap< CChannelInfoEx, TRUE, stdext::hash_map<DWORD, CChannelInfoEx*> > ObjMapChannelInfos;	
			ObjMapChannelInfos	m_ObjMapChannelInfos;	// 디비에서 읽어올 채널 정보
};

#define CHANNEL_TABLE CChannelTable::GetInstance()