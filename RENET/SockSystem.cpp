#include "StdAfx.h"
#include "SockSystem.h"

START_NAMESPACE

#define MTU_SIZE	1460

LPFN_ACCEPTEX				CSockSystem::_AcceptEx = NULL;
LPFN_GETACCEPTEXSOCKADDRS	CSockSystem::_GetAcceptExSockAddrs = NULL;
LPFN_TRANSMITFILE			CSockSystem::_TransmitFile = NULL;

CSockSystem::CSockSystem()
{
	m_bInitialized = FALSE;
}

CSockSystem::~CSockSystem()
{
	ReleaseWinsock();
}

BOOL CSockSystem::Create( BYTE Major, BYTE Minor )
{
	if ( InitWinsock( Major, Minor ) == FALSE )
		return FALSE;

	return SetWinsock2Extensions();
}

BOOL CSockSystem::GetLocalIP( TCHAR* lpszIP, DWORD& dwIP )
{
	SOCKADDR_IN server_sin;
	PHOSTENT phe;

	phe = gethostbyname("localhost");
	if (phe == NULL) 
		return FALSE;

	phe = gethostbyname(phe->h_name);
	if (phe == NULL) 
		return FALSE;

	::memcpy(&server_sin.sin_addr, phe->h_addr, phe->h_length);
	dwIP = server_sin.sin_addr.S_un.S_addr;

	if (lpszIP)
	{
//		sprintf(lpszIP, "%d.%d.%d.%d", (dwIP >>  0) & 0x000000ff, (dwIP >>  8) & 0x000000ff, (dwIP >> 16) & 0x000000ff, (dwIP >> 24) & 0x000000ff);
		lstrcpy( lpszIP, CODE_CONVERT_WIDE( inet_ntoa(server_sin.sin_addr) ) );
	}

	return TRUE;
}

BOOL CSockSystem::GetPeerIP( SOCKET hHandle, TCHAR* lpszPeerIP, DWORD& dwIP )
{
	SOCKADDR_IN addrPeer;
	::ZeroMemory( &addrPeer, sizeof(addrPeer) );
	int	len = sizeof(addrPeer);

	int rval = getpeername( hHandle, (SOCKADDR*)&addrPeer, &len );
	if (rval == SOCKET_ERROR)
		return FALSE;

	dwIP = (DWORD)addrPeer.sin_addr.S_un.S_addr;

	if ( lpszPeerIP )
		lstrcpy( lpszPeerIP, CODE_CONVERT_WIDE( inet_ntoa(addrPeer.sin_addr) ) );

	return TRUE;
}

BOOL CSockSystem::GetPeerPort( SOCKET hHandle, WORD& wPort )
{
	SOCKADDR_IN addrPeer;
	::ZeroMemory( &addrPeer, sizeof(addrPeer) );
	int	len = sizeof(addrPeer);

	int rval = getpeername( hHandle, (SOCKADDR*)&addrPeer, &len );
	if (rval == SOCKET_ERROR)
		return FALSE;

	wPort = ntohs( addrPeer.sin_port );

	return TRUE;
}

DWORD CSockSystem::GetAddressFromName( LPCTSTR name )
{
	if ( name == NULL )
		return 0;

	if( _tcslen( name ) == 0 )
		return 0;

	DWORD addr = ::inet_addr( CODE_CONVERT_ANSI(name) );

	// DNS를 통해서 물어본다. 대게의 경우 인터넷이 먹통되서 문제가 생기기 때문에. 여기서 속도가 무지 느릴 수도 있다
	if ( addr == INADDR_NONE )	
	{
		PHOSTENT pHostEnt = ::gethostbyname( CODE_CONVERT_ANSI(name) );

		if (pHostEnt == NULL)
			return INADDR_NONE;

		if (pHostEnt->h_length != 4)
			return INADDR_NONE;

		return *(DWORD*)pHostEnt->h_addr_list[0];
	}

	return addr;
}

