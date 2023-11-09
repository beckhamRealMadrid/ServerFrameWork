#pragma once

#include "Singleton.h"

START_NAMESPACE

typedef BOOL (PASCAL FAR* LPFN_ACCEPTEX)(
    SOCKET sListenSocket,
	SOCKET sAcceptSocket,
	PVOID lpOutputBuffer,
	DWORD dwReceiveDataLength,
	DWORD dwLocalAddressLength,
	DWORD dwRemoteAddressLength,
	LPDWORD lpdwBytesReceived,
	LPOVERLAPPED lpOverlapped);

typedef VOID(PASCAL FAR* LPFN_GETACCEPTEXSOCKADDRS)(
    PVOID lpOutputBuffer,       
	DWORD dwReceiveDataLength,  
	DWORD dwLocalAddressLength,  
	DWORD dwRemoteAddressLength,  
	LPSOCKADDR *LocalSockaddr,  
	LPINT LocalSockaddrLength,  
	LPSOCKADDR *RemoteSockaddr,  
	LPINT RemoteSockaddrLength);

typedef BOOL (PASCAL FAR* LPFN_TRANSMITFILE)(
    SOCKET hSocket,
	HANDLE hFile,
	DWORD nNumberOfBytesToWrite,
	DWORD nNumberOfBytesPerSend,
	LPOVERLAPPED lpOverlapped,  
	LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
	DWORD dwFlags);

class RENET_API CSockSystem : public CSingleton <CSockSystem>
{
public:
					CSockSystem();
	virtual			~CSockSystem();

protected:
			BOOL	m_bInitialized;
protected:
			BOOL	InitWinsock( BYTE Major, BYTE Minor );
public:
			BOOL	Create( BYTE Major  = 2, BYTE Minor = 2 );
			void	ReleaseWinsock();
	static 	BOOL	GetLocalIP( TCHAR* lpszPeerIP, DWORD& dwIP );
	static 	BOOL	GetPeerIP( SOCKET hHandle, TCHAR* lpszPeerIP, DWORD& dwIP );
	static	BOOL	GetPeerPort( SOCKET hHandle, WORD& wPort );
	static 	DWORD	GetAddressFromName( LPCTSTR name );
	static 	BOOL	SetWinsock2Extensions();
	static 	BOOL	TurnOffBuffer( SOCKET hSock, BOOL bSendBuffer = TRUE, BOOL bRecvBuffer = FALSE );
	static 	long	SetMaxBuffer( SOCKET hSock, long BigBufSize, long nWhichBuffer );
	static 	BOOL	SetLinger( SOCKET hSock, WORD bTurnOn = 1, WORD wTimeout = 0 /* in second */ );
	static 	BOOL	SetNonBlockingIO( SOCKET hSock, BOOL bSetNonBlock = TRUE );
	static 	int		ShowWSAErrorString( LPCTSTR lpszDesc,LPCTSTR lpszFunc );
	static 	void	ShowWSAErrorString( int WSAErr, LPCTSTR lpszDesc,LPCTSTR lpszFunc );
	static  void	ErrorCode2Text( int Err, LPTSTR ErrMsg );
	static 	void	WSAErrStr( int Err, LPTSTR ErrMsg );
	static	void	AddressToString( std::wstring& str, SOCKADDR_IN* paddr );
	static	void	AddressToString( std::string& str, SOCKADDR_IN* paddr );
public:
	static  LPFN_ACCEPTEX				_AcceptEx;
	static  LPFN_GETACCEPTEXSOCKADDRS	_GetAcceptExSockAddrs;
	static  LPFN_TRANSMITFILE			_TransmitFile;
};

#ifdef _DEBUG
#	define WSAERROR(a,fn)			(CSockSystem::ShowWSAErrorString(a,fn))
#	define WSAERROR2(a,b,fn)		(CSockSystem::ShowWSAErrorString(a,b,fn))
#else
#	define WSAERROR(a,fn)			WSAGetLastError()
#	define WSAERROR2(a,b,fn)		WSAGetLastError()
#endif

END_NAMESPACE
