#pragma once

#include "Que.h"
#include "SynchObject.h"
#include "ScopedLock.h"

START_NAMESPACE

#define DEFAULT_CHUNK_SIZE	3000

#ifdef _DEBUG
//#	define _DUMP_ALLOCATED_ITEM 1
#endif

template <class T>
class CChunkAllocatorST
{
protected:
	typedef std::list<T*>	ITEMBLOCK;
	typedef std::set<T*>	ITEMSET;
	typedef void (T::*INIT_CALLBACK) (long);
	typedef void (*DUMP_CALLBACK) (T* pItem);

	INIT_CALLBACK 	m_lpfInitializer;
	DUMP_CALLBACK	m_lpfDumpCallback;

	long			m_nChunkSize;
	CQue<T*>		m_FreeItemQueue;
	ITEMBLOCK		m_ItemChunkArray;

#if _DUMP_ALLOCATED_ITEM
	ITEMSET			m_AllocatedItems;
#endif

	DWORD			m_dwParam;

	DWORD			m_nAllocatedItemNum;
	DWORD			m_nCapacity;

public:
	
	CChunkAllocatorST( long nChunkSize = DEFAULT_CHUNK_SIZE, INIT_CALLBACK lpInitializer = NULL, DWORD dwParam = 0 )
	{ 
		m_nChunkSize = nChunkSize;		
		m_nAllocatedItemNum = 0;

		m_lpfInitializer = lpInitializer;
		m_dwParam = dwParam;

		m_lpfDumpCallback= NULL;
	}

	~CChunkAllocatorST() 
	{ 
		Release(); 
	}

	int	 GetChunkSize()					{ return m_nChunkSize; }
	void SetChunkSize(long nChunkSize)	{ ::InterlockedExchange( &m_nChunkSize, nChunkSize ); }
	int	 GetItemBlockNum()				{ return m_ItemChunkArray.size(); }
	int  GetFreeItemNum()				{ return m_FreeItemQueue.GetSize(); }
	int	 GetAllocatedItemNum()			{ return m_nAllocatedItemNum; }

	void SetDumpCallbacks(DUMP_CALLBACK lpDumpCallback)
	{
		m_lpfDumpCallback = lpDumpCallback;
	}

	T* NewItem()
	{
		T* pT = PopFreeItem();
		if (pT == NULL)
			pT = ThereIsNoFreeItem();

#if _DUMP_ALLOCATED_ITEM
		if (m_lpfDumpCallback != NULL)
			m_AllocatedItems.insert(pT);
#endif
		++m_nAllocatedItemNum;

		return pT;
	}

	void FreeItem(T* pT)
	{
		if (!pT)
		{
			_ASSERT(0);
			return;
		}

		m_FreeItemQueue.Enque(pT);

#if _DUMP_ALLOCATED_ITEM
		if (m_lpfDumpCallback != NULL)
		{
			ITEMSET::iterator it = m_AllocatedItems.find(pT);
			_ASSERT(it != m_AllocatedItems.end());
			m_AllocatedItems.erase(it);
		}
#endif
		--m_nAllocatedItemNum;
	}

	void GetUsage(DWORD& nTotal,DWORD& nUsed,DWORD& nFree)
	{
		nTotal = m_nCapacity;
		nUsed = m_nAllocatedItemNum;
		_ASSERT( nUsed <= nTotal);
		nFree = nTotal - nUsed;
	}

	void Release()
	{
		for ( ITEMBLOCK::iterator it = m_ItemChunkArray.begin(); it != m_ItemChunkArray.end(); ++it )
		{
			T* pBlock = *it;
			if (pBlock)
				delete [] pBlock;
		}
		m_ItemChunkArray.clear();

#if _DUMP_ALLOCATED_ITEM
		m_AllocatedItems.clear();
#endif
	}

	void Dump(int count = -1)
	{
#if _DUMP_ALLOCATED_ITEM
		if (m_lpfDumpCallback != NULL)
		{
			if (count == -1)
				count = m_AllocatedItems.size();

			int i = 0;

			T* pItem = NULL;
			for (ITEMSET::iterator it = m_AllocatedItems.begin(); it != m_AllocatedItems.end(); ++it, ++i)
			{
				pItem = (*it);
				(m_lpfDumpCallback)(pItem);

				if (i == count)
					break;
			}
		}
#endif
	}

protected:
	BOOL AllocItemBlock()
	{
		if ( !m_FreeItemQueue.IsEmpty() )
		{
			return FALSE;
		}		

		T* pItemBlock = new T[m_nChunkSize];
		if ( pItemBlock )
		{
			m_ItemChunkArray.push_back(pItemBlock);

			for ( int i = 0; i < m_nChunkSize; i++ )
				m_FreeItemQueue.Enque( &pItemBlock[i] );

			m_nCapacity = DWORD(m_ItemChunkArray.size()) * m_nChunkSize;

			return TRUE;
		}

		return FALSE;
	}

	T* ThereIsNoFreeItem()
	{
		AllocItemBlock();
		return PopFreeItem();
	}

	T* PopFreeItem()
	{
		T* pT = NULL;
		m_FreeItemQueue.Deque(pT);

		if ( pT != NULL && m_lpfInitializer != NULL )
			(pT->*m_lpfInitializer)(m_dwParam);

		return pT;
	}
};

// multi-thread safe
template <class T>
class CChunkAllocatorMT : public CChunkAllocatorST<T>
{
public:
	CChunkAllocatorMT( long nChunkSize = DEFAULT_CHUNK_SIZE, INIT_CALLBACK lpInitializer = NULL, DWORD dwParam = 0 )
		: m_CS(TRUE), CChunkAllocatorST<T>( nChunkSize, lpInitializer, dwParam ) 
	{
		
	}

	~CChunkAllocatorMT(void) { Release(); }

protected:
	CCriticalSectionBS m_CS;

public:
	T* NewItem()
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		return CChunkAllocatorST<T>::NewItem();
	}

	void FreeItem(T* pT)
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		CChunkAllocatorST<T>::FreeItem(pT);
	}

	int	 GetItemBlockNum() 
	{ 
		SCOPED_LOCK_SINGLE(&m_CS);
		return CChunkAllocatorST<T>::GetItemBlockNum(); 
	}

	int  GetFreeItemNum()  
	{ 
		SCOPED_LOCK_SINGLE(&m_CS);
		return CChunkAllocatorST<T>::GetFreeItemNum(); 
	}

	void Release()
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		CChunkAllocatorST<T>::Release();
	}

	void Dump(int count = -1)
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		CChunkAllocatorST<T>::Dump(count);
	}

	void GetUsage(DWORD& nTotal,DWORD& nUsed,DWORD& nFree)
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		CChunkAllocatorST<T>::GetUsage(nTotal, nUsed, nFree);
	}
};

END_NAMESPACE
