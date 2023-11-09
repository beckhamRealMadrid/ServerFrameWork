#include "StdAfx.h"
#include "TWServer.h"
#include "TWClient.h"
#include "GatewayService.h"
#include "IOCPManager.h"
#include "SupervisorSession.h"

CTWServer::CTWServer( CSessionManager* pSM, CTwoWaySession* pOtherSession ) : CTwoWaySession( pSM )
{
	m_pOtherSession = pOtherSession;
}

CTWServer::~CTWServer(void)
{

}

void CTWServer::PostCreate()
{
	CSession::PostCreate();	
}

int CTWServer::DispatchRecv( DWORD dwBytes, LPOVERLAPPED_BASE lpov )
{
	return CTwoWaySession::DispatchRecv( dwBytes, lpov );
}

bool CTWServer::OnError( DWORD dwErrorCode, int nDetail )
{
	CSession::OnError( dwErrorCode, nDetail );

	return true;
}

void CTWServer::Close( int nError, int nDetail )
{
	CSession::Close( nError, nDetail );	
}

BEGIN_MESSAGEMAP( CTWServer )
END_MESSAGEMAP()