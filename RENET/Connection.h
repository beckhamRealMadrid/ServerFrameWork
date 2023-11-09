#pragma once

#include "Synchronized.h"
#include "Packet.h"

START_NAMESPACE

class CConnectionManager;
class CRelaySession;

class RENET_API CConnection
{
	DECLARE_PARSING_MSG_HANDLER( CConnection )
public:
								CConnection( CConnectionManager* pPM );
	virtual						~CConnection();
	virtual	void				OnClose()						{}
	virtual	void				ProcessMessage( CPacket& Msg )	{}	
	virtual	bool				IsOnBindMessageID( WORD id )	{ return false; }
	virtual	void				PostCreate( CRelaySession* pSession, SOCKADDR_IN* pAddress );
			bool				Run();
			void				Close();
			void				OnReceived( BYTE byFlag, DWORD dwBytes );	
			int					SendMessage( CPacket& Packet, bool bEncrypt = true );
			LPSOCKADDR_IN		GetAddress()					{ return &m_Address; }
			void				SetProfileID( DWORD dwID )		{ m_dwProfileID = dwID; }
			void				SetIdentify( bool bIdentify )	{ m_bIsIdentify = bIdentify; }
			void				SetAddress( DWORD dwIP, WORD wPort, struct sockaddr_in* pAddress );
private:
			int					Send( BYTE* pBuf, WORD len );	
			void				SendToSignal( BYTE byFlag );
			void				ReSend( bool immediateMode );
			void				RemoveDatagram( int sequenceID );
			void				SetNextBufferingBuffer( BYTE* pBuffer, DWORD size );
			void				SetRoundTripTime( DWORD dwSendTime, DWORD dwCurrentTick, BYTE byState );
			bool				IsReTransmission( DWORD dwSendTime );
			BYTE				CheckRecvSequenceID( int nSequenceID );
			void				CheckBufferingBuffer();
			bool				ProcessMessage( BYTE* pBuf, WORD len, int nSequenceID );
			void				ClearSendBuffer();
			void				ClearRecvBuffer();
			bool				OnMsgUnhandled( DWORD dwBytes );
			bool				OnMsgPacket( DWORD dwBytes );
			bool				OnMsgPulse( DWORD dwBytes );
			bool				OnMsgReSend( DWORD dwBytes );
			bool				OnMsgTransSuccess( DWORD dwBytes );
			bool				OnMsgClose( DWORD dwBytes );
			bool				OnMsgPacketRight( WORD totalBytes, int nSequenceID );
			bool				OnMsgPacketNext( WORD totalBytes, int nSequenceID );
			bool				OnMsgPacketPrevious( WORD totalBytes, int nSequenceID );
#ifdef PROFILER_SUPPORT
			void				BeginProfiling();
			void				EndProfiling( int sequenceID );
#endif
public:
	typedef	void (CConnection::*PFMsg) (CPacket& Msg);
	typedef	std::map <DWORD, PFMsg>				msgmap;
	typedef	std::map <DWORD, PFMsg>::iterator	msgmapitor;
	typedef	std::list< char* >					SendBufferlist;
	typedef	std::list< char* >					RecvBufferlist;
	typedef	std::map < int, DWORD >				ProfilerMap;
private:
			CConnectionManager*	m_pConnectionManager;
			CRelaySession*		m_pSession;	
			BYTE*				m_pRecvBuffer;
			SOCKADDR_IN			m_Address;
			DWORD				m_dwLastRecvTick;
			DWORD				m_dwLastPulseTick;
			DWORD				m_dwRoundTripTime;
			DWORD				m_dwPreviousRoundTripTime;
			int					m_nSendSequenceID;
			int					m_nRecvSequenceID;
			SectionObject		m_soSend;
			SendBufferlist		m_SendBufferlist;
			RecvBufferlist		m_RecvBufferlist;
			ProfilerMap			m_ProfilerMap;
			bool				m_bDeleteFlag;
			DWORD				m_dwProfileID;
protected:
			bool				m_bIsIdentify;
};

END_NAMESPACE