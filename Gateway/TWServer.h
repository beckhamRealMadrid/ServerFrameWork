#pragma once

#include "TwoWaySession.h"

class CTWServer : public CTwoWaySession
{
	DECLARE_MESSAGEMAP()
public:
					CTWServer( CSessionManager* pSM, CTwoWaySession* pOtherSession );
	virtual			~CTWServer(void);
	virtual int		DispatchRecv( DWORD dwBytes, LPOVERLAPPED_BASE lpov );
	virtual	void	Close( int nError, int nDetail );
	virtual bool	OnError( DWORD dwErrorCode, int nDetail );	
	virtual void	PostCreate();
};
