#include "StdAfx.h"
#include "DynamicMemoryPool.h"

START_NAMESPACE

#define _WIN32_WINNT		0x0500
#define ERROR_CHECK_CODE	79

CDynamicMemoryPool::CDynamicMemoryPool( DWORD dwMaxBlockNum, DWORD dwObjectNumPerBlock, DWORD dwMaxObjectSize, BYTE bFreeUnusedBlock )
{
	memset(m_ppMemoryArray, 0, sizeof(m_ppMemoryArray));
	memset(m_dwUsedMemoryIndex, 0, sizeof(m_dwUsedMemoryIndex));

	m_dwMaxObjectNum = dwObjectNumPerBlock;
	m_dwMaxObjectSize = dwMaxObjectSize;
	m_dwMaxMemoryBlock = dwMaxBlockNum;
	m_bFreeUnusedBlock = bFreeUnusedBlock;

	m_pFreeNodeHead = NULL;
	m_pFreeNodeTail = NULL;
	m_pAllocNodeHead = NULL;
	m_pAllocNodeTail = NULL;
	m_nFreeListCount = 0;
	m_nAllocListCount = 0;
	m_nAllocatedBlockNum = 0;
	m_nCurBlock	= -1;

	InitializeMemoryBlock();

#ifdef DEBUG_MODE
	m_fpLog = NULL;
	InitLog();
	Log("DynamicMemoryPool Initialized.");
#endif
}

CDynamicMemoryPool::~CDynamicMemoryPool()
{
#ifdef MULTI_THREAD_SUPPORT
	DeleteCriticalSection(&m_csMemory);
#endif

	ClearAllMemoryBlock();

#ifdef DEBUG_MODE
	Log("DynamicMemoryPool Terminated.");
	FreeLog();
	m_fpLog = NULL;
#endif
}

void CDynamicMemoryPool::InitializeMemoryBlock()
{
	if (m_dwMaxObjectNum <= 0) return;
	if (m_dwMaxObjectSize<= 0) return;

#ifdef MULTI_THREAD_SUPPORT
	InitializeCriticalSectionAndSpinCount(&m_csMemory,200);

	EnterCriticalSection(&m_csMemory);
#endif

	AllocNewBlock();

#ifdef MULTI_THREAD_SUPPORT
	LeaveCriticalSection(&m_csMemory);
#endif
}

void CDynamicMemoryPool::ClearAllMemoryBlock()
{
	ClearFreeList();

	if(m_bFreeUnusedBlock)	
		ClearAllocList();

	for( DWORD dwBlockIndex = 0; dwBlockIndex < m_nAllocatedBlockNum; dwBlockIndex++ )
	{
		delete [] m_ppMemoryArray[ dwBlockIndex ];
		m_ppMemoryArray[ dwBlockIndex ] = NULL;

		m_dwUsedMemoryIndex[ dwBlockIndex ] = 0;
	}

	m_nAllocatedBlockNum = 0;		
}

