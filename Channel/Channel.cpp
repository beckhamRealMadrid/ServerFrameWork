#include "stdafx.h"
#include "ChannelService.h"

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
#ifdef _DEBUG
	CChannelService Service( false );
#else
	CChannelService Service( true );
#endif

#ifdef _DEBUG
	lpCmdLine = _T("ChannelLogic");
#else
#endif

	return Service.Start( hInstance, lpCmdLine );
}