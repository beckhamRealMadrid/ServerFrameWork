#include "StdAfx.h"
#include "ChannelTable.h"

CChannelTable::CChannelTable(void)
{

}

CChannelTable::~CChannelTable(void)
{

}

bool CChannelTable::RegisterChannelInfo( DWORD dwID, CChannelInfoEx* pChannelInfoEx )
{
	if ( FALSE == m_ObjMapChannelInfos.AddObj( dwID, pChannelInfoEx ) )
		return false;	

	return true;
}

void CChannelTable::MakeChannelState( CMsgSend& msg )
{
	DWORD dwCount = 0;
	ObjMapChannelInfos::iterator it = m_ObjMapChannelInfos.LockIterator(dwCount);

	msg << dwCount;

	for ( DWORD i = 0; i < dwCount; ++i, ++it )
	{
		CChannelInfoEx* pChannelInfoEx = (*it).second;
		msg << pChannelInfoEx->m_dwChannelID << pChannelInfoEx->m_szChannelName << pChannelInfoEx->m_dwCurUserNum;
	}
}