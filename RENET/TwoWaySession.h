#pragma once

#include "Session.h"

START_NAMESPACE

class RENET_API CTwoWaySession : public CSession
{
public:
							CTwoWaySession( CSessionManager* pSessionManager );
	virtual	bool			Connect( LPCTSTR strAddr, int nPort );
			CTwoWaySession* GetOtherSession()	{ return m_pOtherSession; }
protected:
	virtual int				DispatchRecv( DWORD dwBytes, LPOVERLAPPED_BASE lpov );
protected:
			CTwoWaySession* m_pOtherSession;			
};

END_NAMESPACE