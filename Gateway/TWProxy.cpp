#include "StdAfx.h"
#include "TWProxy.h"
#include "TWClient.h"
#include "GatewayService.h"

CTWProxy::CTWProxy( CSessionManager* pSM ) : CTwoWaySession( pSM )
{
		
}

CTWProxy::~CTWProxy(void)
{
	
}

void CTWProxy::PostCreate()
{
	CSession::PostCreate();

	CTWClient* pTWClient = reinterpret_cast<CTWClient*>(m_pOtherSession);

	CMsgSend MsgSend;
	MsgSend.ID(RFC_SS_NOTIFY_GATEWAYSESSINFO_REQ)	<< pTWClient->GetUserID()
													<< pTWClient->GetIPAddr()
													<< pTWClient->GetSelectedChrName()
													<< pTWClient->GetSessionKey()
													<< GATEWAYSERVICE->GetServerID()
													<< pTWClient->GetBeforeServerID()
													<< pTWClient->GetSelectedChannelID()
													<< pTWClient->GetGameStep()
													<< pTWClient->GetSelectedRoomID();													
	SendMessage( MsgSend );	
}

int CTWProxy::DispatchRecv( DWORD dwBytes, LPOVERLAPPED_BASE lpov )
{
	return CTwoWaySession::DispatchRecv( dwBytes, lpov );
}

bool CTWProxy::OnError( DWORD dwErrorCode, int nDetail )
{
//	m_pOtherSession->Close( dwErrorCode, nDetail );

	return CSession::OnError( dwErrorCode, nDetail );
}

void CTWProxy::Close( int nError, int nDetail )
{
	CSession::Close( nError, nDetail );	
}

BEGIN_MESSAGEMAP( CTWProxy )
END_MESSAGEMAP()