#pragma once

#include "Service.h"

class CUserManagerService :	public CService
{
public:
					CUserManagerService( bool bService = true );
	virtual			~CUserManagerService(void);
	virtual bool	InitDatabase();
	virtual bool	InitNetwork();
	virtual void	DestroyDatabase();
	virtual void	DestroyNetwork();
	virtual void	WaitForTerminateSignal();

			CSessionManager*	GetUserSessionManager()	{ return m_pUserSessionManager; }
protected:
			CSessionManager*	m_pUserSessionManager;
};