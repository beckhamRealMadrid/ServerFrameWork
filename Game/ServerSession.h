#pragma once

#include "Session.h"

class CServerTable;

class CServerSession : public CSession
{
	DECLARE_MESSAGEMAP()
public:
					CServerSession( CSessionManager* pSM );
	virtual			~CServerSession(void);
	virtual	void	Close( int nError, int nDetail );
	virtual void	PostCreate();
};