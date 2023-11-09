#include "StdAfx.h"
#include "Connection.h"
#include "ConnectionManager.h"
#include "RelaySession.h"
#include "Log.h"
#include "Profiler.h"

START_NAMESPACE

CConnection::CConnection( CConnectionManager* pPM ) : m_pConnectionManager( pPM )
{
	m_pSession = NULL;
	m_pRecvBuffer = NULL;	
	m_dwLastRecvTick = 0;
	m_dwLastPulseTick = 0;
	m_dwRoundTripTime = DEFAULT_RESEND_TIME;
	m_dwPreviousRoundTripTime = DEFAULT_RESEND_TIME;
	m_nSendSequenceID = 0;
	m_nRecvSequenceID = -1;
	m_bDeleteFlag = false;
	m_bIsIdentify = false;
	m_dwProfileID = 0;
    ZeroMemory( &m_Address, sizeof(m_Address) );	

	ClearSendBuffer();
	ClearRecvBuffer();	
}

CConnection::~CConnection()
{
	ClearSendBuffer();
	ClearRecvBuffer();
}

void CConnection::SetAddress( DWORD dwIP, WORD wPort, struct sockaddr_in* pAddress )
{
	ZeroMemory( (void*)pAddress, sizeof( struct sockaddr_in ) );

	pAddress->sin_family = AF_INET;
	pAddress->sin_addr.s_addr = dwIP;
	pAddress->sin_port = htons( wPort );
}

bool CConnection::Run()
{
	if ( m_bDeleteFlag )
		return false;

	DWORD dwGetTickCount = ::GetTickCount();
	DWORD dwAliveTime = dwGetTickCount - SOCKET_KEEPALIVE_TIME;

	if ( m_bIsIdentify )
	{
		if ( m_dwLastPulseTick < dwAliveTime )
			SendToSignal( FLAG_PULSE );
	}
		
	ReSend(false);

	dwGetTickCount = ::GetTickCount();
	dwAliveTime = dwGetTickCount - SOCKET_ALIVE_TIME;

	if ( m_dwLastRecvTick < dwAliveTime )	
		m_bDeleteFlag = true;

	return true;
}

void CConnection::Close()
{
	SendToSignal( FLAG_CLOSE );
	m_bDeleteFlag = true;
}

void CConnection::PostCreate( CRelaySession* pSession, SOCKADDR_IN* pAddress )
{
	m_pSession = pSession;
	m_pRecvBuffer = m_pSession->GetRecvBuffer();	
	m_dwLastRecvTick = ::GetTickCount();
	m_dwLastPulseTick = ::GetTickCount();
	m_dwRoundTripTime = DEFAULT_RESEND_TIME;
	m_dwPreviousRoundTripTime = DEFAULT_RESEND_TIME;
	m_nSendSequenceID = 0;
	m_nRecvSequenceID = -1;
	m_bDeleteFlag = false;
	m_bIsIdentify = false;	
	m_dwProfileID = 0;
	memcpy( &m_Address, pAddress, sizeof(SOCKADDR_IN) );

	ClearSendBuffer();
	ClearRecvBuffer();
	m_ProfilerMap.clear();	
}

void CConnection::ClearSendBuffer()
{
	SendBufferlist::iterator iter_end( m_SendBufferlist.end() );
	for ( SendBufferlist::iterator iter = m_SendBufferlist.begin(); iter != iter_end; )
	{
		char* pSendBuffer = *iter;
		iter = m_SendBufferlist.erase( iter );
		m_pConnectionManager->Free( pSendBuffer );		
	}

	m_SendBufferlist.clear();	
}

void CConnection::ClearRecvBuffer()
{
	RecvBufferlist::iterator iter_end( m_RecvBufferlist.end() );
	for ( RecvBufferlist::iterator iter = m_RecvBufferlist.begin(); iter != iter_end; )
	{
		char* pRecvBuffer = *iter;
		iter = m_RecvBufferlist.erase( iter );
		m_pConnectionManager->Free( pRecvBuffer );
	}

	m_RecvBufferlist.clear();
}

