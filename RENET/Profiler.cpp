#include "StdAfx.h"
#include "Profiler.h"

START_NAMESPACE

#define MAX_PROFILE_SESSION 16

using namespace std;

typedef map< string, DWORD > ProfileMap;

typedef struct _PROFILE_HISTORY
{
	BOOL  bReserved;		// ���� ����
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

// ���ο� ���������� �����Ѵ�.
BOOL CProfiler::CreateProfileSession( const char* session_name, DWORD *pSessionId )
{
	DWORD i;
	BOOL  ret;
	ProfileMap*      profiling_map;
	PROFILE_HISTORY* profiling_his;			// ���α׷��� �ð� Check�� ���ؼ�.
	map<string,DWORD>::iterator index;

	i = 0; ret = FALSE;

	string name = session_name;	
	profiling_map = (ProfileMap*)m_profiling_map;
	profiling_his = (PROFILE_HISTORY*)m_profiling_his;				// �������� ���� �ð� ����ü �����͸� ������ �ٴѴ�.

	Synchronized so( &m_soList );

	index = profiling_map->find( name );

	// ���ο� ���������� ����� ������ �翬�� End���� �Ѵ�.
	if ( index == profiling_map->end() )
	{
		while ( i < MAX_PROFILE_SESSION )
		{
			if ( profiling_his[i].bReserved == FALSE )
			{
				if ( pSessionId )
					*pSessionId = i;

				// profile�� ���� ���尪�� �����ͷ� ������ �Ѵ�. �ش��ϴ� ��ġ�� 0���� �ʱ�ȭ�� ���ش�.
				// ���� ���۸� ����Ű�� �ش� ��ġ�� ã�´�. ���
				::ZeroMemory( profiling_his + i , sizeof( PROFILE_HISTORY ) );

				profiling_his[i].bReserved = TRUE;				

				// �迭�� ��ġ������ �����ؼ� ������ �ִ�.
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

// �ð� ����� �����Ѵ�.
void CProfiler::StartProfiling( DWORD dwSessionId, DWORD* pRunKey )
{
	PROFILE_HISTORY* profiling_his;

	profiling_his = (PROFILE_HISTORY*)m_profiling_his;

	// ������� �����̴ٸ�
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

// �ð� ����� ������. ���� : 1��° - sessionId, 2��° - ó�� ������ �ð�   
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

// �ش� ���������� ��� �ð��� �����Ѵ�.
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

// �ش� ���������� ������ �ð��� �����Ѵ�.
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