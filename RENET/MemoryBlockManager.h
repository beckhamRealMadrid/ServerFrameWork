#pragma once

#include "Singleton.h"
#include "Synchronized.h"

START_NAMESPACE

#define MEMORYBLOCK_COUNT 10000

template <typename S>
class CMemoryBlockManager
{
protected:
	std::list <S*> m_listInactive;
	std::list <S*> m_listactive;
	SectionObject m_soList;

public:
	volatile long m_nMsgOut;
	void	Initialize();
	void	Destroy();

	S*		Alloc();
	void	Release(S* pMemoryBlock);
};

template <typename S>
void CMemoryBlockManager <S>::Initialize()
{
	for ( int i = 0; i < MEMORYBLOCK_COUNT; i++ )
		m_listInactive.push_back( new S() );

	m_nMsgOut = 0;
}

template <typename S>
void CMemoryBlockManager <S>::Destroy()
{
	std::list <S*>::iterator itor;
	for ( itor = m_listInactive.begin(); itor != m_listInactive.end(); ++itor )
		delete *(itor);

	for ( itor = m_listactive.begin(); itor != m_listactive.end(); ++itor )
		delete *(itor);
	
	m_listInactive.clear();
	m_listactive.clear();
}

template <typename S>
S* CMemoryBlockManager <S>::Alloc()
{
	S* pMemoryBlock = NULL; 

	Synchronized so(&m_soList);
	
	m_nMsgOut++;

	if ( m_listInactive.size() )
	{
		pMemoryBlock = m_listInactive.front();
		m_listInactive.pop_front();
	}
	else
	{
		pMemoryBlock = new S();
		Log( _T("MSG Pool underrun, Total %d MSGs Out"), m_nMsgOut);
	}

	m_listactive.push_back( pMemoryBlock );

	pMemoryBlock->Create();

	return pMemoryBlock;
}

template <typename S>
void CMemoryBlockManager <S>::Release(S* pMemoryBlock)
{
	assert( pMemoryBlock->GetRefCount() == 0 );

	Synchronized so(&m_soList);

	std::list <S*>::iterator itor = std::find( m_listactive.begin(), m_listactive.end(), pMemoryBlock );
	if ( itor != m_listactive.end() )
	{
		m_listactive.erase( itor );
		m_listInactive.push_back( pMemoryBlock );
		m_nMsgOut--;
	}	
}

END_NAMESPACE