#pragma once

START_NAMESPACE

class RENET_API CSynchObject
{
public:
	CSynchObject(LPCSTR pstrName);
	virtual ~CSynchObject();

protected:
	HANDLE		m_hObject;
	std::string	m_strName;

public:
	virtual BOOL Lock(DWORD dwTimeout = INFINITE);

	virtual BOOL Unlock() = 0;
	virtual BOOL Unlock(LONG, LPLONG);
	virtual BOOL IsCriticalSection();
	LPCTSTR		 GetName();

	operator HANDLE() const;

	friend class CScopedLockSingle;
	friend class CScopedLockMulti;
};

class CSemaBS : public CSynchObject
{
public:
	CSemaBS(LONG lInitialCount = 1, LONG lMaxCount = 1, LPCSTR pstrName=NULL, LPSECURITY_ATTRIBUTES lpsaAttributes = NULL) : CSynchObject(pstrName)
	{
		_ASSERT(lMaxCount > 0);
		_ASSERT(lInitialCount <= lMaxCount);

		m_hObject = ::CreateSemaphoreA(lpsaAttributes, lInitialCount, lMaxCount, pstrName);
		_ASSERT(m_hObject != NULL);
	}
	virtual ~CSemaBS() {}

public:
	virtual BOOL Unlock();
	virtual BOOL Unlock(LONG lCount, LPLONG lprevCount = NULL);
};

class CMutexBS : public CSynchObject
{
public:
	CMutexBS(LPSECURITY_ATTRIBUTES lpsaAttribute = NULL, BOOL bInitiallyState = FALSE, LPCSTR lpszName = NULL) : CSynchObject(lpszName)
	{
		m_hObject = ::CreateMutexA(lpsaAttribute, bInitiallyState, lpszName);
		_ASSERT(m_hObject != NULL);
	}

	virtual ~CMutexBS() {};

public:
	BOOL Unlock()
	{
		return ::ReleaseMutex(m_hObject);
	}
};

class CEventBS : public CSynchObject
{
public:
	CEventBS(LPSECURITY_ATTRIBUTES lpsaAttribute = NULL, BOOL bManualReset = TRUE, BOOL bInitiallyState = FALSE, LPCSTR pstrName = NULL) 
		: CSynchObject(pstrName)
	{
		m_hObject = ::CreateEventA(lpsaAttribute, bManualReset, bInitiallyState, pstrName);
		_ASSERT(m_hObject != NULL);
	}

	virtual ~CEventBS() {};

public:
	BOOL SetEvent()		{ _ASSERT(m_hObject != NULL); return ::SetEvent(m_hObject);   }
	BOOL PulseEvent()	{ _ASSERT(m_hObject != NULL); return ::PulseEvent(m_hObject); }
	BOOL ResetEvent()	{ _ASSERT(m_hObject != NULL); return ::ResetEvent(m_hObject); }
	BOOL Unlock()		{ return TRUE; }
};

#define DEFAULT_SPIN_COUNT	4000

class RENET_API CCriticalSectionBS : public CSynchObject
{
public:
	CCriticalSectionBS(BOOL bUseSpinCount = FALSE, DWORD dwSpinCount = DEFAULT_SPIN_COUNT);

	virtual ~CCriticalSectionBS()  
	{ 
		::DeleteCriticalSection(&m_CS); 	
	}

public:
	virtual BOOL IsCriticalSection()  { return TRUE; }

	operator CRITICAL_SECTION*() { return (CRITICAL_SECTION*) &m_CS; }
	CRITICAL_SECTION m_CS;

public:
	BOOL Lock() 
	{ 
		::EnterCriticalSection(&m_CS);
		return TRUE; 
	}

	BOOL Lock(DWORD dwTimeout)
	{
		return Lock();
	}

	BOOL Unlock() 
	{ 
		::LeaveCriticalSection(&m_CS); 		
		return TRUE; 
	}
};

class _condition;

class _monitor
{
protected:
	HANDLE				m_hMutex;

