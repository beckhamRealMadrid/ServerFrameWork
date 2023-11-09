#pragma once

#include "Service.h"
#include "Singleton.h"
#include "SupervisorSession.h"
#include "UserManagerSession.h"
#include "IDGenerator.h"

class CGatewayService : public CService, public CSingleton <CGatewayService>
{
public:
					CGatewayService( bool bService = true );
	virtual			~CGatewayService(void);
	virtual bool	InitDatabase();
	virtual void	DestroyDatabase();
	virtual bool	InitNetwork();
	virtual void	DestroyNetwork();
	virtual bool	InitApplication();
	virtual void	DestroyApplication();	
	virtual void	WaitForTerminateSignal();
	static	void	FuncForException( LPTSTR szFileName );			
protected:
			CSessionManager*	m_pUserSessionManager;			
			CSupervisorSession*	m_pSupervisorSession;
			CUserManagerSession*	m_UserManagerSession;
};

#define GATEWAYSERVICE	CGatewayService::GetInstance()