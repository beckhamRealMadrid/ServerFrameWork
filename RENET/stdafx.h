#pragma once

#define _ATL_FREE_THREADED
#define _WIN32_DCOM 

#define WIN32_LEAN_AND_MEAN		// ���� ������ �ʴ� ������ Windows ������� �����մϴ�.

#ifndef WINVER					// Windows 95 �� Windows NT 4 ���� ���������� ����� ����� �� �ֽ��ϴ�.
#define WINVER 0x0500			// Windows 98�� Windows 2000 ���� ������ �µ��� ������ ������ ������ �ֽʽÿ�.
#endif

#ifndef _WIN32_WINNT			// Windows NT 4 ���� ���������� ����� ����� �� �ֽ��ϴ�.
#define _WIN32_WINNT 0x0500		// Windows 98�� Windows 2000 ���� ������ �µ��� ������ ������ ������ �ֽʽÿ�.
#endif						

#ifndef _WIN32_WINDOWS			// Windows 98 ���� ���������� ����� ����� �� �ֽ��ϴ�.
#define _WIN32_WINDOWS 0x0500	// Windows Me ���� ������ �µ��� ������ ������ ������ �ֽʽÿ�.
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