#include "stdafx.h"
#include "GatewayService.h"

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
#ifdef _DEBUG
	CGatewayService service(false);
#else
	CGatewayService service(true);
#endif

#ifdef _DEBUG
	lpCmdLine = _T("Geteway");
#else
#endif

	return service.Start( hInstance, lpCmdLine );
}