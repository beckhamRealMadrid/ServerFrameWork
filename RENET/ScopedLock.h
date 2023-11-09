#pragma once

START_NAMESPACE

#define SCOPED_LOCK_SINGLE(LOCK)					\
	CScopedLockSingle TempScopedLockS(LOCK, TRUE);	\

class RENET_API CScopedLockSingle
{
public:
	CScopedLockSingle(CSynchObject* pObject, BOOL bInitialLock);
	~CScopedLockSingle() { Unlock(); }

public:
	inline BOOL IsLocked() { return m_bAcquired; }
	inline BOOL Lock(DWORD dwTimeOut = INFINITE);
	inline BOOL Unlock();
	inline BOOL Unlock(LONG lCount, LPLONG lPrevCount = NULL);

protected:
	CSynchObject*	m_pObject;
	HANDLE			m_hObject;
	BOOL			m_bAcquired;
};

inline CScopedLockSingle::CScopedLockSingle(CSynchObject* pObject, BOOL bInitialLock)
{
	_ASSERT(pObject != NULL);

	m_pObject	= pObject;
	m_hObject	= pObject->m_hObject;
	m_bAcquired = FALSE;

	if (bInitialLock)
		Lock();
}

inline BOOL CScopedLockSingle::Lock(DWORD dwTimeOut)  // = INFINITE
{
	_ASSERT(m_pObject != NULL || m_hObject != NULL);
	_ASSERT(!m_bAcquired);

	m_bAcquired = m_pObject->Lock(dwTimeOut);
	return m_bAcquired;
}

inline BOOL CScopedLockSingle::Unlock()
{
	_ASSERT(m_pObject != NULL);
	if (m_bAcquired)
		m_bAcquired = !m_pObject->Unlock();

	return !m_bAcquired;
}

inline BOOL CScopedLockSingle::Unlock(LONG lCount, LPLONG lpPrevCount)
{
	_ASSERT(m_pObject != NULL);
	if (m_bAcquired)
		m_bAcquired = !m_pObject->Unlock(lCount, lpPrevCount);

	return !m_bAcquired;
}

#define SCOPED_LOCK_MULTI(LOCKs, COUNT, INITIAL_LOCK)					\
	CScopedLockMulti TempScopedLockS(LOCKs, COUNT, INITIAL_LOCK);	\		

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

class RENET_API CScopedLockMulti
{
public:
	inline CScopedLockMulti(CSynchObject* ppObjects[], DWORD dwCount, BOOL bInitialLock = FALSE);
	inline ~CScopedLockMulti();

public:
	inline DWORD	Lock(DWORD dwTimeOut = INFINITE, BOOL bWaitForAll = TRUE, DWORD dwWakeMask = 0);
	inline BOOL		Unlock();
	inline BOOL		Unlock(LONG lCount, LPLONG lPrevCount = NULL);
	inline BOOL		IsLocked(DWORD dwItem);

protected:
	HANDLE  m_hPreallocated[8];
	BOOL    m_bPreallocated[8];

	HANDLE* m_pHandleArray;
	BOOL*   m_bLockedArray;
	DWORD   m_dwCount;

	CSynchObject* const * m_ppObjectArray;
};

inline BOOL CScopedLockMulti::IsLocked(DWORD dwObject)
{
	_ASSERT(dwObject >= 0 && dwObject < m_dwCount);
	return m_bLockedArray[dwObject];
}

inline CScopedLockMulti::CScopedLockMulti(CSynchObject* pObjects[], DWORD dwCount, BOOL bInitialLock)
{
	_ASSERT(dwCount > 0 && dwCount <= MAXIMUM_WAIT_OBJECTS);
	_ASSERT(pObjects != NULL);

	m_ppObjectArray = pObjects;
	m_dwCount		= dwCount;

	if (m_dwCount > _countof(m_hPreallocated))
	{
		m_pHandleArray = new HANDLE[m_dwCount];
		m_bLockedArray = new BOOL[m_dwCount];
	}
	else
	{
		m_pHandleArray = m_hPreallocated;
		m_bLockedArray = m_bPreallocated;
	}

	for (DWORD i = 0; i < m_dwCount; i++)
	{
		_ASSERT(pObjects[i]);

		_ASSERT(pObjects[i]->IsCriticalSection() == FALSE);

		m_pHandleArray[i] = pObjects[i]->m_hObject;
		m_bLockedArray[i] = FALSE;
	}

	if (bInitialLock)
		Lock();
}

