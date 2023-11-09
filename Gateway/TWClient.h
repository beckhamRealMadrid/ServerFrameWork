#pragma once

#include "TwoWaySession.h"

class CUserMap;

class CTWClient : public CTwoWaySession
{
	DECLARE_MESSAGEMAP()
public:
					CTWClient( CSessionManager* pSM );
	virtual			~CTWClient(void);
	virtual int		DispatchRecv( DWORD dwBytes, LPOVERLAPPED_BASE lpov );
	virtual bool	OnError( DWORD dwErrorCode, int nDetail );
	virtual	void	Close( int nError, int nDetail );
	virtual void	PostCreate();
public:
			bool	GetPassThru()					{ return m_bPassThru; }
			void	SetPassThru( bool bPassThru )	{ m_bPassThru = bPassThru; }
protected:
			void	OnLoginReq( CMsgRecv& msg );
protected:
			bool	m_bPassThru;			
};