void CConnection::SendToSignal( BYTE byFlag )
{
	char* pSendBuf = m_pConnectionManager->Alloc();
	LPBYTE pFlag = (LPBYTE)&pSendBuf[0];
	LPWORD pSize = (LPWORD)&pSendBuf[1];

	CPacket msgSend;

	switch(byFlag)
	{
		case FLAG_PULSE:
			m_dwLastPulseTick = ::GetTickCount(); 		
//			Log( _T("Send Flag Heartbeat") );
			break;
		case FLAG_CLOSE:
			break;
		case FLAG_TRANS_SUCCESS:
		case FLAG_RESEND:
			msgSend << m_nRecvSequenceID;													
			break;
		default:
			m_pConnectionManager->Free( pSendBuf );	
			return;
	}

	*pFlag = byFlag;
	msgSend.Encrypt();
	msgSend.AddCheckSum();
	memcpy( &pSendBuf[1], msgSend.GetBuffer(), msgSend.GetSize() );
	
	m_pSession->SendMessage( &pSendBuf[0], *pSize + PACKET_TAIL_SIZE + (UDP_HEADER_SIZE - 4), &m_Address );

	m_pConnectionManager->Free( pSendBuf );
}

inline int CConnection::SendMessage( CPacket& Packet, bool bEncrypt )
{
	if ( bEncrypt )
		Packet.Encrypt();

	Packet.AddCheckSum();
	return Send( Packet.GetBuffer(), Packet.GetSize() );
}

int CConnection::Send( BYTE* pBuf, WORD len )
{
	Synchronized so( &m_soSend );

	char* pSendBuf = m_pConnectionManager->Alloc();

	LPDWORD pSendTime	= (LPDWORD)&pSendBuf[HEADER_SENDTIME];
	LPBYTE	pState		= (LPBYTE)&pSendBuf[HEADER_STATE];
	LPBYTE	pFlag		= (LPBYTE)&pSendBuf[HEADER_FLAG];
	LPINT	pSeqID		= (LPINT)&pSendBuf[HEADER_SEQID];
	LPBYTE	pWrite		= (LPBYTE)&pSendBuf[HEADER_WRITE];
	LPWORD	pSize		= (LPWORD)&pSendBuf[HEADER_WRITE];

	*pSendTime	= ::timeGetTime();
	*pState		= 0;
	*pFlag		= (BYTE)FLAG_PACKET;
	*pSeqID		= m_nSendSequenceID; 
	memcpy( pWrite, pBuf, len );

#ifdef PROFILER_SUPPORT
	BeginProfiling();
#endif

	m_pSession->SendMessage( &pSendBuf[HEADER_FLAG], *pSize + PACKET_TAIL_SIZE + UDP_HEADER_SIZE, &m_Address );

	m_SendBufferlist.push_back( pSendBuf );	
	m_nSendSequenceID++;

	return ERROR_NONE;
}

void CConnection::ReSend( bool immediateMode )
{
	Synchronized so( &m_soSend );

	if ( m_SendBufferlist.empty() )
		return;

	SendBufferlist::iterator iter_end( m_SendBufferlist.end() );
	for( SendBufferlist::iterator iter = m_SendBufferlist.begin(); iter != iter_end; iter++ )
	{
		char*	pSendBuf	= *iter;
		LPDWORD pSendTime	= (LPDWORD)&pSendBuf[HEADER_SENDTIME];
		LPBYTE	pState		= (LPBYTE)&pSendBuf[HEADER_STATE];
		LPBYTE	pFlag		= (LPBYTE)&pSendBuf[HEADER_FLAG];
		LPWORD	pSize		= (LPWORD)&pSendBuf[HEADER_WRITE];		

		if ( IsReTransmission( *pSendTime ) || immediateMode || (!(*pSendTime)) )
		{
			if ( *pSendTime != 0 )
				*pState = 1;

			*pSendTime = ::timeGetTime();

			WSABUF wsaBuf;
			wsaBuf.buf = &pSendBuf[HEADER_FLAG];
			wsaBuf.len = *pSize + PACKET_TAIL_SIZE + UDP_HEADER_SIZE;

            m_pSession->SendMessage( wsaBuf.buf, wsaBuf.len, &m_Address );
		}
	}
}

