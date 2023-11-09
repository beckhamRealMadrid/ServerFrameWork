#pragma once

#include "Session.h"

class CUser : public CSession
{
	DECLARE_MESSAGEMAP()
public:
					CUser( CSessionManager* pSM );;
	virtual			~CUser(void);
	virtual bool	OnError( DWORD dwErrorCode, int nDetail );
	virtual	void	Close( int nError, int nDetail );
	virtual void	PostCreate();		
};