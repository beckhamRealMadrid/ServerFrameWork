#include "stdafx.h"
#include "ListeningSession.h"
#include "SessionManager.h"
#include "IOCPManager.h"

START_NAMESPACE

CListeningSession::CListeningSession( CSessionManager* pSM ) : CSession( pSM )
{
	m_hAcceptSocket = INVALID_SOCKET;	
}

CListeningSession::~CListeningSession()
{
	if ( m_hAcceptSocket != INVALID_SOCKET )
	{
		closesocket( m_hAcceptSocket );
	}
}

const SOCKET CListeningSession::GetAcceptSocket()
{
	return m_hAcceptSocket;
}

bool CListeningSession::Create( LPCTSTR szAddr, int nPort )
{
	::StringCchCopy( m_strIPAddr, 256, szAddr );
	m_nPort = nPort;

	struct sockaddr_in serv_addr;
	memset (&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	if (szAddr)
		serv_addr.sin_addr.s_addr = inet_addr( CODE_CONVERT_ANSI(m_strIPAddr) );
	else
		serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
	serv_addr.sin_port = htons(m_nPort);

	m_hSocket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );

	if ( m_hSocket != INVALID_SOCKET )
	{
		bool bTrue = true;
		bool bFalse = false;
		int nZero = 0;
		int nOne = 1;

		setsockopt( m_hSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&nOne, sizeof(int) );
		setsockopt( m_hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nZero, sizeof(int) );
		setsockopt( m_hSocket, SOL_SOCKET, SO_DONTLINGER, (char *)&bTrue, sizeof(BOOL) );

/*
		u_long argp = 1;
		ioctlsocket(m_hSocket, FIONBIO, &argp);
*/

		if ( bind( m_hSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr) ) != SOCKET_ERROR )
		{
			if ( listen( m_hSocket, SOMAXCONN ) != SOCKET_ERROR )
			{
				m_pCQue->Clear();
				Log( _T("listen IPAddr : %s Port : %d"), szAddr, nPort );
				return true;
			}
			else
			{
				Log( _T("Listening socket creation failed on Listen()") );
			}
		}
		else
		{
			Log( _T("Listening socket creation failed on bind()") );
		}
	}
	else
	{
		Log( _T("Listening socket creation failed on socket()") );
	}

	return false;
}

int CListeningSession::Dispatch( DWORD& rdwDispatchCount, LPOVERLAPPED_BASE lpov )
{
	if ( lpov == NULL )
		return ERROR_TRANS;

	switch ( lpov->m_dwFlag )
	{
		case IOCP_ACCEPT :	return DispatchAccept( rdwDispatchCount, lpov );
		default :
			assert( false );
			Log( _T("!!!!! Dispatch Accept Session for NOT ACCEPT !!!!!") );
			return ERROR_NONE;
	}
}

int CListeningSession::DispatchAccept( DWORD dwTransferCount, LPOVERLAPPED_BASE lpov )
{
	int optsize = 4;
	int def_send_size = 0;
	int def_recv_size = 0;

	// »÷µå ¼ÒÄ¹¹öÆÛ°¡ 0ÀÌ µÈ´Ù.
	setsockopt( m_hAcceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&m_hSocket, sizeof(m_hSocket) );

	LPSOCKADDR_IN saLocal, saRemote;
	int nSALocal = sizeof(SOCKADDR_IN);
	int nSARemote = sizeof(SOCKADDR_IN);

	GetAcceptExSockaddrs( m_pCQue->GetBuf(), dwTransferCount, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		(LPSOCKADDR *)&saLocal, &nSALocal, (LPSOCKADDR *)&saRemote, &nSARemote );

	TCHAR szLocalAddr[256] = {0, };
	TCHAR szRemoteAddr[256] = {0, };

	::StringCchCopy( szLocalAddr, 256, CODE_CONVERT_WIDE( inet_ntoa(((LPSOCKADDR_IN)saLocal)->sin_addr) ) );
	::StringCchCopy( szRemoteAddr, 256, CODE_CONVERT_WIDE( inet_ntoa(((LPSOCKADDR_IN)saRemote)->sin_addr) ) );

//	Log("Socket Accepted Local = %s, %d, Remote = %s, %d", szLocalAddr, nSALocal, szRemoteAddr, nSARemote);

	int nError = m_pSessionManager->AcceptSocket( m_hAcceptSocket, szRemoteAddr );

	return PrepareAccept();
}

int CListeningSession::PrepareAccept() 
{
	DWORD dwRecvBytes = 0;
	
	m_hAcceptSocket = socket(AF_INET, SOCK_STREAM, 0);

/*
	u_long argp = 1;
	ioctlsocket(m_hAcceptSocket, FIONBIO, &argp);
*/

	memset(&m_ovRecv.m_ov, 0, sizeof(OVERLAPPED_BASE));
	m_ovRecv.m_dwFlag = IOCP_ACCEPT;

	BOOL bResult = ::AcceptEx( m_hSocket, m_hAcceptSocket, m_pCQue->GetBuf(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&dwRecvBytes, (LPOVERLAPPED)&m_ovRecv );

	if (!bResult)
	{
		int nError = WSAGetLastError();

		if ( nError = ERROR_IO_PENDING || nError == ERROR_SUCCESS )
		{
			return ERROR_NONE;
		}
		else
		{
			Log( _T("Error in AcceptEx error = %d"), nError );
			return ERROR_ACCEPT;
		}
	}

	return ERROR_NONE;
}

void CListeningSession::Close( int nError, int nDetail )
{
	if ( m_hAcceptSocket != INVALID_SOCKET )
	{
		int ncode = ::shutdown( m_hAcceptSocket, SD_BOTH );

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

		::closesocket( m_hAcceptSocket );

		m_hAcceptSocket = INVALID_SOCKET;
	}

	CSession::Close( nError, nDetail );
}

int CListeningSession::PrepareReceive()
{
	assert(false);
	Log( _T("!!! Receive for Listening Session ??? !!!!") );
	return ERROR_NONE;
}

int	CListeningSession::SendMessage( CMsgSend& msgSend, bool bEncrypt )
{
	assert(false);
	Log( _T("!!! Send Message for Listening Session ??? !!!!") );
	return ERROR_NONE;
}

bool CListeningSession::OnError( DWORD dwErrorCode, int nDetail )
{
	Log( _T("Listening Session Error %d, %d"), dwErrorCode, nDetail );

	if ( PrepareAccept() != ERROR_NONE )
	{
		Log( _T("PrepareAccept() Failed, so Recreate listening session") );

		Close( dwErrorCode, nDetail );

		if ( Create( m_strIPAddr, m_nPort ) )
		{
			if ( IOCP->AddIOPort( (HANDLE)GetSocket(), (DWORD)this ) )
			{
				if ( PrepareAccept() == ERROR_NONE )
				{
					Log( _T("Listening Session Recreation Succeed, Addr = %s, Port = %d"), m_strIPAddr, m_nPort );
				}
				else
				{
					Log( _T("Listening session recreation failed on PrepareAccept(), Addr = %s, Port = %d"), m_strIPAddr, m_nPort );
				}
			}
			else
			{
				Log( _T("Listening session recreation failed on AddIOPort(), Addr = %s, Port = %d"), m_strIPAddr, m_nPort );
			}
		}
		else
		{
			Log( _T("Listening session recreation failed on Create(), Addr = %s, Port = %d"), m_strIPAddr, m_nPort );
		}
	}

	return false;
}

END_NAMESPACE