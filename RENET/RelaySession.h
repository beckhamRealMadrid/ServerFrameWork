#pragma once

#include "BaseSession.h"
#include "ChunkAllocator.h"
#include "Sectionobject.h"
#include "Netmsg.h"

START_NAMESPACE

class CConnectionManager;

class RENET_API CRelaySession : public CBaseSession
{
public:
								CRelaySession( CConnectionManager* pPM );
	virtual						~CRelaySession(void);
	virtual void				OnErrorSession( DWORD dwErrorCode, int nDetail );
	virtual int					Dispatch( DWORD& rdwDispatchCount, LPOVERLAPPED_BASE lpov );
			int					DispatchRecv( DWORD dwBytes, LPOVERLAPPED_BASE lpov );
			int					DispatchSend( DWORD dwBytes, LPOVERLAPPED_BASE lpov );
			bool				Create( LPCTSTR szAddr, int nPort );
			int					SendMessage( char FAR* pBuf, u_long len, SOCKADDR_IN* pToAddress );
			int					PrepareReceive();
			BYTE*				GetRecvBuffer();
	const	SOCKET				GetSocket()		{ return m_hSocket; }
protected:
			int					OnReceived( DWORD dwBytes );
			int					RequestSend( char FAR* pBuf, u_long len, SOCKADDR_IN* pToAddress );
			void				Destroy();
			void				Close();
private:
			SOCKET				m_hSocket;
			TCHAR				m_strIPAddr[256];
			BYTE				m_pRecvBuffer[NETWORK_UDP_RECVBUFFER_SIZE];
			bool				m_bSendInProgress;
			DWORD				m_dwRecvSize;
			DWORD				m_dwFlag;
			int					m_nPort;
			int					m_nAddrLength;
			SOCKADDR_IN			m_FromAddr;
			OVERLAPPED_BASE		m_ovRecv;
			OVERLAPPED_BASE		m_ovSend;
			SectionObject		m_soRecv;
			SectionObject		m_soSend;
			netmsgudplist		m_listMsgToSend;
			CConnectionManager*	m_pConnectionManager;
			CChunkAllocatorMT<UDPSENDBUCKET> m_SendPool;	
};

END_NAMESPACE
