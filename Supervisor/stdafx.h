#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define WINVER 0x0500			// Windows 98과 Windows 2000 이후 버전에 맞도록 적합한 값으로 변경해 주십시오.
#define _WIN32_WINNT 0x0500		// Windows 98과 Windows 2000 이후 버전에 맞도록 적합한 값으로 변경해 주십시오.
#define _WIN32_WINDOWS 0x0500	// Windows Me 이후 버전에 맞도록 적합한 값으로 변경해 주십시오.

#pragma warning( disable : 4251 4786 4955 4312 4311 4267 4996 4995 4005 4244 4100 4201 4189 4702 4706 4389 4701 4245 )

#include <winsock2.h>
#include <windows.h>
#include <Mmsystem.h>
#include <mswsock.h>
#include <assert.h>
#include <tchar.h>
#include <strsafe.h>
#include <string>
#include <stdlib.h>
#include <atlbase.h>
#include <DbgHelp.h>
#include <process.h>
#include <time.h>
#include <set>
#include <map>
#include <list>
#include <vector>
#include <deque>
#include <queue> 
#include <unordered_map>
#include <functional>
#include <algorithm>

#include "RENET.h"
#include "GlobalDefine.h"
#include "Monitor.h"
#include "Log.h"