#pragma once

START_NAMESPACE

#define	MEMORY_DESC_SIZE			8
#define	MEMORY_BLOCK_HEADER_SIZE	4

typedef void* STMPOOL_HANDLE;

struct MEMORY_DESC
{
	void*			m_pAddr;
	MEMORY_DESC*	m_pNext;
};

struct MEMORY_BLOCK
{
	MEMORY_DESC*	m_pDesc;
	char			m_pMemory[1];
};

class CLookAsideList
{
public:
					CLookAsideList();
	DWORD			GetMaxSizePerObject()	{ return m_dwUnitSize; }
public:
	MEMORY_DESC*	m_pBaseDesc;
	DWORD			m_dwCommitedBlockNum;	
	DWORD			m_dwDefaultCommitBlockNum;
	DWORD			m_dwUnitSize;
	DWORD			m_dwMaxBlockNum;
	void*			m_pLinearMemoryPool;
	void*			m_pLinearDescPool;
};

STMPOOL_HANDLE	__stdcall	CreateStaticMemoryPool();
void			__stdcall	ReleaseStaticMemoryPool( STMPOOL_HANDLE pool );
BOOL			__stdcall	InitializeStaticMemoryPool( STMPOOL_HANDLE pool, DWORD dwUnitSize, DWORD dwDefaultCommitNum, DWORD dwMaxNum );
void*			__stdcall	LALAlloc( STMPOOL_HANDLE pool );
void			__stdcall	LALFree( STMPOOL_HANDLE pool, void* pMemory );

END_NAMESPACE