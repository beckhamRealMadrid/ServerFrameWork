#include "StdAfx.h"
#include "ServerSession.h"

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

BEGIN_MESSAGEMAP( CServerSession )
END_MESSAGEMAP()