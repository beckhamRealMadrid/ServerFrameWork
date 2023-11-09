#pragma once

#include "SpinLock.h"
#include "LookAsideList.h"

START_NAMESPACE

class CThreadSafeStaticMemPool
{
public:
					CThreadSafeStaticMemPool(int dwObjSize, DWORD dwBaseSize, DWORD dwMaxCount);
					~CThreadSafeStaticMemPool();
	void*			Alloc();
	void			Free(void* pBlock);
private:
	DWORD			m_dwObjSize;
	STMPOOL_HANDLE	m_pPool;
	CSpinLock		m_Lock;	
};

END_NAMESPACE