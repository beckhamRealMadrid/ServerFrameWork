#include "StdAfx.h"
#include "User.h"
#include "SupervisorSession.h"
#include "GameService.h"

CUser::CUser( CSessionManager* pSM ) : CSession( pSM )
{
	
}

CUser::~CUser(void)
{
		
}

void CUser::PostCreate()
{
	CSession::PostCreate();		
}

bool CUser::OnError(DWORD dwErrorCode, int nDetail)
{
	return CSession::OnError( dwErrorCode, nDetail );
}

void CUser::Close( int nError, int nDetail )
{
	CSession::Close( nError, nDetail );	
}

BEGIN_MESSAGEMAP( CUser )	
END_MESSAGE_MAP()