BOOL CSockSystem::SetWinsock2Extensions()
{
	SOCKET hDummySock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP); 
	
	int rVal;
	DWORD dwBytesReturned = 0;
	DWORD dwIOControlCode = SIO_GET_EXTENSION_FUNCTION_POINTER;
	HINSTANCE hMSwsock = NULL;

	// for AcceptEx()
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	rVal = WSAIoctl( hDummySock, dwIOControlCode, (void*)&guidAcceptEx, sizeof(guidAcceptEx), (void*)&_AcceptEx, sizeof(LPFN_ACCEPTEX),
		&dwBytesReturned, NULL, NULL );

	if ( rVal == SOCKET_ERROR )
	{
		hMSwsock = LoadLibrary(_T("mswsock.dll"));
		_AcceptEx = (LPFN_ACCEPTEX)::GetProcAddress( hMSwsock, "AcceptEx" );
		if (!_AcceptEx)
		{
			WSAERROR(TEXT("SetWinsock2Extensions() - Getting AcceptEx()"), TEXT("CSockSystem::SetWinsock2Extensions"));
			::closesocket(hDummySock);
			return FALSE;
		}
	}

	// for GetAcceptExSockAddrs()
	GUID guidAddr = WSAID_GETACCEPTEXSOCKADDRS;
	rVal = WSAIoctl( hDummySock, dwIOControlCode, (void*)&guidAddr, sizeof(guidAddr), (void*)&_GetAcceptExSockAddrs, 
		sizeof(LPFN_GETACCEPTEXSOCKADDRS), &dwBytesReturned, NULL, NULL );
	if ( rVal == SOCKET_ERROR )
	{
		_GetAcceptExSockAddrs = (LPFN_GETACCEPTEXSOCKADDRS)::GetProcAddress( hMSwsock, "GetAcceptExSockaddrs" );
		if (!_GetAcceptExSockAddrs)
		{
			WSAERROR(TEXT("SetWinsock2Extensions() - Getting GetAcceptExSockAddrs()"), TEXT("CSockSystem::SetWinsock2Extensions"));
			::closesocket(hDummySock);
			return FALSE;
		}
	}

	// for TrasmitFile()
	GUID guidTransmitFile = WSAID_TRANSMITFILE;
	rVal = WSAIoctl( hDummySock, dwIOControlCode, (void*)&guidTransmitFile, sizeof(guidTransmitFile), (void*)&_TransmitFile, 
		sizeof(LPFN_TRANSMITFILE), &dwBytesReturned, NULL, NULL );
	if (rVal == SOCKET_ERROR)
	{
		_TransmitFile = (LPFN_TRANSMITFILE)::GetProcAddress( hMSwsock, "TransmitFile" );
		if (!_TransmitFile)
		{
			WSAERROR(TEXT("SetWinsock2Extensions() - Getting TransmitFile()"), TEXT("CSockSystem::SetWinsock2Extensions"));
			::closesocket(hDummySock);
			return FALSE;
		}
	}

	if (hMSwsock)
		FreeLibrary(hMSwsock);

	::closesocket(hDummySock);
	return TRUE;
}

BOOL CSockSystem::InitWinsock(BYTE Major, BYTE Minor)
{
	if ( m_bInitialized )
		return TRUE;

	WSADATA wsaData;
	WORD wVersionReq = MAKEWORD(Major, Minor);
	int nResult = ::WSAStartup(wVersionReq, &wsaData);
	if (nResult != 0)
		return FALSE;

	if ( LOBYTE(wsaData.wVersion) != Major || HIBYTE(wsaData.wVersion) != Minor )
	{
		WSACleanup();
		return FALSE;
	}

	m_bInitialized = TRUE;

	return TRUE;
}

void CSockSystem::ReleaseWinsock()
{
	if ( m_bInitialized )
	{
		int rval = WSACleanup();
		if (rval != 0)
			ShowWSAErrorString(_T("ReleaseSocket()"), _T("CSockSystem::ReleaseWinsock"));

		m_bInitialized = FALSE;
	}
}

BOOL CSockSystem::TurnOffBuffer( SOCKET hSock, BOOL bSendBuffer, BOOL bRecvBuffer )
{
	int nZero = 0;

	if ( bSendBuffer == TRUE )
	{
		int nRet = ::setsockopt( hSock, SOL_SOCKET, SO_SNDBUF, (char*)&nZero, sizeof(nZero) );
		if (SOCKET_ERROR == nRet) 
		{
			WSAERROR(TEXT("TurnOffWinsockBuffer() - Send Buffer"), TEXT("CSockSystem::TurnOffBuffer"));
			return FALSE;
		}
	}

/*
	Recv 버퍼를 끄는 것은 자칫 엄청난 성능 저하를 초래할 수 있다.
	Recv 버퍼를 0으로 한 상태에서 Posting된 Recv마저 없다면 TCP 스택이
	window size도 0으로 만들어버리기 땜에 recv를 Posting 하기 전까지는 상대편에서
	아예 send조차 할 수도 없기 때문이다.
	if ( bRecvBuffer == TRUE )
	{
		nZero = 0;
		long nRet = setsockopt( hSock, SOL_SOCKET, SO_RCVBUF, (char*)&nZero, sizeof(nZero) );
		if (nRet == SOCKET_ERROR) 
		{
			WSAERROR( TEXT("TurnOffWinsockBuffer() - Recv Buffer"), TEXT("CSockSystem::TurnOffBuffer") );
			return FALSE;
		}
	}
*/
	return TRUE;
}

