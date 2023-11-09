#include "StdAfx.h"
#include "Profiler.h"

START_NAMESPACE

#define MAX_PROFILE_SESSION 16

using namespace std;

typedef map< string, DWORD > ProfileMap;

typedef struct _PROFILE_HISTORY
{
	BOOL  bReserved;		// 예비 보유
	float fTotalTime;
	float fLastTime;
	DWORD dwCount;
} PROFILE_HISTORY, *LPPROFILE_HISTORY;

// -----------------------------------------
// Timer Implementation
// -----------------------------------------

float time_interval = 1.0f; // Time 

void init_timer()
{
	LARGE_INTEGER frequency;	

	QueryPerformanceFrequency( &frequency );

	time_interval = (float)( 1000.0f / (double)(frequency.QuadPart) );
}

float get_curr_time()
{
	LARGE_INTEGER count;

	QueryPerformanceCounter( &count );

	return (float)( time_interval * (double)(count.QuadPart) );
}

// -----------------------------------------
// CProfiler Implementation
// -----------------------------------------
CProfiler::CProfiler()
{
	int i;

	init_timer();

	m_profiling_map = new ProfileMap;
	m_profiling_his = new PROFILE_HISTORY[MAX_PROFILE_SESSION];

	for ( i = 0 ; i < MAX_PROFILE_SESSION ; ++i )
	{
		( (PROFILE_HISTORY*)m_profiling_his + i )->bReserved = FALSE;
	}
}

CProfiler::~CProfiler()
{
	if ( m_profiling_map )
	{
		ProfileMap* pProfileMap = (ProfileMap*)m_profiling_map;
		delete pProfileMap;
		m_profiling_map = NULL;
	}

	if ( m_profiling_his )
	{
		PROFILE_HISTORY* pHistory = (PROFILE_HISTORY*)m_profiling_his;
		delete[] pHistory;
		m_profiling_his = NULL;
	}
}

CProfiler* CProfiler::GetInstance()
{
	static CProfiler singleObject;

	return &singleObject;
}

// 새로운 프로파일을 생성한다.
BOOL CProfiler::CreateProfileSession( const char* session_name, DWORD *pSessionId )
{
	DWORD i;
	BOOL  ret;
	ProfileMap*      profiling_map;
	PROFILE_HISTORY* profiling_his;			// 프로그램의 시간 Check를 위해서.
	map<string,DWORD>::iterator index;

	i = 0; ret = FALSE;

	string name = session_name;	
	profiling_map = (ProfileMap*)m_profiling_map;
	profiling_his = (PROFILE_HISTORY*)m_profiling_his;				// 프로파일 관리 시간 구조체 포인터를 가지고 다닌다.

	Synchronized so( &m_soList );

	index = profiling_map->find( name );

	// 새로운 프로파일을 만들기 때문에 당연히 End여야 한다.
	if ( index == profiling_map->end() )
	{
		while ( i < MAX_PROFILE_SESSION )
		{
			if ( profiling_his[i].bReserved == FALSE )
			{
				if ( pSessionId )
					*pSessionId = i;

				// profile에 대한 저장값은 포인터로 관리를 한다. 해당하는 위치에 0으로 초기화를 해준다.
				// 뭔가 버퍼를 가르키고 해당 위치를 찾는다. 등등
				::ZeroMemory( profiling_his + i , sizeof( PROFILE_HISTORY ) );

				profiling_his[i].bReserved = TRUE;				

				// 배열에 위치값만을 저장해서 가지고 있다.
				profiling_map->insert( ProfileMap::value_type( name , i ) );	
				ret = TRUE;
				break;
			}

			++i;
		}
	}

	return ret;
}

void CProfiler::DeleteProfileSession( const char* session_name )
{
	ProfileMap*      profiling_map;
	PROFILE_HISTORY* profiling_his;
	map<string,DWORD>::iterator index;

	string name = session_name;
	profiling_map = (ProfileMap*)m_profiling_map;
	profiling_his = (PROFILE_HISTORY*)m_profiling_his;

	Synchronized so( &m_soList );

	index = profiling_map->find( name );
	if ( index != profiling_map->end() )
	{
		DWORD his_index = (*index).second;
		profiling_map->erase( name );
		profiling_his[his_index].bReserved = FALSE;				
	}	
}

