#include "stdafx.h"
#include "Session.h"
#include "SessionManager.h"
#include "SockSystem.h"

START_NAMESPACE

CSession::CSession( CSessionManager* pSM )
{
	m_pSessionManager = pSM;
	m_strUserID[0] = NULL;
	m_strIPAddr[0] = NULL;
	m_hSocket = INVALID_SOCKET;
	m_timeLastAccess = ::GetTickCount();
	m_dwIPAddr = 0;
	m_wPort = 0;
	m_wNotRecvedCount = 0;
	m_bIsActive = false;
	m_pCQue	= new CCircularQueue( NETWORK_RECVBUFFER_SIZE );
}

CSession::~CSession()
{
	Close( 0, 0 );
	delete m_pCQue;
}

bool CSession::Create( SOCKET hSocket )
{
	Close( 0, 0 );

	m_hSocket = hSocket;

	BOOL bTrue = TRUE;
	int nZero = 0;

	setsockopt( m_hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nZero, sizeof(int));
	setsockopt( m_hSocket, SOL_SOCKET, SO_DONTLINGER, (char *)&bTrue, sizeof(BOOL));
	setsockopt( m_hSocket, SOL_SOCKET, SO_KEEPALIVE, (char *)&bTrue, sizeof(BOOL));
	setsockopt( m_hSocket, IPPROTO_TCP, TCP_NODELAY, (LPSTR)&bTrue, sizeof(BOOL));

/*
	u_long argp = 1;
	ioctlsocket(m_hSocket, FIONBIO, &argp);
*/

	m_pCQue->Clear();
	m_bIsActive = true;

	return true;
}

void CSession::PostCreate()
{
	CSockSystem::GetPeerIP( m_hSocket, NULL, m_dwIPAddr );
	CSockSystem::GetPeerPort( m_hSocket, m_wPort );	
}

bool CSession::Connect( LPCTSTR strAddr, int nPort )
{
	Close( 0, 0 );

	m_pCQue->Clear();
	
	m_hSocket = ::WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );

	if ( m_hSocket != INVALID_SOCKET ) 
	{
/*
		u_long argp = 1;
		ioctlsocket(m_hSocket, FIONBIO, &argp);
*/

		struct sockaddr_in serv_addr;
		memset (&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family      = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr( CODE_CONVERT_ANSI(strAddr) );
		serv_addr.sin_port        = htons(nPort);

		int nConnect = ::connect( m_hSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr) );

		if ( nConnect )
		{
			int nError = WSAGetLastError();

			if (nError == WSAEWOULDBLOCK)
			{
				fd_set fds;
				FD_ZERO(&fds);
				FD_SET(m_hSocket, &fds);

				struct timeval t;
				t.tv_sec = 5;
				t.tv_usec = 0;

				::select(0, NULL, &fds, NULL, &t);

				if (!FD_ISSET(m_hSocket, &fds))
				{
					::closesocket (m_hSocket);
					m_hSocket = INVALID_SOCKET;
					return false;
				}
				else
				{
					::StringCchCopy(m_strIPAddr, 256, strAddr);
					return OnConnected();
				}
			}
			else
			{
				::closesocket (m_hSocket);
				m_hSocket = INVALID_SOCKET;
				return false;
			}
		}
		else
		{
			::StringCchCopy(m_strIPAddr, 256, strAddr);
			return OnConnected();
		}
	}
	else
	{
		return false;
	}
}