long CSockSystem::SetMaxBuffer( SOCKET hSock, long BigBufSize, long nWhichBuffer )
{
	long nRet = 0, nTrySize = 0, nFinalSize = 0;

	for (nTrySize = BigBufSize; nTrySize > MTU_SIZE; nTrySize >>= 1)
	{
		nRet = setsockopt(hSock, SOL_SOCKET, nWhichBuffer, (char*)&nTrySize, sizeof(long));
		if (nRet == SOCKET_ERROR)
		{
			int WSAErr = WSAGetLastError();
			if ((WSAErr == WSAENOPROTOOPT) || (WSAErr == WSAEINVAL))
				break;
			else
			{
				nRet = sizeof(long);
				getsockopt(hSock, SOL_SOCKET, nWhichBuffer, (char*)&nFinalSize, (int*)&nRet);
				break;
			}
		}
	}

	return nFinalSize;
}

BOOL CSockSystem::SetLinger(SOCKET hSock, WORD bTurnOn, WORD wTimeout /* in second */)
{
	LINGER lingerStruct;
	lingerStruct.l_onoff = bTurnOn;
	lingerStruct.l_linger = wTimeout;

	int nRet = setsockopt(hSock, SOL_SOCKET, SO_LINGER, (char*)&lingerStruct, sizeof(lingerStruct));
	if (nRet == SOCKET_ERROR) 
	{
		WSAERROR(TEXT("SetLinger()"), TEXT("CSockSystem::SetLinger"));
		return FALSE;
	}

	return TRUE;
}

BOOL CSockSystem::SetNonBlockingIO(SOCKET hSock, BOOL bSetNonBlock)
{
	return TRUE;

	// Set Non-blocking mode
	u_long argp = bSetNonBlock;

	int nRet = ::ioctlsocket( hSock, FIONBIO, (u_long*)&argp );
	if (nRet == SOCKET_ERROR)
	{
		WSAERROR( TEXT("SetNonBlockingIO()"), TEXT("CSockSystem::SetNonBlockingIO") );
		return FALSE;
	}

	return TRUE;
}

void CSockSystem::AddressToString( std::wstring& str, SOCKADDR_IN* paddr )
{
	wchar_t szBuf[0xff] = {0, };
	StringCbPrintfW( szBuf, 0xff, L"[IP : %d.%d.%d.%d Port : %d]",
		paddr->sin_addr.S_un.S_un_b.s_b1,
		paddr->sin_addr.S_un.S_un_b.s_b2,
		paddr->sin_addr.S_un.S_un_b.s_b3,
		paddr->sin_addr.S_un.S_un_b.s_b4,
		ntohs( paddr->sin_port ) );

	str = szBuf;	
}

void CSockSystem::AddressToString( std::string& str, SOCKADDR_IN* paddr )
{
	char szBuf[0xff] = {0, };
	StringCbPrintfA( szBuf, 0xff, "[IP : %d.%d.%d.%d Port : %d]",
		paddr->sin_addr.S_un.S_un_b.s_b1,
		paddr->sin_addr.S_un.S_un_b.s_b2,
		paddr->sin_addr.S_un.S_un_b.s_b3,
		paddr->sin_addr.S_un.S_un_b.s_b4,
		ntohs( paddr->sin_port ) );

	str = szBuf;	
}

// Error Displayer
int CSockSystem::ShowWSAErrorString( LPCTSTR lpszDesc, LPCTSTR lpszFunc )
{
	int WSAErr = WSAGetLastError();

#ifdef _DEBUG
	TCHAR ErrBuf[512];
	TCHAR ErrMsg[512];

	WSAErrStr( WSAErr, ErrMsg );
	wsprintf( ErrBuf, _T("%s (%s)\n"), lpszDesc, ErrMsg );

	OutputDebugString( ErrBuf );
#endif

	return WSAErr;
}

