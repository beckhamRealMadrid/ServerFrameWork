#pragma once

#include "Service.h"
#include "Singleton.h"
#include "Facilities.h"

class CUserGuideManager;

class CSupervisorService : public CService, public CSingleton <CSupervisorService>
{
public:
								CSupervisorService( bool bService = true );
	virtual						~CSupervisorService(void);
	virtual bool				InitDatabase();
	virtual void				DestroyDatabase();
	virtual bool				InitNetwork();
	virtual void				DestroyNetwork();
	virtual bool				InitApplication();
	virtual void				DestroyApplication();	
	virtual void				WaitForTerminateSignal();
	static	void				FuncForException( LPTSTR szFileName );			
protected:
	typedef CScheduledCallbacker<CSupervisorService, DWORD>	Scheduler;

			Scheduler			m_ScheduledJob;
			CUserGuideManager*	m_pUserGuideManager;
			CSessionManager*	m_pServerSessionManager;		
};

#define SUPERVISORSERVICE	CSupervisorService::GetInstance()