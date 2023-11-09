#pragma once

#include "Service.h"
#include "Singleton.h"
#include "SupervisorSession.h"

class CCharacterService : public CService, public CSingleton <CCharacterService> 
{
public:
								CCharacterService( bool bService = true );
	virtual						~CCharacterService(void);
	virtual bool				InitDatabase();
	virtual void				DestroyDatabase();
	virtual bool				InitNetwork();
	virtual void				DestroyNetwork();
	virtual bool				InitApplication();
	virtual void				DestroyApplication();	
	virtual void				WaitForTerminateSignal();
	static	void				FuncForException( LPTSTR szFileName );			
protected:
			CSessionManager*	m_pUserSessionManager;
			CSupervisorSession*	m_pSupervisorSession;			
};

#define CHARACTERSERVICE	CCharacterService::GetInstance()
