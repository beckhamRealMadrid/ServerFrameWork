#include "StdAfx.h"
#include "ServerSession.h"
#include "SupervisorService.h"
#include "../Common/Protocol.h"

CServerSession::CServerSession( CSessionManager* pSM ) : CSession( pSM )
{
	
}

CServerSession::~CServerSession(void)
{

}

void CServerSession::PostCreate()
{
	CSession::PostCreate();		
}

void CServerSession::Close( int nError, int nDetail )
{
	CSession::Close( nError, nDetail );	
}

BEGIN_MESSAGEMAP(CServerSession)
	ON_MESSAGE( RSNET_ATTACH_REQ, &CServerSession::OnAttachReq )
END_MESSAGEMAP()

void CServerSession::OnAttachReq( CMsgRecv& msg )
{
	char strServiceName[256] = {0,};
	char strServerIP[16] = {0,};
	int nAcceptPort = 0;

	msg.ReadString( strServiceName, 50 );
	msg.ReadString( strServerIP, 16 );
	msg >> nAcceptPort;

	CMsgSend MsgSend;
	MsgSend.ID( RSNET_ATTACH_ACK ) << ERROR_GOOD;
	SendMessage( MsgSend );
}