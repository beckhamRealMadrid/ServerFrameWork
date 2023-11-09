#include "stdafx.h"
#include "SynchObject.h"

START_NAMESPACE

BOOL CSemaBS::Unlock()
{
	return Unlock(1, NULL); 
}

BOOL CSemaBS::Unlock( LONG lCount, LPLONG lpPrevCount /* =NULL */ )
{
	return ::ReleaseSemaphore(m_hObject, lCount, lpPrevCount);
}

CSynchObject::CSynchObject( LPCSTR pstrName )
{
	m_hObject = NULL;

	if (pstrName)
		m_strName = pstrName;
}

CSynchObject::~CSynchObject()
{
	if (m_hObject != NULL)
	{
		::CloseHandle(m_hObject);
		m_hObject = NULL;
	}
}

BOOL CSynchObject::Lock(DWORD dwTimeout)
{
	if (::WaitForSingleObject(m_hObject, dwTimeout) == WAIT_OBJECT_0)
		return TRUE;
	else
		return FALSE;
}

BOOL CSynchObject::Unlock(LONG, LPLONG) 
{ 
	return TRUE; 
}

BOOL CSynchObject::IsCriticalSection()  
{ 
	return FALSE; 
}

LPCTSTR CSynchObject::GetName() 
{ 
	return (LPCTSTR)(m_strName.c_str()); 
}

CSynchObject::operator HANDLE() const
{
	return m_hObject;
}

CCriticalSectionBS::CCriticalSectionBS(BOOL bUseSpinCount, DWORD dwSpinCount) : CSynchObject(NULL)
{
/*
	if (bUseSpinCount)
	{
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);

		if (sysinfo.dwNumberOfProcessors > 1)	// 1 CPU에서는 spin lock 쓰는게 더 성능이 떨어진다!
			InitializeCriticalSectionAndSpinCount(&m_CS, dwSpinCount);
		else
			InitializeCriticalSection(&m_CS);
	}
	else
*/
	InitializeCriticalSection(&m_CS);
}

END_NAMESPACE