BOOL CDynamicMemoryPool::AllocNewBlock()
{
	m_nCurBlock++;

	if(m_nCurBlock >= (int)m_dwMaxMemoryBlock)
	{
		MessageBox( NULL, _T("MemoryBlock Overflow!"), 0, 0 );
		return FALSE;
	}

	// 먼저 메모리 배열 생성 
	m_ppMemoryArray[ m_nCurBlock ] = new DATABLOCK*[ m_dwMaxObjectNum ];
	if (!m_ppMemoryArray[ m_nCurBlock ]) 
	{
		MessageBox( NULL, _T("Memory Alloc FAIL : m_ppMemoryArray"), _T("ERROR"), NULL );
		return FALSE;
	}

	memset(m_ppMemoryArray[ m_nCurBlock ], 0, sizeof(DATABLOCK*)*m_dwMaxObjectNum);

	m_dwUsedMemoryIndex[ m_nCurBlock ] = m_dwMaxObjectNum;

	// 실제 메모리 생성 
	for( DWORD i = 0; i < m_dwMaxObjectNum; i++ )
	{
		DATABLOCK* pBlock = new DATABLOCK;
		memset(pBlock, 0, sizeof(DATABLOCK));

#ifdef ERROR_CHECK
		pBlock->szData = new char[ m_dwMaxObjectSize + 8 ];	// ObjectSize + Error Code(4byte) + Datablock pointer(4byte)
#else
		pBlock->szData = new char[ m_dwMaxObjectSize + 4 ];	// ObjectSize + Datablock pointer(4byte)
#endif
		if (!pBlock->szData) 
		{
			MessageBox( NULL, _T("Memory Alloc FAIL : pBlock->szData"), _T("ERROR"), NULL );
			return FALSE;
		}

#ifdef	DEBUG_MODE
		memset(pBlock->szData, 0xff, m_dwMaxObjectSize);	// test
#else
		memset(pBlock->szData, 0, m_dwMaxObjectSize);
#endif

#ifdef ERROR_CHECK
		memset(pBlock->szData + m_dwMaxObjectSize, ERROR_CHECK_CODE, 4); // 뒤에 4Byte의 Error 검출 코드 추가 
#endif

		m_dwUsedMemoryIndex[ m_nCurBlock ]--;
		m_ppMemoryArray[ m_nCurBlock ][ m_dwUsedMemoryIndex[ m_nCurBlock ] ] = pBlock;

		// Data block Pointer copy
#ifdef ERROR_CHECK
		memcpy((pBlock->szData+m_dwMaxObjectSize+4), &m_ppMemoryArray[ m_nCurBlock ][ m_dwUsedMemoryIndex[ m_nCurBlock ] ], 4);
#else
		memcpy((pBlock->szData+m_dwMaxObjectSize), &m_ppMemoryArray[ m_nCurBlock ][ m_dwUsedMemoryIndex[ m_nCurBlock ] ], 4);
#endif

		AddTailAtFreeList(pBlock);
	}

	if(m_dwUsedMemoryIndex[ m_nCurBlock ])
		__asm int 3

	m_nAllocatedBlockNum++;

	return TRUE;
}

char* CDynamicMemoryPool::Alloc()
{
#ifdef MULTI_THREAD_SUPPORT
	EnterCriticalSection(&m_csMemory);
#endif

	char* pReturnMemory = m_ppMemoryArray[ m_nCurBlock ][ m_dwUsedMemoryIndex[ m_nCurBlock ] ]->szData;

	if(m_bFreeUnusedBlock)	
	{
		RemoveAtFromFreeList( m_ppMemoryArray[ m_nCurBlock ][ m_dwUsedMemoryIndex[ m_nCurBlock ] ] );
		AddTailAtAllocList( m_ppMemoryArray[ m_nCurBlock ][ m_dwUsedMemoryIndex[ m_nCurBlock ] ] );
	}

	m_dwUsedMemoryIndex[ m_nCurBlock ]++;

	// 메모리 블럭이 꽉찼을경우 
	if(m_dwUsedMemoryIndex[ m_nCurBlock ] == m_dwMaxObjectNum)
	{
		if(m_nAllocatedBlockNum - 1 == m_nCurBlock)	// 미리 할당해 놓은 블럭이 없는경우 
		{
			AllocNewBlock();	
#ifdef DEBUG_MODE
			Log("New Block Allocated! Total Allocated Block Num is %d", m_nAllocatedBlockNum);
#endif
		}
		else
		{
			m_nCurBlock++;
		}
	}
	else if(m_dwUsedMemoryIndex[ m_nCurBlock ] > m_dwMaxObjectNum)
	{
		m_dwUsedMemoryIndex[ m_nCurBlock ] =  m_dwMaxObjectNum;
		__asm int 3	// 있을수 없는상황 ..
	}

#ifdef MULTI_THREAD_SUPPORT
	LeaveCriticalSection(&m_csMemory);
#endif

#ifdef ERROR_CHECK
	// 메모리 블럭이 정상인지 확인
	if (pReturnMemory[ m_dwMaxObjectSize ]  == ERROR_CHECK_CODE && pReturnMemory[m_dwMaxObjectSize+1]  == ERROR_CHECK_CODE &&
		pReturnMemory[m_dwMaxObjectSize+2]  == ERROR_CHECK_CODE && pReturnMemory[m_dwMaxObjectSize+3]  == ERROR_CHECK_CODE)
	{
		return pReturnMemory;
	}
	else 
	{
		MessageBox( NULL, "CDynamicMemoryPool::Alloc() : Memory Block was broken", "error", MB_OK);
		return NULL;
	}
#endif

	return pReturnMemory;
}