void CSockSystem::ShowWSAErrorString(int WSAErr, LPCTSTR lpszDesc, LPCTSTR lpszFunc)
{
#ifdef _DEBUG
	TCHAR ErrBuf[512];
	TCHAR ErrMsg[512];

//	WSAErrStr( WSAErr, ErrMsg );
	ErrorCode2Text( WSAErr, ErrMsg );
	wsprintf( ErrBuf, _T("%s (%s)\n"), lpszDesc, ErrMsg );

	OutputDebugString(ErrBuf);
#endif
}

void CSockSystem::ErrorCode2Text( int Err, LPTSTR ErrMsg )
{
	LPTSTR lpstrErr;
	
	switch(Err)
	{
		case WSAEFAULT:			lpstrErr = _T("WSAEFAULT : The buf parameter is not completely contained in a valid part of the user address space."); break; 
		case WSAENOTCONN:		lpstrErr = _T("WSAENOTCONN : The socket is not connected."); break;
		case WSAEINTR:			lpstrErr = _T("WSAEINTR : The (blocking) call was canceled through WSACancelBlockingCall."); break;
		case WSAENOTSOCK:		lpstrErr = _T("WSAENOTSOCK : The descriptor s is not a socket."); break; 
		case WSANOTINITIALISED: lpstrErr = _T("WSANOTINITIALISED: A successful WSAStartup call must occur before using this function."); break; 
		case WSAENETDOWN:		lpstrErr = _T("WSAENETDOWN : The network subsystem has failed."); break;
		case WSAEINPROGRESS:	lpstrErr = _T("WSAEINPROGRESS : A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function."); break;
		case WSAENETRESET:		lpstrErr = _T("WSAENETRESET : The connection has been broken due to the keep-alive activity detecting a failure while the operation was in progress."); break; 
		case WSAEOPNOTSUPP:		lpstrErr = _T("WSAEOPNOTSUPP : MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only send operations."); break; 
		case WSAESHUTDOWN:		lpstrErr = _T("WSAESHUTDOWN : The socket has been shut down; it is not possible to receive on a socket after shutdown has been invoked with how set to SD_RECEIVE or SD_BOTH."); break;
		case WSAEWOULDBLOCK:	lpstrErr = _T("WSAEWOULDBLOCK : The socket is marked as nonblocking and the receive operation would block."); break; 
		case WSAEMSGSIZE:		lpstrErr = _T("WSAENOTSOCK : The message was too large to fit into the specified buffer and was truncated."); break;
		case WSAEINVAL:			lpstrErr = _T("WSAEINVAL :	The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled or (for byte stream sockets only) len was zero or negative."); break;
		case WSAECONNABORTED:	lpstrErr = _T("WSAECONNABORTED : The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable."); break; 
		case WSAETIMEDOUT:		lpstrErr = _T("WSAETIMEDOUT : The connection has been dropped because of a network failure or because the peer system failed to respond."); break; 
		case WSAECONNRESET:		lpstrErr = _T("WSAECONNRESET : Connection dropped.."); break;
		default:				lpstrErr = _T("");  break;
	}

	if ( lpstrErr != NULL )
		lstrcpy( ErrMsg, lpstrErr );
	else
	{
		LPVOID lpMsgBuf;
		FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
						NULL,
						Err,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(LPTSTR) &lpMsgBuf,
						0,
						NULL );
		lstrcpy( ErrMsg, (LPCTSTR)lpMsgBuf );		
	}	
}

