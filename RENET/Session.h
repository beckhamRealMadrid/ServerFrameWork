#pragma once

#include "BaseSession.h"
#include "Msg.h"
#include "SectionObject.h"
#include "Synchronized.h"

START_NAMESPACE

class CSessionManager;
class CSession;

class RENET_API CSession : public CBaseSession
{
public:
								CSession( CSessionManager* pSM );
	virtual						~CSession();
	virtual bool				Create( SOCKET hSocket );	
	virtual bool				Connect( LPCTSTR strAddr, int nPort );	
	virtual void				Close( int nError, int nDetail );	
	virtual int					PrepareReceive( DWORD dwFlag = IOCP_RECV );	
	virtual int					SendMessage( CMsgSend& msgSend, bool bEncrypt = true ); 
	virtual int					Send( CNetMsg* pNetMsg );	
	virtual int					Dispatch( DWORD& rdwDispatchCount, LPOVERLAPPED_BASE lpov );
	virtual int					DispatchRecv( DWORD dwReadCount, LPOVERLAPPED_BASE lpov );
	virtual int					DispatchSend( DWORD dwSendCount, LPOVERLAPPED_BASE lpov );
	virtual int					DispatchAccept( DWORD dwTransferCount, LPOVERLAPPED_BASE lpov );
	virtual int					DispatchTransmitFileRecv( DWORD dwTransferCount, LPOVERLAPPED_BASE lpov );
	virtual int					DispatchTransmitFileCompleted( DWORD dwTransferCount, LPOVERLAPPED_BASE lpov );
	virtual bool				OnConnected();
	virtual bool				OnError( DWORD dwErrorCode, int nDetail );
	virtual int					OnReceived();
	virtual void				OnErrorSession( DWORD dwErrorCode, int nDetail );	
	virtual void				PostCreate();
	virtual int					ProcessMessage( CMsgRecv& msg )	{ return ERROR_NONE; }	
	virtual	void				SendAliveReq();
			void				GetErrorMessage( LPTSTR ErrMsg );
			bool				IsActive()						{ return m_bIsActive; }
			LPCTSTR				GetUserID()						{ return m_strUserID; }
			void				SetUserID(LPCTSTR szUserID)		{ ::StringCchCopy( m_strUserID, 50, szUserID ); }
			LPCTSTR				GetIPAddr()						{ return m_strIPAddr; }
			SOCKET				GetSocket()						{ return m_hSocket; }
            void				SetIPAddr(LPCTSTR strIPAddr)	{ ::StringCchCopy(m_strIPAddr, 256, strIPAddr);	}
			CSessionManager*	GetSessionManager()				{ return m_pSessionManager; }
			WORD*				GetNotRecvedCount()				{ return &m_wNotRecvedCount; }
			OVERLAPPED_BASE*	GetOverlappedRecv()				{ return &m_ovRecv; }
public:
	typedef void (CSession::*PFMsg) (CMsgRecv& msg);
	typedef std::map <DWORD, PFMsg> msgmap;
	typedef std::map <DWORD, PFMsg>::iterator msgmapitor;
	typedef std::map <DWORD, CNetMsg*> netmsgmap;
	typedef std::map <DWORD, CNetMsg*>::iterator netmsgmapitor;
protected:
			CSessionManager*	m_pSessionManager;
			TCHAR				m_strUserID[50];
			TCHAR				m_strIPAddr[256];
			SectionObject		m_soSend;
			SectionObject		m_soRecv;
			DWORD				m_timeLastAccess;
			DWORD				m_dwIPAddr;
			bool				m_bIsActive;
			bool				m_bSendInProgress;
			SOCKET				m_hSocket;
			CCircularQueue*		m_pCQue;
			OVERLAPPED_BASE		m_ovRecv;
			OVERLAPPED_SEND		m_ovSend;
			WORD				m_wPort;
			WORD				m_wNotRecvedCount;			
private:
			netmsglist			m_listMsgToSend;
};

END_NAMESPACE