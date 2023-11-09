#pragma once

#include "AutoConnSession.h"
#include "Singleton.h"

class CChannelSession : public CAutoConnSession, public CSingleton < CChannelSession >
{
	DECLARE_MESSAGEMAP()
public:
					CChannelSession( CSessionManager* pSM );
	virtual			~CChannelSession(void);
protected:
	virtual void	PostCreate();
};

#define CHANNEL_SESSION	CChannelSession::GetInstance()
