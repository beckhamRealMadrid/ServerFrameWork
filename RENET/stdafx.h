#pragma once

#define _ATL_FREE_THREADED
#define _WIN32_DCOM 

#define WIN32_LEAN_AND_MEAN		// 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.

#ifndef WINVER					// Windows 95 및 Windows NT 4 이후 버전에서만 기능을 사용할 수 있습니다.
#define WINVER 0x0500			// Windows 98과 Windows 2000 이후 버전에 맞도록 적합한 값으로 변경해 주십시오.
#endif

#ifndef _WIN32_WINNT			// Windows NT 4 이후 버전에서만 기능을 사용할 수 있습니다.
#define _WIN32_WINNT 0x0500		// Windows 98과 Windows 2000 이후 버전에 맞도록 적합한 값으로 변경해 주십시오.
#endif						

#ifndef _WIN32_WINDOWS			// Windows 98 이후 버전에서만 기능을 사용할 수 있습니다.
#define _WIN32_WINDOWS 0x0500	// Windows Me 이후 버전에 맞도록 적합한 값으로 변경해 주십시오.
#endif

#pragma warning( disable : 4251 4786 4955 4312 4311 4267 4996 4995 4005 4244 4100 4201 4189 4702 4706 4389 4701 4245 4390 )

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
#include "Log.h"
#include "CodeConvert.h"