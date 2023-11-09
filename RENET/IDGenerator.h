#pragma once

START_NAMESPACE

#define ID_POOL_INCREMENT_SIZE		3000			
#define ID_POOL_KEEP_MINIMAL_RATIO	(float)0.1f		

class RENET_API CIDPool
{
public:
	CIDPool()
	{
		m_dwLastAllocID = 1;
		m_dwSpanSize = ID_POOL_INCREMENT_SIZE;
	}

	~CIDPool() {};

protected:
	DWORD	m_dwSpanSize;
	DWORD	m_dwLastAllocID;
	std::queue<DWORD> m_Pool;

public:
	BOOL Create( DWORD pool_span_size = ID_POOL_INCREMENT_SIZE )
	{
		m_dwSpanSize = pool_span_size;
		Span();

		return TRUE;
	}

	DWORD AllocID()
	{
		DWORD NewID;

		if ( m_Pool.empty() == TRUE || m_Pool.size() < ( m_dwSpanSize * ID_POOL_KEEP_MINIMAL_RATIO ) )	
			Span();

		_ASSERT(m_Pool.size() > 0);

		NewID = m_Pool.front();
		m_Pool.pop();

		_ASSERT(NewID != NULL);

		return NewID;
	}

	void FreeID( DWORD ID ) 
	{ 
		if (ID == 0)
			return;

		m_Pool.push(ID); 
	}

	DWORD GetFreeIDNum() 
	{ 
		return (DWORD)m_Pool.size(); 
	}

	DWORD GetAllocatedIDNum() 
	{ 
		return (m_dwLastAllocID - 1); 
	}

	void GetPoolUsage( DWORD& dwAllocated, DWORD& dwInUse )
	{
		dwAllocated = m_dwLastAllocID - 1;
		dwInUse		= (dwAllocated - m_Pool.size());
	}

	DWORD GetSpanSize() { return m_dwSpanSize; }

protected:
	virtual void Span()
	{
		for ( DWORD i = m_dwLastAllocID; i < (m_dwLastAllocID + m_dwSpanSize); ++i )
		{
			if ( i == 0 )
				continue;

			m_Pool.push(i);
		}

		m_dwLastAllocID += m_dwSpanSize;
	}
};

END_NAMESPACE
