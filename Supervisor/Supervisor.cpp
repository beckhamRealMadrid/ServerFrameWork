#include "stdafx.h"
#include "SupervisorService.h"

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
#ifdef _DEBUG
	CSupervisorService service(false);
#else
	CSupervisorService service(true);
#endif

#ifdef _DEBUG
	lpCmdLine = _T("Supervisor");
#else
#endif

	return service.Start( hInstance, lpCmdLine );
}