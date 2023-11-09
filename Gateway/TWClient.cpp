#include "StdAfx.h"
#include "TWClient.h"
#include "TWServer.h"
#include "GatewayService.h"
#include "SupervisorSession.h"
#include "IOCPManager.h"
#include "../Common/Protocol.h"

CTWClient::CTWClient( CSessionManager* pSM ) : CTwoWaySession( pSM )
{
	m_pOtherSession = new CTWServer( NULL, this );
	m_bPassThru = false;	
}

CTWClient::~CTWClient(void)
{
	SAFE_DELETE( m_pOtherSession );	
}

void CTWClient::PostCreate()
{
	CSession::PostCreate();

	m_bPassThru = false;	
}

int CTWClient::DispatchRecv( DWORD dwBytes, LPOVERLAPPED_BASE lpov )
{
	if ( m_bPassThru )
		return CTwoWaySession::DispatchRecv( dwBytes, lpov );
	else
		return CSession::DispatchRecv( dwBytes, lpov );
}

bool CTWClient::OnError( DWORD dwErrorCode, int nDetail )
{
	CSession::OnError( dwErrorCode, nDetail );

	m_pOtherSession->Close( dwErrorCode, nDetail );

    return true;
}

void CTWClient::Close( int nError, int nDetail )
{
	CSession::Close( nError, nDetail );		
}

BEGIN_MESSAGEMAP( CTWClient )
	ON_MESSAGE( RSNET_LOGIN_REQ, &CTWClient::OnLoginReq )
END_MESSAGEMAP()

void CTWClient::OnLoginReq( CMsgRecv& msg )
{
	char szUserID[50] = {0,};
	char szPasswd[50] = {0,};

	msg.ReadString( szUserID, 50 );
	msg.ReadString( szPasswd, 50 );

	CMsgSend MsgSend;
	MsgSend.ID( RSNET_LOGIN_REQ ) << szUserID << szPasswd << GetIPAddr() << (DWORD)this;
	USERMANAGER_SESSION->SendMessage( MsgSend );
}