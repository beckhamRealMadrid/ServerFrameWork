#pragma once

#include "Session.h"

class CServerSession : public CSession
{
	DECLARE_MESSAGEMAP()
public:
					CServerSession( CSessionManager* pSM );
	virtual			~CServerSession(void);
	virtual bool	Create( SOCKET hSocket );
protected:
			void	OnLogInReq( CMsgRecv& msg );
};