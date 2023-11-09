#include "stdafx.h"
#include "UserManager.h"
#include "UserManagerService.h"

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
#ifdef _DEBUG
	CUserManagerService service(false);
#else
	CUserManagerService service(true);
#endif

#ifdef _DEBUG
	lpCmdLine = _T("UserManager");
#else
#endif

	return service.Start( hInstance, lpCmdLine );
}