void CConnection::RemoveDatagram( int sequenceID )
{
	Synchronized so( &m_soSend );

	if ( m_SendBufferlist.empty() )
		return;

	SendBufferlist::iterator iter_end( m_SendBufferlist.end() );
	for ( SendBufferlist::iterator iter = m_SendBufferlist.begin(); iter != iter_end; )
	{
		char*	pSendBuf	= *iter;		
		LPINT	pSeqID		= (LPINT)&pSendBuf[HEADER_SEQID];

		if ( sequenceID == *pSeqID )
		{
			LPDWORD pSendTime		= (LPDWORD)&pSendBuf[HEADER_SENDTIME];
			LPBYTE	pState			= (LPBYTE)&pSendBuf[HEADER_STATE];
			DWORD	dwCurrentTick	= ::timeGetTime();

			SetRoundTripTime( *pSendTime, dwCurrentTick, *pState );

			m_pConnectionManager->Free( pSendBuf );
			iter = m_SendBufferlist.erase(iter);

#ifdef PROFILER_SUPPORT
			EndProfiling( *pSeqID );
#endif
			return;
		}
		else
		{
			iter++;				
		}
	}	
}

void CConnection::SetNextBufferingBuffer( BYTE* pBuffer, DWORD size )
{
	int sequenceID = (int)(*(int*)(pBuffer + 1));
    
	RecvBufferlist::iterator iter_end = m_RecvBufferlist.end();
	for( RecvBufferlist::iterator iter = m_RecvBufferlist.begin(); iter != iter_end; iter++ )
	{
		char* pRecvBuffer = *iter;
		int checkSequenceID = (int)(*(int*)(pRecvBuffer + 1));

		if ( sequenceID == checkSequenceID )
			return;
	}

	// 여기까지 오면 리시브버퍼 리스트에 시퀀스 아이디가 없는거야..
	char* pAllocBuffer = m_pConnectionManager->Alloc();
	memcpy( pAllocBuffer, pBuffer, size );
	m_RecvBufferlist.push_back( pAllocBuffer );
}

void CConnection::CheckBufferingBuffer()
{
	if ( m_RecvBufferlist.empty() )
		return;

	RecvBufferlist::iterator iter_end( m_RecvBufferlist.end() );
	for ( RecvBufferlist::iterator iter = m_RecvBufferlist.begin(); iter != iter_end; )
	{
		char* pRecvBuffer = *iter;
		int checkSequenceID = (int)(*(int*)(pRecvBuffer + 1));
		if ( CheckRecvSequenceID( checkSequenceID - 1 ) != 0 )
		{
			iter++;
			continue;
		}

		LPWORD pRecvSize = (LPWORD)(pRecvBuffer + UDP_HEADER_SIZE);
		if ( !(*pRecvSize) )
		{
			iter++;
			continue;
		}

		WORD wTotalBytes = *pRecvSize + PACKET_TAIL_SIZE + UDP_HEADER_SIZE;
		if( !ProcessMessage( (BYTE*)pRecvBuffer + UDP_HEADER_SIZE, wTotalBytes - UDP_HEADER_SIZE, checkSequenceID ) )
		{
			iter++;
			continue;	
		}

		iter = m_RecvBufferlist.erase( iter );
		m_pConnectionManager->Free( pRecvBuffer );
	}
}

void CConnection::SetRoundTripTime( DWORD dwSendTime, DWORD dwCurrentTick, BYTE byState )
{
	if ( dwSendTime && (byState == 0) )
	{
//		TCP 에서의 재전송 시간 결정 -> 재전송 시간 = 2 * RTT(Round Trip Time:패킷 왕복 시간)
//		RTT = x * 이전 RTT + ( 1 - x ) * 현재 RTT
//		x 는 보통 90%가 사용된다.
		m_dwRoundTripTime = ( (m_dwPreviousRoundTripTime * 10) + ( ( dwCurrentTick - dwSendTime ) * 90 ) ) / 100;
		m_dwRoundTripTime *= 2;
		m_dwPreviousRoundTripTime = dwCurrentTick - dwSendTime;				
	}
}