void CSockSystem::WSAErrStr( int Err, LPTSTR ErrMsg )
{
#ifdef _DEBUG
	LPTSTR lpstrErr;

	switch ( Err )
	{
		case WSABASEERR:			lpstrErr = _T("No Error");											break;
		case WSAEINTR:				lpstrErr = _T("Interruped System Call");							break;
		case WSAEBADF:				lpstrErr = _T("Bad File Number");									break;
		case WSAEACCES:				lpstrErr = _T("Permission Denied");									break;
		case WSAEFAULT:				lpstrErr = _T("Bad Address");										break;
		case WSAEINVAL:				lpstrErr = _T("Invalid Argument");									break;
		case WSAEMFILE:				lpstrErr = _T("Too many open files");								break;
		case WSAEWOULDBLOCK:		lpstrErr = _T("Operation would block");								break;
		case WSAEINPROGRESS:		lpstrErr = _T("Operation now in progress");							break;
		case WSAEALREADY:			lpstrErr = _T("Operation already in progress");						break;
		case WSAENOTSOCK:			lpstrErr = _T("Socket operation on Non-Socket");					break;
		case WSAEDESTADDRREQ:		lpstrErr = _T("Destination address required");						break;
		case WSAEMSGSIZE:			lpstrErr = _T("Message too long");									break;
		case WSAEPROTOTYPE:			lpstrErr = _T("Protocol wrong type for socket");					break;
		case WSAENOPROTOOPT:		lpstrErr = _T("Bad Protocol option");								break;
		case WSAEPROTONOSUPPORT:	lpstrErr = _T("Protocol not supported");							break;
		case WSAESOCKTNOSUPPORT:	lpstrErr = _T("Socket type not supported");							break;
		case WSAEOPNOTSUPP:			lpstrErr = _T("Operation not supported on socket");					break;
		case WSAEPFNOSUPPORT:		lpstrErr = _T("Protocol family not supported");						break;
		case WSAEAFNOSUPPORT:		lpstrErr = _T("Address Family not supported by protocol family");	break;
		case WSAEADDRINUSE:			lpstrErr = _T("Address already in use");							break;
		case WSAEADDRNOTAVAIL:		lpstrErr = _T("Can't assign requested address");					break;
		case WSAENETDOWN:			lpstrErr = _T("Network is down");									break;
		case WSAENETUNREACH:		lpstrErr = _T("Network is unreachable");							break;
		case WSAENETRESET:			lpstrErr = _T("Net dropped connection or reset");					break;
		case WSAECONNABORTED:		lpstrErr = _T("Software caused connection abort");					break;
		case WSAECONNRESET:			lpstrErr = _T("connection reset by peer");							break;
		case WSAENOBUFS:			lpstrErr = _T("No buffer space available");							break;
		case WSAEISCONN:			lpstrErr = _T("Socket is Already connected");						break;
		case WSAENOTCONN:			lpstrErr = _T("Socket is not connected");							break;
		case WSAESHUTDOWN:			lpstrErr = _T("Can't send after socket shutdown");					break;
		case WSAETOOMANYREFS:		lpstrErr = _T("Too many references, can't splice");					break;
		case WSAETIMEDOUT:			lpstrErr = _T("Connection timed out");								break;
		case WSAECONNREFUSED:		lpstrErr = _T("Connection refused");								break;
		case WSAELOOP:				lpstrErr = _T("Too many levels of symbolic links");					break;
		case WSAENAMETOOLONG:		lpstrErr = _T("File name too long");								break;
		case WSAEHOSTDOWN:			lpstrErr = _T("Host is down");										break;
		case WSAEHOSTUNREACH:		lpstrErr = _T("No route to host");									break;
		case WSAENOTEMPTY:			lpstrErr = _T("Directory not empty");								break;
		case WSAEPROCLIM:			lpstrErr = _T("Too many processes");								break;
		case WSAEUSERS:				lpstrErr = _T("Too many users");									break;
		case WSAEDQUOT:				lpstrErr = _T("Disc Quota Exceeded");								break;
		case WSAESTALE:				lpstrErr = _T("Stale NFS file handle");								break;
		case WSAEREMOTE:			lpstrErr = _T("Too many levels of remote in path");					break;
		case WSASYSNOTREADY:		lpstrErr = _T("Network Subsystem is unavailable");					break;
		case WSAVERNOTSUPPORTED:	lpstrErr = _T("Winsock DLL Version out of range");					break;
		case WSANOTINITIALISED:		lpstrErr = _T("Successful WSASTARUP not yet performed");			break;
		case WSAHOST_NOT_FOUND:		lpstrErr = _T("Host not found");									break;
		case WSATRY_AGAIN:			lpstrErr = _T("Non-Authoritative host not found");					break;
		case WSANO_RECOVERY:		lpstrErr = _T("Non-Recoverable errors: FORMERR, REFUSED, NOTIMP");	break;
		case WSANO_DATA:			lpstrErr = _T("Valid name, do data record of requested type");		break;
		case WSA_IO_PENDING:		lpstrErr = _T("io pending");										break;
		default:					lpstrErr = NULL;													break;
	}

	if ( lpstrErr != NULL )
		lstrcpy( ErrMsg, lpstrErr );
	else
		wsprintf( ErrMsg, _T("Unknown Error: %d"), Err );
#endif
}

END_NAMESPACE