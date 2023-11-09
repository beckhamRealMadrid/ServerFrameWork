#include "StdAfx.h"
#include "UserManagerSession.h"
#include "TWClient.h"
#include "../Common/Protocol.h"

CUserManagerSession::CUserManagerSession(void)
{

}

CUserManagerSession::~CUserManagerSession(void)
{

}

void CUserManagerSession::PostCreate()
{
	CSession::PostCreate();
}

BEGIN_MESSAGEMAP( CUserManagerSession )
	ON_MESSAGE( RSNET_LOGIN_ACK, &CUserManagerSession::OnLoginAck )
END_MESSAGEMAP()

void CUserManagerSession::OnLoginAck( CMsgRecv& msg )
{
	char szUserID[50] = {0,};
	DWORD dwSessKey = 0;
	u_char ucError = ERROR_GOOD;
	CTWClient* pSess = NULL;

	msg.ReadString( szUserID, 50 );
	msg >> dwSessKey >> ucError;
	
	pSess = (CTWClient*)dwSessKey;

	CMsgSend msgSend;
	msgSend.ID( RSNET_LOGIN_ACK ) << dwSessKey << ucError;		
	pSess->SendMessage( msgSend );

	pSess->SetUserID( szUserID );
}