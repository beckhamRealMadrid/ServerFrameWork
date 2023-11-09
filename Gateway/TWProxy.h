#pragma once

#include "TwoWaySession.h"

class CTWClient;

class CTWProxy : public CTwoWaySession
{
	DECLARE_MESSAGEMAP()
public:
					CTWProxy( CSessionManager* pSM );
	virtual			~CTWProxy(void);
	virtual int		DispatchRecv( DWORD dwBytes, LPOVERLAPPED_BASE lpov );
	virtual	void	Close( int nError, int nDetail );
	virtual bool	OnError( DWORD dwErrorCode, int nDetail );	
	virtual void	PostCreate();
			void	SetProxyClient( CTwoWaySession* pOtherSession )	{ m_pOtherSession = pOtherSession; }
};
