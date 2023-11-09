#pragma once

#include "AutoConnSession.h"
#include "Singleton.h"

class CUserManagerSession :	public CAutoConnSession, public CSingleton < CUserManagerSession >
{
	DECLARE_MESSAGEMAP()
public:
					CUserManagerSession(void);
	virtual			~CUserManagerSession(void);
protected:
	virtual void	PostCreate();
protected:
			void	OnLoginAck( CMsgRecv& msg );
};

#define USERMANAGER_SESSION	CUserManagerSession::GetInstance()