void CSession::Close( int nError, int nDetail )
{
	m_bIsActive = false;
	m_bSendInProgress = false;
	m_dwIPAddr = 0;
	m_wPort = 0;
	m_wNotRecvedCount = 0;	

/*
	if ( m_hSocket != INVALID_SOCKET )
	{
		::shutdown( m_hSocket, SD_BOTH );
		::closesocket( m_hSocket );

		m_hSocket = INVALID_SOCKET;
	}
*/

/*
	if ( m_hSocket != INVALID_SOCKET )
	{
		int ncode = ::shutdown( m_hSocket, SD_BOTH );

		if( ncode != SOCKET_ERROR ) 
		{			
			fd_set	readfds; 
			fd_set	errorfds;
			timeval	timeout;

			FD_ZERO( &readfds ); 
			FD_ZERO( &errorfds ); 

			FD_SET( m_hSocket, &readfds ); 
			FD_SET( m_hSocket, &errorfds );

			timeout.tv_sec  = 0L; 
			timeout.tv_usec = 0L; 

			::select( 1, &readfds, NULL, &errorfds, &timeout ); 
		}

		::closesocket( m_hSocket );

		m_hSocket = INVALID_SOCKET;
	}
*/

	if ( m_hSocket != INVALID_SOCKET )
	{
		struct linger li = {1, 0};	
		::shutdown( m_hSocket, SD_BOTH );
		::setsockopt( m_hSocket, SOL_SOCKET, SO_LINGER, (char *)&li, sizeof(li) );

		::closesocket( m_hSocket );
		m_hSocket = INVALID_SOCKET;
	}

/*
	long BytesGot = 0;
	if ( m_hSocket != INVALID_SOCKET )
	{
		int nRval = 0;

		CSockSystem::SetNonBlockingIO( m_hSocket, FALSE );

		::shutdown( m_hSocket, SD_SEND );

		nRval = 0;
		DWORD dwReturned;
		DWORD dwPendingData = 0;
		DWORD dwRecvNumBytes = 0;
		DWORD dwFlags = 0;

		while (	nRval != SOCKET_ERROR && WSAIoctl(m_hSocket, FIONREAD, NULL, 0, (void*)dwPendingData, sizeof(DWORD), &dwReturned, NULL, NULL) != SOCKET_ERROR )
		{
			if (dwPendingData == 0)
			{
				break;
			}

			int nBufCount;
			LPWSABUF pwsabuf = m_pCQue->MakeWSABuf( &nBufCount );

			nRval = WSARecv( m_hSocket, pwsabuf, nBufCount, &dwRecvNumBytes, &dwFlags, NULL, NULL );
			if (nRval != SOCKET_ERROR)
				BytesGot += dwRecvNumBytes;
		}

		::closesocket( m_hSocket );
		m_hSocket = INVALID_SOCKET;
	}	
*/
	
	Synchronized so( &m_soSend );

	for ( netmsglistitor itor = m_ovSend.m_listMsgs.begin(); itor != m_ovSend.m_listMsgs.end(); ++itor )
		(*itor)->Release();
	
	m_ovSend.SendClear();

	for ( netmsglistitor itor = m_listMsgToSend.begin(); itor != m_listMsgToSend.end(); ++itor )
		(*itor)->Release();
	
	m_listMsgToSend.clear();	
}

bool CSession::OnConnected()
{
	m_bIsActive = true;
	return true;
}

bool CSession::OnError( DWORD dwErrorCode, int nDetail )
{
	m_bSendInProgress = false;

	if ( m_hSocket != INVALID_SOCKET )
		Close( dwErrorCode, nDetail );
	
	return true;
}

void CSession::OnErrorSession( DWORD dwErrorCode, int nDetail )
{
	if ( m_hSocket != INVALID_SOCKET )	;
//		Log( _T("Error Detected Session, CODE=%d, %d, 0x%08x, %d"), dwErrorCode, nDetail, this, ::GetCurrentThreadId() );

	if ( GetSessionManager() )
		GetSessionManager()->ErrorOnSession( this, dwErrorCode, nDetail );
	else
		OnError( dwErrorCode, nDetail );	
}

int CSession::Dispatch( DWORD& rdwBytes, LPOVERLAPPED_BASE lpov )
{
	if ( lpov == NULL )
		return ERROR_TRANS;
	
	switch ( lpov->m_dwFlag )
	{
		case IOCP_RECV :	return DispatchRecv( rdwBytes, lpov );
		case IOCP_SEND :	return DispatchSend( rdwBytes, lpov );
		case IOCP_ACCEPT :	return DispatchAccept( rdwBytes, lpov );
		case IOCP_TRANSMITFILE_COMPLETED : return DispatchTransmitFileCompleted( rdwBytes, lpov ); 
		case IOCP_TRANSMITFILE_RECV : return DispatchTransmitFileRecv( rdwBytes, lpov );
		default :	assert( false );	return ERROR_TRANS;
	}
}

int CSession::DispatchAccept( DWORD dwTransferCount, LPOVERLAPPED_BASE lpov )
{
	assert( false ); 
	return ERROR_INVALIDSOCKET;
}

int	CSession::DispatchTransmitFileRecv( DWORD dwTransferCount, LPOVERLAPPED_BASE lpov )
{
	return ERROR_NONE;
}

int	CSession::DispatchTransmitFileCompleted( DWORD dwTransferCount, LPOVERLAPPED_BASE lpov )
{
	return ERROR_NONE;
}

int CSession::DispatchRecv( DWORD dwBytes, LPOVERLAPPED_BASE lpov )
{
	assert( lpov == &m_ovRecv );

	if (dwBytes == 0)
		return ERROR_TRANS;
		
	Synchronized so( &m_soRecv );

	m_pCQue->IncTailPos( dwBytes );

	int nError = OnReceived();

	if (nError == ERROR_NONE)
		return PrepareReceive();
	else
		return nError;	
}

