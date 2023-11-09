#pragma once

#include "Session.h"

START_NAMESPACE

class RENET_API CListeningSession :	public CSession
{
public:
					CListeningSession( CSessionManager* pSM );
	virtual			~CListeningSession();
	virtual bool	Create( LPCTSTR szAddr, int nPort );
	virtual int		Dispatch( DWORD& rdwDispatchCount, LPOVERLAPPED_BASE lpov );
	virtual int		DispatchAccept( DWORD dwTransferCount, LPOVERLAPPED_BASE lpov );
	virtual int		PrepareAccept();
	virtual void	Close( int nError, int nDetail );
	virtual int		PrepareReceive( void );
	virtual int		SendMessage( CMsgSend& msgSend, bool bEncrypt = true );
	virtual bool	OnError( DWORD dwErrorCode, int nDetail );
	const	SOCKET	GetAcceptSocket();
protected:
			SOCKET	m_hAcceptSocket;
			TCHAR	m_strIPAddr[256];
			int		m_nPort;
};

END_NAMESPACE