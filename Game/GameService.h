#pragma once

#include "Service.h"
#include "Singleton.h"
#include "ConnectionManager.h"
#include "SupervisorSession.h"
#include "ChannelSession.h"

class CGameService : public CService, public CSingleton <CGameService>
{
public:
								CGameService( bool bService = true );
	virtual						~CGameService(void);
	virtual bool				InitDatabase();
	virtual void				DestroyDatabase();	
	virtual bool				InitNetwork();
	virtual void				DestroyNetwork();	
	virtual bool				InitApplication();
	virtual void				DestroyApplication();	
	virtual void				WaitForTerminateSignal();
			int					GetAcceptUDPPort()					{ return m_nAcceptUDPPort; }
protected:
			int					m_nAcceptServerPort;
			int					m_nAcceptUDPPort;
			CSessionManager*	m_pUserSessionManager;
			CConnectionManager*	m_pConnectionManager;
			CSupervisorSession*	m_pSupervisorSession;
			CChannelSession*	m_pChannelSession;			
};

#define GAMESERVICE	CGameService::GetInstance()