// 시간 기록을 시작한다.
void CProfiler::StartProfiling( DWORD dwSessionId, DWORD* pRunKey )
{
	PROFILE_HISTORY* profiling_his;

	profiling_his = (PROFILE_HISTORY*)m_profiling_his;

	// 사용중인 영역이다면
	if ( dwSessionId < MAX_PROFILE_SESSION && profiling_his[dwSessionId].bReserved == TRUE )   
	{
		float cur_time;

		assert( pRunKey != NULL );
		cur_time = get_curr_time();
		memcpy( pRunKey , &cur_time , sizeof(float) );
	}
	else
	{
		*pRunKey = -1;
	}
}

// 시간 기록을 끝낸다. 인자 : 1번째 - sessionId, 2번째 - 처음 시작한 시간   
void CProfiler::EndProfiling( DWORD dwSessionId, DWORD RunKey )
{
	PROFILE_HISTORY* profiling_his;

	profiling_his = (PROFILE_HISTORY*)m_profiling_his;

	if ( dwSessionId < MAX_PROFILE_SESSION && profiling_his[dwSessionId].bReserved == TRUE )
	{
		float pre_time;
		float cur_time;
		float time_interval;

		memcpy( &pre_time , &RunKey , sizeof(float) );
		cur_time = get_curr_time();
		time_interval = cur_time - pre_time;

		profiling_his[dwSessionId].dwCount++;
		profiling_his[dwSessionId].fTotalTime += time_interval;
		profiling_his[dwSessionId].fLastTime  =  time_interval;
	}	
}

// 해당 프로파일의 평균 시간을 리턴한다.
float CProfiler::GetAvgResult( DWORD dwSessionId )
{
	PROFILE_HISTORY* profiling_his;

	profiling_his = (PROFILE_HISTORY*)m_profiling_his;

	if ( dwSessionId < MAX_PROFILE_SESSION && profiling_his[dwSessionId].bReserved == TRUE )
	{
		if ( profiling_his[dwSessionId].dwCount )
		{
			return (profiling_his[dwSessionId].fTotalTime / profiling_his[dwSessionId].dwCount);
		}
		else
		{
			return 0.0f;
		}
	}

	return -1.0f;
}

// 해당 프로파일의 마지막 시간을 리턴한다.
float CProfiler::GetLastResult( DWORD dwSessionId )
{
	PROFILE_HISTORY* profiling_his;

	profiling_his = (PROFILE_HISTORY*)m_profiling_his;

	if ( dwSessionId < MAX_PROFILE_SESSION && profiling_his[dwSessionId].bReserved == TRUE )
	{
		return profiling_his[dwSessionId].fLastTime;
	}

	return -1.0f;
}

BOOL CProfiler::GetIdFromName( const char* session_name, DWORD *pSessionId )
{
	ProfileMap*      profiling_map;
	PROFILE_HISTORY* profiling_his;
	map<string,DWORD>::iterator index;

	string name = session_name;

	profiling_map = (ProfileMap*)m_profiling_map;
	profiling_his = (PROFILE_HISTORY*)m_profiling_his;

	Synchronized so( &m_soList );

	assert( pSessionId != NULL );
	index = profiling_map->find( name );
	if ( index != profiling_map->end() )
	{
		return (*index).second;
	}

	return FALSE;
}

DWORD CProfiler::GetTotalCount( DWORD dwSessionId )
{
	PROFILE_HISTORY* profiling_his;

	profiling_his = (PROFILE_HISTORY*)m_profiling_his;

	if ( dwSessionId < MAX_PROFILE_SESSION && profiling_his[dwSessionId].bReserved == TRUE )
	{
		return profiling_his[dwSessionId].dwCount;
	}

	return 0;
}

DWORD CProfiler::GetSessionNum()
{
	return ((ProfileMap*)m_profiling_map)->size();
}

END_NAMESPACE