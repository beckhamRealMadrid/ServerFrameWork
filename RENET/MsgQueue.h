#pragma once

#include "SpinLock.h"
#include "MsgQueueUnit.h"

START_NAMESPACE

// INSERT �ϴ� ���� FOREGROUD�� ����
// ��� �ִ� ���� ó���ϰ� ����� ���� BACKGROUND
class CMsgQueue
{
public:
					CMsgQueue(DWORD dwMaxNodeNum = 100);
					~CMsgQueue();
	// ť ����Ī.
	void			SwitchQueues();

	// ���� ť �޽��	
	int				GetCount(BOOL bForeground);
	BOOL			IsEmpty(BOOL bForeground);

	// forground�� �޽��
	POSITION_		AddTail(void* newElement);

	// background�� �޽��
	void			RemoveAll();
	POSITION_		GetHeadPosition();
	void*			GetNext(POSITION_& pos);
private:
	CMsgQueueUnit*	GetForegroundQueue();	// insert �ϴ� ť.
	CMsgQueueUnit*	GetBackgroundQueue();	// remove �ϴ� ť.
private:
	LONG volatile	m_lForegroundQueue;		// ���� ť�� ǥ��.
	CSpinLock		m_Lock;
	CMsgQueueUnit*	m_pMsgQueue[2];
};

END_NAMESPACE