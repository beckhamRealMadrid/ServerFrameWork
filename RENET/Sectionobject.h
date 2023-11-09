#pragma once

START_NAMESPACE

/* CRITICAL_SECTION struct capsule For Synchronized class */

class RENET_API SectionObject	
{
public:
	SectionObject()
	{
		::InitializeCriticalSection( &m_cs );
	}

	virtual ~SectionObject()
	{
		::DeleteCriticalSection( &m_cs );
	}

	inline void EnterSection()
	{
		::EnterCriticalSection( &m_cs );
	}

	inline void LeaveSection()
	{
		::LeaveCriticalSection( &m_cs );
	}
private:
	CRITICAL_SECTION m_cs;
};

END_NAMESPACE