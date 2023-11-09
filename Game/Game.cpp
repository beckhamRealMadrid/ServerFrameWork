#include "stdafx.h"
#include "GameService.h"

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
#ifdef _DEBUG
	CGameService Service( false );
#else
	CGameService Service( true );
#endif

#ifdef _DEBUG
	lpCmdLine = _T("Game");
#else
#endif	

	return Service.Start( hInstance, lpCmdLine );
}