	virtual void		lock()    { WaitForSingleObject( m_hMutex, INFINITE ); }
	virtual void		release() { ReleaseMutex( m_hMutex ); }
	friend class		_condition;

public:
	_monitor() 
	{ 
		m_hMutex = CreateMutex( NULL, FALSE, NULL); 
	}

	~_monitor() 
	{ 
		CloseHandle( m_hMutex ); 
	}
};

class _semaphore
{
protected:
	HANDLE				m_hSemaphore;   

public:
	_semaphore( int nSema = 1 ) 
	{ 
		m_hSemaphore = CreateSemaphore( NULL, 0, nSema, NULL ); 
	}

	~_semaphore() 
	{ 
		CloseHandle( m_hSemaphore ); 
	}

	void				P() { WaitForSingleObject( m_hSemaphore, INFINITE ); }
	BOOL				V() { return ReleaseSemaphore( m_hSemaphore, 1, NULL ); }
};

class _condition
{
protected:
	_semaphore		m_Semaphore;  
	long			m_nSemaCount;    
	_monitor*		m_Monitor;

public:
	_condition( _monitor* mon, int nSema = 1 ) : m_Semaphore( nSema )
	{ 
		m_Monitor		= mon;
		m_nSemaCount	= 0; 
	}
	
	~_condition() {};

	void wait() 
	{
		::InterlockedIncrement( &m_nSemaCount );			
		m_Monitor->release();
		m_Semaphore.P();
		m_Monitor->lock();
		::InterlockedDecrement( &m_nSemaCount );
	};

	void signal() 
	{		  
		if( m_nSemaCount > 0 )   
		{
			if( !m_Semaphore.V() )
			{
				DWORD dwRet = ::GetLastError();
			}
		}
	};
};

class CRWMonitor : public _monitor
{
protected:
	long			m_nReaderCount;  
	long			m_nWriterCount;  
	long			m_nWaitingReaders; 
	long			m_nWaitingWriters; 

	_condition*		m_ReadWaitQueue; 
	_condition*		m_WriteWaitQueue;  

public:
	CRWMonitor(int nSema = 1)
	{
		m_ReadWaitQueue		= new _condition(this, nSema);
		m_WriteWaitQueue	= new _condition(this, nSema);


		m_nReaderCount		= m_nWriterCount 
			= 0;
		m_nWaitingReaders	= m_nWaitingWriters 
			= 0; 
	}

	~CRWMonitor()
	{
		delete m_ReadWaitQueue;
		delete m_WriteWaitQueue;
	}

	void			StartReading();
	void			StopReading();    
	void			StartWriting();
	void			StopWriting();    
};

inline void CRWMonitor::StartReading()
{
	lock(); 

	if( m_nReaderCount || m_nWaitingWriters )
	{
		::InterlockedIncrement( &m_nWaitingReaders );		
		m_ReadWaitQueue->wait();
		::InterlockedDecrement( &m_nWaitingReaders );		
	}
	::InterlockedIncrement( &m_nReaderCount );	

	m_ReadWaitQueue->signal();  
	release();
}

inline void CRWMonitor::StopReading()
{
	lock();   
	::InterlockedDecrement( &m_nReaderCount );

	if( ( m_nReaderCount == 0 ) && m_nWaitingWriters )
		m_WriteWaitQueue->signal();
	else
		m_ReadWaitQueue->signal();  

	release();
}

inline void CRWMonitor::StartWriting()
{
	lock();

	if( m_nWriterCount || m_nReaderCount )
	{
		::InterlockedIncrement( &m_nWaitingWriters );		
		m_WriteWaitQueue->wait();
		::InterlockedDecrement( &m_nWaitingWriters );		
	}
	::InterlockedIncrement( &m_nWriterCount );	

	release();
}

inline void CRWMonitor::StopWriting()
{
	lock();

	::InterlockedDecrement( &m_nWriterCount );  
	if( m_nWaitingReaders )
		m_ReadWaitQueue->signal();
	else
		m_WriteWaitQueue->signal();

	release();
}

END_NAMESPACE