bool CConnection::IsReTransmission( DWORD dwSendTime )
{
	DWORD dwtimeGetTime = ::timeGetTime();
	return ( dwSendTime < dwtimeGetTime - m_dwRoundTripTime ) ? true : false;
}

// 지금 Receive 된 패킷의 SequenceID가 적합한지 체크한다. 0 : 적합한 패킷 1 : 다음 패킷 2 : 이전패킷
BYTE CConnection::CheckRecvSequenceID( int nSequenceID )
{
	if ( ( m_nRecvSequenceID == -1 ) || ( nSequenceID == m_nRecvSequenceID ) ) 
		return 0;

	if ( m_nRecvSequenceID  < nSequenceID )
		return 1;

	return 2;
}

bool CConnection::OnMsgPacketRight( WORD totalBytes, int nSequenceID )
{
	if( !ProcessMessage( m_pRecvBuffer + UDP_HEADER_SIZE, totalBytes - UDP_HEADER_SIZE, nSequenceID ) )
		return false;

	CheckBufferingBuffer();	

	return true;
}

bool CConnection::OnMsgPacketNext( WORD totalBytes, int nSequenceID )
{
	CPacket msg;
	msg.SetBuffer( m_pRecvBuffer + UDP_HEADER_SIZE, totalBytes - UDP_HEADER_SIZE );

	if ( !msg.IsValidate() )
		return false;

	SetNextBufferingBuffer( m_pRecvBuffer, totalBytes );
	SendToSignal( FLAG_RESEND );	

	return true;
}

bool CConnection::OnMsgPacketPrevious( WORD totalBytes, int nSequenceID )
{
	SendToSignal( FLAG_TRANS_SUCCESS );	
	return true;
}

bool CConnection::ProcessMessage( BYTE* pBuf, WORD len, int nSequenceID )
{
	CPacket msg;
	msg.SetBuffer( pBuf, len );

	if ( !msg.IsValidate() )
		return false;

	m_nRecvSequenceID = nSequenceID;
	SendToSignal( FLAG_TRANS_SUCCESS );

//	Log( "[ RecvPacket ID : %d ]", msg.ID() );
	msg.Decrypt();

	ProcessMessage( msg );

	return true;
}

void CConnection::OnReceived( BYTE byFlag, DWORD dwBytes )
{
	bool bResult = CALL_PARSING_MSG_HANDLER( CConnection, byFlag, dwBytes );

	if ( bResult )
		m_dwLastRecvTick = ::GetTickCount();
}

BEGIN_PARSING_MSG_MAP( CConnection, OnMsgUnhandled )
	BIND_PARSING_MSG_HANDLER( CConnection, FLAG_PACKET, OnMsgPacket )	
	BIND_PARSING_MSG_HANDLER( CConnection, FLAG_TRANS_SUCCESS, OnMsgTransSuccess )
	BIND_PARSING_MSG_HANDLER( CConnection, FLAG_RESEND, OnMsgReSend )
	BIND_PARSING_MSG_HANDLER( CConnection, FLAG_PULSE, OnMsgPulse )
	BIND_PARSING_MSG_HANDLER( CConnection, FLAG_CLOSE, OnMsgClose )
END_PARSING_MSG_MAP( CConnection )

bool CConnection::OnMsgUnhandled( DWORD dwBytes )
{
	Log( _T("OnReceived::OnMsgUnhandled()") );
	return false;
}

bool CConnection::OnMsgPacket( DWORD dwBytes )
{
	int		nSequenceID	= (int)(*(int*)(m_pRecvBuffer + 1));
	LPWORD	pRecvSize	= (LPWORD)(m_pRecvBuffer + UDP_HEADER_SIZE);
	LPWORD	pID			= (LPWORD)(m_pRecvBuffer + RECV_HEADER_ID);
	WORD	wTotalBytes = *pRecvSize + PACKET_TAIL_SIZE + UDP_HEADER_SIZE;

	if ( !(*pRecvSize) || wTotalBytes != dwBytes )
		return false;

	BYTE byCheck = CheckRecvSequenceID( nSequenceID - 1 );

	bool bReturn = false;

	switch( byCheck )
	{
		case 0:	bReturn = OnMsgPacketRight( wTotalBytes, nSequenceID );		break;
		case 1:	bReturn = OnMsgPacketNext( wTotalBytes, nSequenceID );		break;
		case 2:	bReturn = OnMsgPacketPrevious( wTotalBytes, nSequenceID );	break;
		default: bReturn = false;
	}

	return bReturn;
}