int CSession::OnReceived()
{
	Synchronized so( &m_soRecv );

	CMsgRecv msg( m_pCQue );

	int nSize = msg.GetSize();
	int nError;

	while( (m_pCQue->GetDataSize() >= 6) && (m_pCQue->GetDataSize() >= nSize) )
	{
		msg.Decrypt();
		nError = ProcessMessage(msg);

		if (nError != ERROR_NONE)
			return nError;

		m_pCQue->IncHeadPos( nSize );

		msg.Clear();
		nSize = msg.GetSize();
	}

	return ERROR_NONE;
}

int CSession::PrepareReceive( DWORD dwFlag )
{
	DWORD dwReadSize = 0;	
	BOOL bResult;

	memset(&m_ovRecv, 0, sizeof(OVERLAPPED_BASE));
	m_ovRecv.m_dwFlag = dwFlag;

	int nBufCount;
	LPWSABUF pwsabuf = m_pCQue->MakeWSABuf( &nBufCount );
	DWORD dwFlags = 0;

	bResult = WSARecv( m_hSocket, pwsabuf, nBufCount, &dwReadSize, &dwFlags, ( LPOVERLAPPED )&m_ovRecv, NULL );

	if ( bResult == SOCKET_ERROR )
	{
		int nLastError = WSAGetLastError();

		if ( nLastError == WSA_IO_PENDING )
		{
			return ERROR_NONE;
		}
		else
		{
			OnError( ERROR_RECV, nLastError );
			return ERROR_RECV;
		}
	}

	return ERROR_NONE;
}

void CSession::SendAliveReq()
{	
	CMsgSend msgSend;
	msgSend.ID(/*RS_ALIVE*/0) << 0 << _T("TEST");
	SendMessage( msgSend );
}

inline int CSession::SendMessage( CMsgSend& msgSend, bool bEncrypt ) 
{
	if ( bEncrypt )
		msgSend.Encrypt();

	return Send( msgSend.m_pNetMsg );
}

int CSession::Send( CNetMsg* pNetMsg )
{
	if ( IsActive() )
	{
		assert( pNetMsg->m_nBufCount < NETWORK_SENDBUFFER_SIZE );
		
		pNetMsg->AddRef();

		Synchronized sa( &m_soSend );

		if ( m_bSendInProgress )
		{
			m_listMsgToSend.push_back( pNetMsg );
		}
		else
		{
			m_ovSend.SendClear();
			m_ovSend.AddNetMsg( pNetMsg );

			DWORD dwBytesWritten;

			int nError = WSASend( m_hSocket, m_ovSend.m_wsaBuf, m_ovSend.GetMsgCount(), &dwBytesWritten, 0, (LPOVERLAPPED)&m_ovSend, NULL );
			
			if ( nError == SOCKET_ERROR )
			{
				int nLastError = WSAGetLastError();

				if ( nLastError != WSA_IO_PENDING && nLastError != ERROR_SUCCESS )
				{
					m_bIsActive = false;
//					Log( _T("Error Detected Session @ Send, CODE=%d, %d"), ERROR_SEND, nLastError );
					return ERROR_SEND;
				}
			}

			m_bSendInProgress = true;
		}

		return ERROR_NONE;
	}
	else
	{
		return ERROR_SEND;
	}
}

int	CSession::DispatchSend( DWORD dwBytes, LPOVERLAPPED_BASE lpov )
{
	assert( lpov == &m_ovSend );

	if ( !IsActive() )
		return ERROR_SEND;

	Synchronized sa( &m_soSend );

	m_bSendInProgress = false;

	m_ovSend.ProcessSendedBytes( dwBytes );
	
	while ( !m_listMsgToSend.empty() )
	{
		if ( !m_ovSend.AddNetMsg( m_listMsgToSend.front() ) )
		{
			break;
		}

		m_listMsgToSend.pop_front();
	}

	if ( m_ovSend.GetMsgCount() )
	{
		m_bSendInProgress = true;
		
		DWORD dwBytesWritten;

		int nError = WSASend( m_hSocket, &m_ovSend.m_wsaBuf[0], m_ovSend.GetMsgCount(), &dwBytesWritten, 0, (LPOVERLAPPED)&m_ovSend, NULL );
		
		if ( nError == SOCKET_ERROR )
		{
			int nLastError = WSAGetLastError();
			if ( nLastError != WSA_IO_PENDING && nLastError != ERROR_SUCCESS )
			{
				m_bIsActive = false;
				return ERROR_SEND;
			}
		}
	}		

	return 0;
}

void CSession::GetErrorMessage( LPTSTR ErrMsg )
{
	LPVOID lpMsgBuf;

	::FormatMessage	
		(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
		);
	
	lstrcpy( ErrMsg, (LPCTSTR)lpMsgBuf );		
	::LocalFree( lpMsgBuf );
}

END_NAMESPACE