void CDynamicMemoryPool::Free(char* pTarget)
{
	if (!pTarget)
		return;

#ifdef MULTI_THREAD_SUPPORT
	EnterCriticalSection(&m_csMemory);
#endif

	// 현재 메모리 블럭의 사용량이 없을때 또 Free가 들어온경우 
	if(m_dwUsedMemoryIndex[ m_nCurBlock ] == 0)
	{
		if(m_nCurBlock == 0)	// 더이상 해제할 블럭이 없다. 	
		{
			__asm int 3
			return;
		}

		if(m_bFreeUnusedBlock)	
		{
			FreeBlock(m_nCurBlock);
#ifdef DEBUG_MODE
			Log("Block Freed! Total Allocated Block Num is %d", m_nAllocatedBlockNum);
#endif
		}

		m_nCurBlock--;	
	}

//	DATABLOCK* pBlock = NULL;
//	pBlock = (DATABLOCK*)(pTarget+4+4);

	DATABLOCK* pBlock;
#ifdef ERROR_CHECK
	memcpy(&pBlock, pTarget+m_dwMaxObjectSize+4, 4);
#else
	memcpy(&pBlock, pTarget+m_dwMaxObjectSize, 4);
#endif

	m_dwUsedMemoryIndex[ m_nCurBlock ]--; 
	m_ppMemoryArray[ m_nCurBlock ][ m_dwUsedMemoryIndex[ m_nCurBlock ] ] = pBlock;

#ifdef DEBUG_MODE
	memset(pBlock->szData, 0xff, m_dwMaxObjectSize);
#endif

	if(m_bFreeUnusedBlock)	
	{
		RemoveAtFromAllocList(pBlock);
		AddTailAtFreeList(pBlock);
	}

#ifdef MULTI_THREAD_SUPPORT
	LeaveCriticalSection(&m_csMemory);
#endif

#ifdef ERROR_CHECK
	if (pTarget[m_dwMaxObjectSize]  == ERROR_CHECK_CODE && pTarget[m_dwMaxObjectSize+1]  == ERROR_CHECK_CODE && 
		pTarget[m_dwMaxObjectSize+2]  == ERROR_CHECK_CODE && pTarget[m_dwMaxObjectSize+3]  == ERROR_CHECK_CODE)
	{
		return;
	}
	else 
	{
		MessageBox( NULL, "CDynamicMemoryPool::Free() : Memory Block was broken", "error", MB_OK );
		return;
	}
#endif
}

DWORD CDynamicMemoryPool::GetFreeIndexNum()
{
#ifdef MULTI_THREAD_SUPPORT
	EnterCriticalSection(&m_csMemory);
#endif

	DWORD temp  = m_dwMaxObjectNum - m_dwUsedMemoryIndex[ m_nCurBlock ];

#ifdef MULTI_THREAD_SUPPORT
	LeaveCriticalSection(&m_csMemory);
#endif
	return temp;
}

void CDynamicMemoryPool::FreeBlock(DWORD dwBlockIndex)
{
	RemoveSomeBlockFromHead(m_dwMaxObjectNum);

	delete [] m_ppMemoryArray[ dwBlockIndex ];

	m_ppMemoryArray[ dwBlockIndex ] = NULL;
	m_dwUsedMemoryIndex[ dwBlockIndex ] = 0;
	m_nAllocatedBlockNum--;
}

