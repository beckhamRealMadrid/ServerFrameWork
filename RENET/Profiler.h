#pragma once

#include "SectionObject.h"
#include "Synchronized.h"

START_NAMESPACE

class RENET_API CProfiler
{
protected:
							CProfiler();
public:
	virtual					~CProfiler();
	static CProfiler*		GetInstance();
			BOOL			CreateProfileSession( /*[in]*/const char *session_name , /*[out]*/DWORD* pSessionId );
			void			DeleteProfileSession( /*[in]*/const char* session_name );
			void			StartProfiling( DWORD dwSessionId , DWORD* pRunKey );
			void			EndProfiling( DWORD dwSessionId   , DWORD RunKey );
			float			GetLastResult( DWORD dwSessionId );
			float			GetAvgResult( DWORD dwSessionId );
			DWORD			GetTotalCount( DWORD dwSessionId );
			BOOL			GetIdFromName( /*[in]*/const char *session_name , /*[out]*/DWORD* pSessionId );
			DWORD			GetSessionNum();
protected:
			void*			m_profiling_map;			
			void*			m_profiling_his;		// �������� ���� �ð� ����ü �����͸� ������ �ٴѴ�.
			SectionObject	m_soList;
};

#define PROFILER	CProfiler::GetInstance()

END_NAMESPACE