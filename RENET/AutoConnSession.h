#pragma once

#include "Session.h"

START_NAMESPACE

class RENET_API CAutoConnSession : public CSession
{
public:
								CAutoConnSession( CSessionManager* pSM = NULL );
	virtual						~CAutoConnSession();
	virtual bool				OnError(DWORD dwErrorCode, int nDetail);
	static	unsigned __stdcall	WorkerThread( void* lpParam );
			void				AutoConnector();
			bool				StartAutoConnect();
			void				SetAutoConnInfo( LPCTSTR szAddr, int nPort );
			LPCTSTR				GetAutoConnAddr()	{ return m_strAutoConnAddr; }
			int					GetAutoConnPort()	{ return m_nAutoConnPort; }
protected:
			HANDLE				m_hConnector;
			HANDLE				m_hKillConnector;
			HANDLE				m_hConnectorKilled;
			TCHAR				m_strAutoConnAddr[50];
			int					m_nAutoConnPort;
			bool				m_bReconnect;
			DWORD				m_dwDelay;	
};

END_NAMESPACE