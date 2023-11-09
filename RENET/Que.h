#pragma once

START_NAMESPACE

#define QUEUE_DEFAULT_SIZE	512
#define QUEUE_NO_LIMIT		-1

template <class T> 
class CQue  
{
public:
	typedef std::deque<T> QUE;

	CQue(long nLimit = QUEUE_NO_LIMIT)	{ m_nLimit = nLimit; }
	virtual ~CQue()						{ m_nLimit = 0; }

	long GetSize() { return m_Que.size(); }
	BOOL IsEmpty() { return m_Que.empty(); }
	
	BOOL IsFull()
	{
		if ( m_nLimit == QUEUE_NO_LIMIT )
			return FALSE;

		return ( m_Que.size() >= m_nLimit );
	}

	BOOL Enque(T Data)
	{
		if (m_nLimit != QUEUE_NO_LIMIT)
		{
			int nCurSize = (int)m_Que.size();
			if (nCurSize >= m_nLimit)
				return FALSE;
		}

		m_Que.push_back(Data);

		return TRUE;
	}

	BOOL Deque(T& Data)
	{
		if (!m_Que.empty())
		{
			Data = m_Que.front();
			m_Que.pop_front();

			return TRUE;
		}
		else
		{
			Data = NULL;
			return FALSE;
		}
	}

protected:
	long	m_nLimit;
	QUE		m_Que;
};

END_NAMESPACE