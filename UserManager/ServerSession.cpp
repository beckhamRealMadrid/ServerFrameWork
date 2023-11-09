#include "StdAfx.h"
#include "ServerSession.h"
#include "SessionManager.h"
#include "../Common/Protocol.h"

CServerSession::CServerSession( CSessionManager* pSM ) : CSession( pSM )
{

}

CServerSession::~CServerSession(void)
{

}

bool CServerSession::Create(SOCKET hSocket)
{
	return CSession::Create( hSocket );
}

BEGIN_MESSAGEMAP( CServerSession )	
	ON_MESSAGE( RSNET_LOGIN_REQ, &CServerSession::OnLogInReq )
END_MESSAGEMAP()

void CServerSession::OnLogInReq( CMsgRecv& msg )
{
	char szUserID[50] = {0,};
	char szPasswd[50] = {0,};
	char szIP[50] = {0,};
	DWORD dwSessKey = 0;
	
	msg.ReadString( szUserID, 50 );
	msg.ReadString( szPasswd, 50 );
	msg.ReadString( szIP, 50 );
	msg >> dwSessKey;

	CMsgSend MsgSend;
	MsgSend.ID( RSNET_LOGIN_ACK ) << szUserID << dwSessKey << ERROR_GOOD;
	SendMessage( MsgSend );
}