#include "StdAfx.h"
#include "RelaySession.h"
#include "SockSystem.h"
#include "Synchronized.h"
#include "ConnectionManager.h"
#include "Connection.h"

START_NAMESPACE

CRelaySession::CRelaySession( CConnectionManager* pPM ) : m_pConnectionManager(pPM)
{
	m_hSocket = INVALID_SOCKET;
	m_nPort = 0;
	m_dwRecvSize = 0;
	m_dwFlag = 0;
	m_nAddrLength = sizeof( SOCKADDR_IN );
	m_bSendInProgress = false;
	m_listMsgToSend.clear();

	memset( m_strIPAddr, NULL, sizeof(m_strIPAddr) );
	memset( m_pRecvBuffer, NULL, sizeof(m_pRecvBuffer) );	
	memset( &m_FromAddr, NULL, sizeof(SOCKADDR_IN) );	
}

CRelaySession::~CRelaySession(void)
{
	Destroy();
}

BYTE* CRelaySession::GetRecvBuffer()
{
	return m_pRecvBuffer;
}

bool CRelaySession::Create( LPCTSTR szAddr, int nPort )
{
	BYTE byLogCode = 0;

	__try
	{
		::StringCchCopy( m_strIPAddr, 256, szAddr );
		m_nPort = nPort;

		struct sockaddr_in serv_addr;
		memset( &serv_addr, 0, sizeof(serv_addr) );

		serv_addr.sin_family = AF_INET;
		if ( szAddr )
			serv_addr.sin_addr.s_addr = inet_addr( CODE_CONVERT_ANSI(m_strIPAddr) );
		else
			serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
		serv_addr.sin_port = htons(m_nPort);

		m_hSocket = ::WSASocket( AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED );
		if ( m_hSocket == INVALID_SOCKET )
			FAIL_VALUE( byLogCode, 1 );

		int nbind = ::bind( m_hSocket, ( const struct sockaddr * )&serv_addr, sizeof( serv_addr ) );
		if ( nbind == SOCKET_ERROR )
			FAIL_VALUE( byLogCode, 2 );

		int optsize = 4;
		int def_send_size = 0;
		int max_send_size = 0;

		::setsockopt( m_hSocket, SOL_SOCKET, SO_SNDBUF, (char*)&max_send_size, 4 );
		::getsockopt( m_hSocket, SOL_SOCKET, SO_SNDBUF, (char*)&def_send_size, &optsize );

		Log( _T("<UDP> listen IPAddr : %s Port : %d"), m_strIPAddr, m_nPort );

		__leave;	
	}
	__finally	
	{
		switch( byLogCode )
		{
			case 1:	Log( _T("Relay socket creation failed on socket()") );	break;
			case 2: Log( _T("Relay socket creation failed on bind()") );	break;			
			default: break;
		}	
	}

	if ( byLogCode )
		return false;
	
	return true;
}

void CRelaySession::Destroy()
{
	Close();

	Synchronized so( &m_soSend );

	LPUDPSENDBUCKET pBucket = NULL;
	for ( netmsgudplistitor itor = m_listMsgToSend.begin(); itor != m_listMsgToSend.end(); itor++ )
	{
		pBucket = (*itor);
		m_SendPool.FreeItem( pBucket );
	}

	m_listMsgToSend.clear();	
}

void CRelaySession::Close()
{
	if ( m_hSocket != INVALID_SOCKET )
	{
		LINGER lingerStruct;
		lingerStruct.l_onoff  = 1;
		lingerStruct.l_linger = 0;
		::setsockopt( m_hSocket, SOL_SOCKET, SO_LINGER, ( char* )&lingerStruct, sizeof( lingerStruct ) );

		::closesocket( m_hSocket );

		m_hSocket = INVALID_SOCKET;
	}
}

int	CRelaySession::Dispatch( DWORD& rdwDispatchCount, LPOVERLAPPED_BASE lpov )
{
	if ( lpov == NULL )
		return ERROR_NONE;

	switch ( lpov->m_dwFlag )
	{
		case IOCP_RECV: return DispatchRecv( rdwDispatchCount, lpov );
		case IOCP_SEND:	return DispatchSend( rdwDispatchCount, lpov );
		default :		assert( false );	return ERROR_NONE;
	}
}

int CRelaySession::SendMessage( char FAR* pBuf, u_long len, SOCKADDR_IN* pToAddress )
{
	assert( len < NETWORK_UDP_SENDBUFFER_SIZE );

	int nResult = RequestSend( pBuf, len, pToAddress );
	return ERROR_NONE;
}