void CDynamicMemoryPool::RemoveAtFromFreeList(DATABLOCK* pBlock)
{
	if(!pBlock)
        __asm int 3

	if(m_pFreeNodeHead == pBlock)
	{
		m_pFreeNodeHead = pBlock->pNextListData;
	}
	else
	{
		pBlock->pPrevListData->pNextListData = pBlock->pNextListData;
	}

	if(m_pFreeNodeTail == pBlock)
	{
		m_pFreeNodeTail = pBlock->pPrevListData;
	}
	else
	{
		pBlock->pNextListData->pPrevListData = pBlock->pPrevListData;
	}

	pBlock->pNextListData = NULL;
	pBlock->pPrevListData = NULL;

	m_nFreeListCount--;
}

void CDynamicMemoryPool::RemoveAtFromAllocList(DATABLOCK* pBlock)
{
	if(!pBlock)
		__asm int 3

	if(m_pAllocNodeHead == pBlock)
	{
		m_pAllocNodeHead = pBlock->pNextListData;
	}
	else
	{
		pBlock->pPrevListData->pNextListData = pBlock->pNextListData;
	}

	if(m_pAllocNodeTail == pBlock)
	{
		m_pAllocNodeTail = pBlock->pPrevListData;
	}
	else
	{
		pBlock->pNextListData->pPrevListData = pBlock->pPrevListData;
	}

	pBlock->pNextListData = NULL;
	pBlock->pPrevListData = NULL;

	m_nAllocListCount--;
}

void CDynamicMemoryPool::AddTailAtFreeList(DATABLOCK* pBlock)
{
	if(m_pFreeNodeTail != NULL)
	{
		m_pFreeNodeTail->pNextListData = pBlock;
		pBlock->pPrevListData = m_pFreeNodeTail;
	}
	else
	{
		m_pFreeNodeHead = pBlock;
	}

	m_pFreeNodeTail = pBlock;
	m_nFreeListCount++;
}

void CDynamicMemoryPool::AddTailAtAllocList(DATABLOCK* pBlock)
{
	if(m_pAllocNodeTail != NULL)
	{
		m_pAllocNodeTail->pNextListData = pBlock;
		pBlock->pPrevListData = m_pAllocNodeTail;
	}
	else
	{
		m_pAllocNodeHead = pBlock;
	}

	m_pAllocNodeTail = pBlock;
	m_nAllocListCount++;
}

void CDynamicMemoryPool::RemoveSomeBlockFromHead(DWORD dwNum)
{
	if(m_nFreeListCount < (int)dwNum)
		__asm int 3

	DATABLOCK* pNewHead = NULL;
	DATABLOCK* pOldHead = NULL;

	for(DWORD i = 0; i < dwNum; i++)
	{
		if (m_pFreeNodeHead == NULL) 
			__asm int 3

		if(m_pFreeNodeHead == m_pFreeNodeTail)	//리스트에 하나 있는 경우 
		{
			delete [] m_pFreeNodeHead->szData;
			delete m_pFreeNodeHead;

			m_pFreeNodeHead = NULL;
			m_pFreeNodeTail = NULL;
		}
		else
		{
			pOldHead = m_pFreeNodeHead;
			pNewHead = m_pFreeNodeHead->pNextListData;

			pOldHead->pNextListData = NULL;
			pNewHead->pPrevListData = NULL;

			delete [] m_pFreeNodeHead->szData;
			delete m_pFreeNodeHead;

			m_pFreeNodeHead = pNewHead;
		}

		m_nFreeListCount--;
	}
}

void CDynamicMemoryPool::ClearFreeList()
{
	if(m_nFreeListCount <= 0)
		return;

	DATABLOCK* pNewHead = NULL;
	DATABLOCK* pOldHead = NULL;

	while(m_pFreeNodeHead)
	{
		if(m_pFreeNodeHead == m_pFreeNodeTail)	//리스트에 하나 있는 경우 
		{
			delete [] m_pFreeNodeHead->szData;
			delete m_pFreeNodeHead;
			m_pFreeNodeHead = NULL;
			m_pFreeNodeTail = NULL;
		}
		else
		{
			pOldHead = m_pFreeNodeHead;
			pNewHead = m_pFreeNodeHead->pNextListData;

			pOldHead->pNextListData = NULL;
			pNewHead->pPrevListData = NULL;

			delete [] pOldHead->szData;
			delete pOldHead;

			m_pFreeNodeHead = pNewHead;
		}

		m_nFreeListCount--;
	}

	if(m_nFreeListCount != 0)
		__asm int 3
}

