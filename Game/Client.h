#pragma once

#include "Connection.h"

class CClientMap;

class CClient : public CConnection
{
	DECLARE_MESSAGE_MAP()
public:
						CClient( CConnectionManager* pPM );
	virtual				~CClient(void);
	virtual	void		OnClose();
	virtual	void		PostCreate( CRelaySession* pSession, SOCKADDR_IN* pAddress );			
};