inline CScopedLockMulti::~CScopedLockMulti()
{
	Unlock();
	if (m_pHandleArray != m_hPreallocated)
	{
		delete [] m_bLockedArray;
		delete [] m_pHandleArray;
	}
}

inline DWORD CScopedLockMulti::Lock(DWORD dwTimeOut, BOOL bWaitForAll, DWORD dwWakeMask)
{
	DWORD dwResult;
	if (dwWakeMask == 0)
		dwResult = ::WaitForMultipleObjects(m_dwCount, m_pHandleArray, bWaitForAll, dwTimeOut);
	else
		dwResult = ::MsgWaitForMultipleObjects(m_dwCount, m_pHandleArray, bWaitForAll, dwTimeOut, dwWakeMask);

	if (dwResult >= WAIT_OBJECT_0 && dwResult < (WAIT_OBJECT_0 + m_dwCount))
	{
		if (bWaitForAll)
		{
			for (DWORD i = 0; i < m_dwCount; i++)
				m_bLockedArray[i] = TRUE;
		}
		else
		{
			_ASSERT((dwResult - WAIT_OBJECT_0) >= 0);
			m_bLockedArray[dwResult - WAIT_OBJECT_0] = TRUE;
		}
	}

	return dwResult;
}

inline BOOL CScopedLockMulti::Unlock()
{
	for (DWORD i=0; i < m_dwCount; i++)
	{
		if (m_bLockedArray[i])
			m_bLockedArray[i] = !m_ppObjectArray[i]->Unlock();
	}
	return TRUE;
}

inline BOOL CScopedLockMulti::Unlock(LONG lCount, LPLONG lpPrevCount)
{
	for (DWORD i=0; i < m_dwCount; i++)
	{
		if (m_bLockedArray[i])
			m_bLockedArray[i] = !m_ppObjectArray[i]->Unlock(lCount, lpPrevCount);
	}

	return TRUE;
}

enum{ RW_READ, RW_WRITE };
#define SCOPED_RW_LOCK(LOCK,TYPE)				\
	CScopedRWLock TempRWLockS(LOCK, TYPE);\

class RENET_API CScopedRWLock
{
public:
	CScopedRWLock(CRWMonitor* pObject,int nType = RW_READ);
	~CScopedRWLock() { Unlock(m_nType); }

public:	
	inline BOOL Lock(int nType);
	inline BOOL Unlock(int nType);	

protected:
	CRWMonitor*	m_pObject;	
	int			m_nType;
};

inline CScopedRWLock::CScopedRWLock(CRWMonitor* pObject, int nType)
{
	_ASSERT(pObject != NULL);	
	m_pObject	= pObject;	
	m_nType		= nType;
	Lock(m_nType);
}

inline BOOL CScopedRWLock::Lock(int nType)
{
	_ASSERT(m_pObject != NULL);	

	switch(nType)
	{
		case RW_READ:
			m_pObject->StartReading();
			return TRUE;
		case RW_WRITE:
			m_pObject->StartWriting();
			return TRUE;
	}

	return FALSE;
}

inline BOOL CScopedRWLock::Unlock(int nType)
{
	_ASSERT(m_pObject != NULL);
	switch(nType)
	{
		case RW_READ:
			m_pObject->StopReading();
			return TRUE;
		case RW_WRITE:
			m_pObject->StopWriting();
			return TRUE;
	}

	return FALSE;
}

END_NAMESPACE