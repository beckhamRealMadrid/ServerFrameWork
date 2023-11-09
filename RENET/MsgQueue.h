#pragma once

#include "SpinLock.h"
#include "MsgQueueUnit.h"

START_NAMESPACE

// INSERT 하는 넘이 FOREGROUD로 정의
// 대신 있는 넘을 처리하고 지우는 넘이 BACKGROUND
class CMsgQueue
{
public:
					CMsgQueue(DWORD dwMaxNodeNum = 100);
					~CMsgQueue();
	// 큐 스위칭.
	void			SwitchQueues();

	// 공용 큐 메쏘드	
	int				GetCount(BOOL bForeground);
	BOOL			IsEmpty(BOOL bForeground);

	// forground용 메쏘드
	POSITION_		AddTail(void* newElement);

	// background용 메쏘드
	void			RemoveAll();
	POSITION_		GetHeadPosition();
	void*			GetNext(POSITION_& pos);
private:
	CMsgQueueUnit*	GetForegroundQueue();	// insert 하는 큐.
	CMsgQueueUnit*	GetBackgroundQueue();	// remove 하는 큐.
private:
	LONG volatile	m_lForegroundQueue;		// 현재 큐를 표시.
	CSpinLock		m_Lock;
	CMsgQueueUnit*	m_pMsgQueue[2];
};

END_NAMESPACE