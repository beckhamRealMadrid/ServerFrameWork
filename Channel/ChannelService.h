#pragma once

#include "Service.h"
#include "Singleton.h"
#include "SupervisorSession.h"

class CChannelService : public CService, public CSingleton <CChannelService>
{
public:
								CChannelService( bool bService = true );
	virtual						~CChannelService(void);
	virtual bool				InitDatabase();
	virtual void				DestroyDatabase();	
	virtual bool				InitNetwork();
	virtual void				DestroyNetwork();	
	virtual bool				InitApplication();
	virtual void				DestroyApplication();	
	virtual void				WaitForTerminateSignal();			
protected:
			int					m_nAcceptServerPort; 
			CSessionManager*	m_pUserSessionManager;
			CSessionManager*	m_pServerSessionManager;	
			CSupervisorSession*	m_pSupervisorSession;	
};

#define CHANNELSERVICE	CChannelService::GetInstance()
