#include "stdafx.h"
#include "CharacterService.h"

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
#ifdef _DEBUG
	CCharacterService service(false);
#else
	CCharacterService service(true);
#endif

#ifdef _DEBUG
	lpCmdLine = _T("Character");
#else
#endif

	return service.Start( hInstance, lpCmdLine );
}