bool CConnection::OnMsgTransSuccess( DWORD dwBytes )
{
	LPWORD pRecvSize = (LPWORD)(m_pRecvBuffer + 1);
	WORD wTotalBytes = *pRecvSize + PACKET_TAIL_SIZE + 1;

	if ( !(*pRecvSize) || wTotalBytes != dwBytes )
		return false;

	CPacket msg;
	msg.SetBuffer( m_pRecvBuffer + 1, wTotalBytes - 1 );

	if ( !msg.IsValidate() )
		return false;

	msg.Decrypt();

	int SequenceID = 0;
	msg >> SequenceID; 	
	RemoveDatagram( SequenceID );
	ReSend(false);	

	return true;
}

bool CConnection::OnMsgReSend( DWORD dwBytes )
{
	LPWORD pRecvSize = (LPWORD)(m_pRecvBuffer + 1);
	WORD wTotalBytes = *pRecvSize + PACKET_TAIL_SIZE + 1;

	if ( !(*pRecvSize) || wTotalBytes != dwBytes )
		return false;

	CPacket msg;
	msg.SetBuffer( m_pRecvBuffer + 1, wTotalBytes - 1 );

	if ( !msg.IsValidate() )
		return false;

	msg.Decrypt();

	int SequenceID = 0;
	msg >> SequenceID; 	
	RemoveDatagram( SequenceID );
	ReSend(true);	

	return true;
}

bool CConnection::OnMsgPulse( DWORD dwBytes )
{
	LPWORD pRecvSize = (LPWORD)(m_pRecvBuffer + 1);
	WORD wTotalBytes = *pRecvSize + PACKET_TAIL_SIZE + 1;

	if ( !(*pRecvSize) || wTotalBytes != dwBytes )
		return false;

	CPacket msg;
	msg.SetBuffer( m_pRecvBuffer + 1, wTotalBytes - 1 );

	if ( !msg.IsValidate() )
		return false;

	Log( _T("OnReceived::OnMsgPulse()") );

	return true;
}

bool CConnection::OnMsgClose( DWORD dwBytes )
{
	LPWORD pRecvSize = (LPWORD)(m_pRecvBuffer + 1);
	WORD wTotalBytes = *pRecvSize + PACKET_TAIL_SIZE + 1;

	if ( !(*pRecvSize) || wTotalBytes != dwBytes )
		return false;

	CPacket msg;
	msg.SetBuffer( m_pRecvBuffer + 1, wTotalBytes - 1 );

	if ( !msg.IsValidate() )
		return false;

	m_bDeleteFlag = true;

	return true;
}

#ifdef PROFILER_SUPPORT
void CConnection::BeginProfiling()
{
	DWORD dwRunKey = 0;
	PROFILER->StartProfiling( m_dwProfileID, &dwRunKey );
	m_ProfilerMap.insert( ProfilerMap::value_type( m_nSendSequenceID, dwRunKey ) );
}

void CConnection::EndProfiling( int sequenceID )
{
	ProfilerMap::iterator iter_find = m_ProfilerMap.find( sequenceID );
	if ( iter_find != m_ProfilerMap.end() )
	{
		PROFILER->EndProfiling( m_dwProfileID, (*iter_find).second );

		float fAvgResult = PROFILER->GetAvgResult( m_dwProfileID );
		DWORD dwTotalCount = PROFILER->GetTotalCount( m_dwProfileID );
//		Log( _T("Average Round Trip Time : %.4f ms / Total : %d") , fAvgResult, dwTotalCount );

		m_ProfilerMap.erase( iter_find );
	}
}
#endif

END_NAMESPACE