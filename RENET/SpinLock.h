#pragma once

START_NAMESPACE

#define AcquireSpinLock(x)	x.Lock()
#define ReleaseSpinLock(x)	x.Unlock()

class CSpinLock  
{
public:
	CSpinLock()			{ m_lSpinLock = 0; }
	~CSpinLock()		{};

	void	Lock()		{ while(InterlockedCompareExchange( &m_lSpinLock, 1, 0 ) != 0); }
	void	Unlock()	{ InterlockedExchange( &m_lSpinLock, 0 ); }

private:
	LONG	m_lSpinLock;
};

END_NAMESPACE