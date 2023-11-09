#include "StdAfx.h"
#include "ChannelSession.h"
#include "GameService.h"
#include "User.h"

CChannelSession::CChannelSession( CSessionManager* pSM ) : CAutoConnSession( pSM )
{

}

CChannelSession::~CChannelSession(void)
{

}

void CChannelSession::PostCreate()
{
	CSession::PostCreate();	
}

BEGIN_MESSAGEMAP( CChannelSession )	
END_MESSAGEMAP()