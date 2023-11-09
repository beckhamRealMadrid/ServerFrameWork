#pragma once

START_NAMESPACE

void RENET_API Log( const TCHAR *strFormat, ... );
void RENET_API SetThreadName( DWORD dwThreadID, LPCSTR szThreadName );

END_NAMESPACE