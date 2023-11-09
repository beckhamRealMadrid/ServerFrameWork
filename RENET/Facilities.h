#pragma once

START_NAMESPACE

template <class T, class _T_TIME = float>
class CScheduledCallbacker
{
	typedef void (T::*LPCALLBACK)();

	struct CallbackEntry
	{
		_T_TIME			Elapsed;
		_T_TIME			Timeout;
		T*				pInstance;
		LPCALLBACK		func;
	};

	typedef std::vector<CallbackEntry>	CALLBACKS;

public:
	CScheduledCallbacker() {}
	virtual ~CScheduledCallbacker() {}

protected:
	CALLBACKS	m_Entries;

public:
	void RegisterCallback( T* pInstance, _T_TIME Timeout, LPCALLBACK lpFunc )
	{
		_ASSERT(Timeout > 0);

		CallbackEntry entry;
		entry.Elapsed   = 0;
		entry.Timeout   = Timeout;
		entry.pInstance = pInstance;
		entry.func		= lpFunc;

		m_Entries.push_back(entry);
	}

	void Update(_T_TIME delta)
	{
		if ( delta == 0 )
			return;

		for ( int i = 0; i < (int)m_Entries.size(); ++i )
		{
			CallbackEntry& entry = m_Entries[i];

			entry.Elapsed += delta;

			if ( entry.Elapsed >= entry.Timeout )
			{
				(*entry.pInstance.*entry.func)();
				entry.Elapsed -= entry.Timeout;
			}
		}
	}
};

template < int MAX_SLOT_COUNT = 10 >
class CAvgTimeChecker
{
	enum { AVG_TIMER_SLOT_COUNT = MAX_SLOT_COUNT };

public:
	CAvgTimeChecker(DWORD accum = 100)
	{
		_ASSERT(accum > 0);

		m_dwMaxAccum = accum;

		m_nSlot = 0;
		m_dwPrev = 0;
		m_dwAccumed = 0;
		m_dwAccumCounter = 0;
		m_dwAvgTime = 0;

		::ZeroMemory(m_Averages, sizeof(m_Averages));
	}

protected:
	int		m_nSlot;
	DWORD	m_dwPrev;
	DWORD	m_dwAccumed;
	DWORD	m_dwAccumCounter;
	DWORD	m_dwAvgTime;
	DWORD	m_dwMaxAccum;
	DWORD	m_Averages[AVG_TIMER_SLOT_COUNT];

public:
	float	GetAvgTimeF()	{ return (m_dwAvgTime / 1000.0f); }
	int		GetAvgTimeN()	{ return m_dwAvgTime; }
	void	Begin()			{ m_dwPrev = ::GetTickCount(); }

	void End()
	{
		DWORD dwElapsed = ::GetTickCount() - m_dwPrev;

		m_dwAccumed += dwElapsed;
		++m_dwAccumCounter;

		if (m_dwAccumCounter >= m_dwMaxAccum)
		{
			++m_nSlot;
			m_nSlot %= AVG_TIMER_SLOT_COUNT;

			m_Averages[m_nSlot] = (m_dwAccumed / m_dwMaxAccum);

			DWORD dwTotal = 0;
			for (int i = 0; i < AVG_TIMER_SLOT_COUNT; ++i)
				dwTotal += m_Averages[i];

			m_dwAvgTime = (dwTotal / AVG_TIMER_SLOT_COUNT);

			// reset;
			m_dwAccumed = 0;
			m_dwAccumCounter = 0;
		}
	}
};

// std::string 을 Key로 하는 map 에서 사용
struct str_comp_no_case
{
	bool operator () (const std::string& a, const std::string& b) const
	{
		return (::_stricmp(a.c_str(), b.c_str()) != 0);
	}
};

inline unsigned __int64 getMisteryCount() 
{ 
	__asm  
	{   
		mov	edx,dword ptr ds:[7FFE000Ch]
		mov eax,dword ptr ds:[7FFE0008h] 
	} 
}

inline unsigned int getElapsedMilliSecond() 
{ 
	return static_cast<unsigned int>( getMisteryCount() / 10000 ); 
}

END_NAMESPACE