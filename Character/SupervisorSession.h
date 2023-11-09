#pragma once

#include "AutoConnSession.h"
#include "Singleton.h"

class CSupervisorSession : public CAutoConnSession, public CSingleton < CSupervisorSession >
{
	DECLARE_MESSAGEMAP()
public:
					CSupervisorSession( CSessionManager* pSM );
	virtual			~CSupervisorSession(void);
protected:
	virtual void	PostCreate();
protected:
			void	OnAttachAck( CMsgRecv& msg );
};

#define SUPERVISOR_SESSION	CSupervisorSession::GetInstance()
