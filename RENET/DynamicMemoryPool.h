#pragma once

START_NAMESPACE

/*
#define DEBUG_MODE
#define ERROR_CHECK
*/
#define MULTI_THREAD_SUPPORT

#define MAX_MEMORY_BLOCK_NUM	20
#define MAX_LOG_LENGTH			256

struct DATABLOCK
{
	char*		szData;
	DATABLOCK*	pPrevListData;
	DATABLOCK*	pNextListData;
};

class CDynamicMemoryPool
{
public:
								CDynamicMemoryPool(DWORD dwMaxBlockNum, DWORD dwObjectNumPerBlock, DWORD dwMaxObjectSize, BYTE bFreeUnusedBlock = TRUE);
	virtual						~CDynamicMemoryPool();
			char*				Alloc();
			void				Free(char* pTarget);
			void				InitializeMemoryBlock();
			void				ClearAllMemoryBlock();	
			DWORD				GetFreeIndexNum();
			DWORD				GetMaxSizePerObject()		{ return m_dwMaxObjectSize;	}	
			int					GetFreeListCount()			{ return m_nFreeListCount;	}
			int					GetAllocListCount()			{ return m_nAllocListCount;	}
protected:
			BOOL				AllocNewBlock();
			void				FreeBlock(DWORD dwBlockIndex);
			void				RemoveAtFromFreeList(DATABLOCK* pBlock);
			void				RemoveAtFromAllocList(DATABLOCK* pBlock);
			void				RemoveSomeBlockFromHead(DWORD dwNum);
			void				AddTailAtFreeList(DATABLOCK* pBlock);
			void				AddTailAtAllocList(DATABLOCK* pBlock);
			void				ClearFreeList();
			void				ClearAllocList();
#ifdef DEBUG_MODE	
			FILE*				m_fpLog;
			CRITICAL_SECTION	m_criLog;
			void				Log( char *logmsg, ... );
			void				InitLog();
			void				FreeLog();
#endif
private:
			DATABLOCK**			m_ppMemoryArray[ MAX_MEMORY_BLOCK_NUM ];
			DWORD				m_dwUsedMemoryIndex[ MAX_MEMORY_BLOCK_NUM ];
			DWORD				m_dwMaxObjectNum;
			DWORD				m_dwMaxObjectSize;
			DWORD				m_dwMaxMemoryBlock;
			int					m_nCurBlock;
			DWORD				m_nAllocatedBlockNum;
			BYTE				m_bFreeUnusedBlock;
			// FreeDataNode
			DATABLOCK*			m_pFreeNodeHead;
			DATABLOCK*			m_pFreeNodeTail;
			DATABLOCK*			m_pAllocNodeHead;
			DATABLOCK*			m_pAllocNodeTail;
			int					m_nFreeListCount;
			int					m_nAllocListCount;
			CRITICAL_SECTION	m_csMemory;	
};

END_NAMESPACE