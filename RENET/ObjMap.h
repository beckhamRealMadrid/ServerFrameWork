#pragma once

START_NAMESPACE

template < class _T, BOOL _Cleanup = TRUE, class _Container = std::map<DWORD, _T*, std::less> >
class CObjMap
{
public:
	CObjMap()	{}
	~CObjMap()	{ Cleanup(); }

public:
	typedef typename _Container			CONTYPE;
	typedef typename _T					value_type;
	typedef typename CONTYPE::iterator	iterator;

protected:
	CONTYPE	m_Map;

public:
	DWORD		Count()				{ return static_cast<DWORD>(m_Map.size()); }
	CONTYPE&	GetContainer()		{ return m_Map; }
	iterator	LBound(DWORD dwKey)	{ return m_Map.lower_bound(dwKey); }
	iterator	UBound(DWORD dwKey)	{ return m_Map.upper_bound(dwKey); }
	iterator	Begin()				{ return m_Map.begin(); }
	iterator	End()				{ return m_Map.end(); }

	BOOL DelObj(DWORD dwID)
	{
		CONTYPE::iterator it = m_Map.find(dwID);
		if (it != m_Map.end())
		{
			if (_Cleanup)
                delete (_T*)((*it).second);

			m_Map.erase(it);
			return TRUE;
		}
		
		return FALSE;
	}

	void Cleanup()
	{
		if (_Cleanup)
		{
			for ( CONTYPE::iterator it = m_Map.begin(); it != m_Map.end(); ++it )
				delete (*it).second;
		}
		
		m_Map.clear();
	}

	_T* GetObj(DWORD dwID)
	{
		CONTYPE::iterator it = m_Map.find(dwID);
		if (it != m_Map.end())
			return (*it).second;
		return NULL;
	}

	BOOL AddObj(DWORD dwID, _T* pObj)
	{
		if (GetObj(dwID) != NULL)
			return FALSE;

		m_Map.insert(CONTYPE::value_type(dwID, pObj));

		return TRUE;
	}

	_T* PopObj(DWORD dwID)
	{
		_T* pObj = NULL;
		CONTYPE::iterator it = m_Map.find(dwID);
		if (it != m_Map.end())
		{
			pObj = (*it).second;
			m_Map.erase(it);
		}

		return pObj;
	}

	_T*	PopFront()
	{
		_T* pObj = NULL;
		CONTYPE::iterator it = m_Map.begin();
		if (it != m_Map.end())
		{
			pObj = (*it).second;
			m_Map.erase(it);
		}

		return pObj;
	}

	iterator LockIterator(DWORD& dwCount)
	{
		dwCount = (DWORD)m_Map.size();
		return m_Map.begin();
	}	
};

END_NAMESPACE