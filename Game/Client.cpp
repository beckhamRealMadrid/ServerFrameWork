#include "StdAfx.h"
#include "Client.h"
#include "Msg.h"
#include "User.h"

CClient::CClient( CConnectionManager* pPM ) : CConnection( pPM )
{	
	
}

CClient::~CClient(void)
{

}

void CClient::PostCreate( CRelaySession* pSession, SOCKADDR_IN* pAddress )
{
	CConnection::PostCreate( pSession, pAddress );	
}

void CClient::OnClose()
{
	
}

BEGIN_MESSAGE_MAP( CClient )
END_MESSAGE_MAP()