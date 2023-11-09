#pragma once

#include "Netmsg.h"

START_NAMESPACE

class RENET_API CBaseSession
{
public:
					CBaseSession(void);
	virtual			~CBaseSession(void);
	virtual int		Dispatch( DWORD& rdwDispatchCount, LPOVERLAPPED_BASE lpov ) = NULL;
	virtual void	OnErrorSession( DWORD dwErrorCode, int nDetail ) = NULL;
};

END_NAMESPACE
