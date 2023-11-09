#include "StdAfx.h"
#include "SupervisorSession.h"
#include "GameService.h"
#include "User.h"
#include "IOCPManager.h"
#include "../Common/Protocol.h"

CSupervisorSession::CSupervisorSession( CSessionManager* pSM ) : CAutoConnSession( pSM )
{
	
}

CSupervisorSession::~CSupervisorSession(void)
{

}

void CSupervisorSession::PostCreate()
{
	CSession::PostCreate();

	CMsgSend MsgSend;
	MsgSend.ID( RSNET_ATTACH_REQ ) << GAMESERVICE->GetServiceName() << GAMESERVICE->GetServerIP() << GAMESERVICE->GetServerPort();
	SendMessage( MsgSend );
}

BEGIN_MESSAGEMAP( CSupervisorSession )	
	ON_MESSAGE( RSNET_ATTACH_ACK, &CSupervisorSession::OnAttachAck )
END_MESSAGEMAP()

void CSupervisorSession::OnAttachAck( CMsgRecv& msg )
{
	BYTE byCode;
	msg >> byCode;

	Log( _T("<OnAttachAck> Code : %d"), byCode );
}