int CRelaySession::RequestSend( char FAR* pBuf, u_long len, SOCKADDR_IN* pToAddress )
{
	Synchronized so( &m_soSend );

	if ( m_bSendInProgress )
	{
		UDPSENDBUCKET* pBucket = m_SendPool.NewItem();
		memcpy( &pBucket->m_ToAddr, pToAddress, sizeof(SOCKADDR_IN) );
		memcpy( pBucket->m_pBuf, pBuf, len );
		pBucket->m_len = len;

		m_listMsgToSend.push_back( pBucket );	
	}
	else
	{
		WSABUF wsabuf;
		wsabuf.buf = pBuf;
		wsabuf.len = len;

		memset( &m_ovSend, 0, sizeof(OVERLAPPED_BASE) );
		m_ovSend.m_dwFlag = IOCP_SEND;

		DWORD dwBytesWritten = 0;

		int ret = ::WSASendTo( m_hSocket, &wsabuf, 1, &dwBytesWritten, 0, (const struct sockaddr*)pToAddress, m_nAddrLength, (LPOVERLAPPED)&m_ovSend, NULL );
		
		if ( ret == SOCKET_ERROR )
		{
			int nLastError = WSAGetLastError();
			if ( nLastError != WSA_IO_PENDING && nLastError != ERROR_SUCCESS )
			{
				WSAERROR2( nLastError, _T("Fail to WSASendTo") , _T("CRelaySession::RequestSend()") );
				return ERROR_NONE;
			}
		}

		m_bSendInProgress = true;		
	}

	return ERROR_NONE;
}

int CRelaySession::DispatchSend( DWORD dwBytes, LPOVERLAPPED_BASE lpov )
{
	assert( lpov == &m_ovSend );

	if ( dwBytes == 0 )
		return ERROR_NONE;

	Synchronized so( &m_soSend );

	m_bSendInProgress = false;

	if ( m_listMsgToSend.empty() )
		return ERROR_NONE;

	m_bSendInProgress = true;

	WSABUF wsabuf;
	memset( &m_ovSend, 0, sizeof(OVERLAPPED_BASE) );
	m_ovSend.m_dwFlag = IOCP_SEND;

	DWORD dwBytesWritten = 0;
	LPUDPSENDBUCKET pBucket = NULL;

	while ( !m_listMsgToSend.empty() )
	{
		pBucket = m_listMsgToSend.front();	
		wsabuf.buf = pBucket->m_pBuf;
		wsabuf.len = pBucket->m_len;

		int ret = ::WSASendTo( 
			m_hSocket, &wsabuf, 1, &dwBytesWritten, 0, (const struct sockaddr*)&pBucket->m_ToAddr, m_nAddrLength, (LPOVERLAPPED)&m_ovSend, NULL );

		if ( ret == SOCKET_ERROR )
		{
			int nLastError = WSAGetLastError();
			if ( nLastError != WSA_IO_PENDING && nLastError != ERROR_SUCCESS )
			{
				WSAERROR2( nLastError, _T("Fail to WSASendTo") , _T("CRelaySession::DispatchSend()") );				
			}		
		}

		m_SendPool.FreeItem( pBucket );
		m_listMsgToSend.pop_front();
	}

	return ERROR_NONE;
}

void CRelaySession::OnErrorSession( DWORD dwErrorCode, int nDetail )
{
	while ( true )
	{
		int nResult = PrepareReceive();
		if ( ERROR_NONE == nResult )
		{
			Log( "Prepare Receive Error None", __FUNCTION__ );
			break;
		}

		::Sleep(1);
	}    	
}

int CRelaySession::PrepareReceive()
{
	memset( &m_ovRecv, 0, sizeof(OVERLAPPED_BASE) );
	m_ovRecv.m_dwFlag = IOCP_RECV;

	WSABUF wsaBuf;
	wsaBuf.buf = (char FAR *)m_pRecvBuffer;
	wsaBuf.len = NETWORK_UDP_RECVBUFFER_SIZE;

	int ret	= ::WSARecvFrom( m_hSocket, &wsaBuf, 1, &m_dwRecvSize, &m_dwFlag, (struct sockaddr*)&m_FromAddr, &m_nAddrLength, (LPOVERLAPPED)&m_ovRecv, NULL );

	if ( ret == SOCKET_ERROR )
	{
		int nLastError = WSAGetLastError();
		if ( nLastError != WSA_IO_PENDING )
		{
			WSAERROR2( nLastError, _T("Fail to WSARecvFrom") , _T("CRelaySession::PrepareReceive()") );
			return ERROR_RECV;		
		}
	}

	return ERROR_NONE;
}

int	CRelaySession::DispatchRecv( DWORD dwBytes, LPOVERLAPPED_BASE lpov )
{
	assert( lpov == &m_ovRecv );

	if ( dwBytes == 0 )
		return ERROR_NONE;

	Synchronized so( &m_soRecv );

	int nError = OnReceived( dwBytes );

	PrepareReceive();

	return ERROR_NONE;
}

int CRelaySession::OnReceived( DWORD dwBytes )
{
	Synchronized so( &m_soRecv );

	CConnection* pSess = m_pConnectionManager->GetSession( &m_FromAddr );
	if ( NULL == pSess )
		return ERROR_NONE;

	BYTE* pFlag = (BYTE*)(m_pRecvBuffer);
	pSess->OnReceived( *pFlag, dwBytes );

	return ERROR_NONE;
}

END_NAMESPACE