void CDynamicMemoryPool::ClearAllocList()
{
	if(m_nAllocListCount <= 0)
		return;

	DATABLOCK* pNewHead = NULL;
	DATABLOCK* pOldHead = NULL;

	while(m_pAllocNodeHead)
	{
		if(m_pAllocNodeHead == m_pAllocNodeTail)	//리스트에 하나 있는 경우 
		{
			delete [] m_pAllocNodeHead->szData;
			delete m_pAllocNodeHead;
			m_pAllocNodeHead = NULL;
			m_pAllocNodeTail = NULL;
		}
		else
		{
			pOldHead = m_pAllocNodeHead;
			pNewHead = m_pAllocNodeHead->pNextListData;

			pOldHead->pNextListData = NULL;
			pNewHead->pPrevListData = NULL;

			delete [] pOldHead->szData;
			delete pOldHead;

			m_pAllocNodeHead = pNewHead;
		}

		m_nAllocListCount--;
	}

	if(m_nAllocListCount != 0)	
		__asm int 3
}

#ifdef DEBUG_MODE

void CDynamicMemoryPool::Log( char *logmsg, ... )
{
	va_list vargs;
	struct tm *now;
	time_t nowTime;

	int year, mon, day;
	static int log_year = 0, log_mon = 0, log_day = 0;
	int hour, min, sec;

	char buf[(MAX_LOG_LENGTH*10)+1];
	static char szLogFileName[80+1];

	// Argument Processing
	va_start( vargs, logmsg );

	// Get nowtime
	time( &nowTime );
	now = localtime(&nowTime);

	// Make it usable.
	year = now->tm_year + 1900;
	mon  = now->tm_mon + 1;
	day  = now->tm_mday;
	hour = now->tm_hour;
	min  = now->tm_min;
	sec  = now->tm_sec;

	// Lock...
	//	EnterCriticalSection(&m_criLog);

	if( log_year && ( (log_year != year) || (log_mon != mon) || (log_day != day) ) )
	{
		// Close fpLog
		fclose( m_fpLog );
		m_fpLog = NULL;

		// Clear log_year
		log_year = 0;
	}

	if( log_year == 0 || !m_fpLog )
	{
		// Set log_year, log_mon, log_day.
		log_year = year;
		log_mon = mon;
		log_day = day;

		sprintf( szLogFileName, ".\\[M]%d-%d-%d.log", year, mon, day );

		if( !(m_fpLog = fopen( szLogFileName, "a" )) )
		{
			// Notify ERROR
			sprintf( buf, "FATAL ERROR at Log() :: Can't open LogFile('%s')", szLogFileName );
			MessageBox(0, buf, 0, 0);
			goto lb_Exit;
		}
	}

	// Write Log rised time.
	sprintf( buf, "<%2d:%2d:%2d> ", hour, min, sec );

	// Write Log's Body.
	if( strlen( logmsg ) > (MAX_LOG_LENGTH-11) )
	{
		// Self-calling.
		Log( "Too long string - This log will be lost" );
		va_end( vargs );
		goto lb_Exit;
	}

	vsprintf( buf+11, logmsg, (vargs) );

	// To File
	if( m_fpLog )
	{
		strcat( buf, "\n" );
		fputs( buf, m_fpLog );
		fflush( m_fpLog );
	}

lb_Exit:
	//LeaveCriticalSection(&m_criLog);

	// Finish Func
	va_end( vargs );
	return;
}

void CDynamicMemoryPool::InitLog()
{
//	InitializeCriticalSection(&m_criLog);
//	InitializeCriticalSectionAndSpinCount(&m_criLog, 1000);
}

void CDynamicMemoryPool::FreeLog()
{
//	DeleteCriticalSection(&m_criLog);

	if( m_fpLog )
	{
		fclose( m_fpLog );
	}
}

#endif

END_NAMESPACE