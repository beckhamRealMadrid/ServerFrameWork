#pragma once

#include "MemoryBlockManager.h"

START_NAMESPACE

class CNetMsg;

typedef struct OVERLAPPED_BASE	// Overlapped Socket Use This Struct
{
	OVERLAPPED	m_ov;
	DWORD		m_dwFlag;
} *LPOVERLAPPED_BASE;

typedef std::list <CNetMsg*> netmsglist;
typedef std::list <CNetMsg*>::iterator netmsglistitor;

typedef struct OVERLAPPED_SEND : public OVERLAPPED_BASE
{
	DWORD		m_dwOffset;
	netmsglist	m_listMsgs;
	WSABUF		m_wsaBuf[64];	// Ư�� ��Ʈ��ī�忡�� WSASend���� scatter /gather io ���۰� 32�� �̻��̸� ����� ���۵��� �ʴ� ������ �ִ�!!!

	int			GetMsgCount();
	int			GetMsgSize();
	void		SendClear();
	bool		AddNetMsg( CNetMsg* pNetMsg );
	void		ProcessSendedBytes( DWORD dwBytes );
} *LPOVERLAPPED_SEND;

class RENET_API CNetMsg
{
public:	
	CNetMsg();
	bool			Create();
	void			AddRef();
	void			Release();
	int				GetRefCount();

	long volatile	m_nRefCount;
	int				m_nBufCount;
	BYTE			m_pBuf[NETWORK_SENDBUFFER_SIZE];

	static CMemoryBlockManager <CNetMsg> s_NetMsgManager;
};

typedef struct UDPSENDBUCKET
{
	SOCKADDR_IN m_ToAddr;	
	u_long		m_len;
	char		m_pBuf[NETWORK_UDP_SENDBUFFER_SIZE];

	UDPSENDBUCKET()
	{
		memset( &m_ToAddr, NULL, sizeof(SOCKADDR_IN) );
		memset( m_pBuf, NULL, sizeof(m_pBuf) );
		m_len = 0;
	}
} *LPUDPSENDBUCKET;

typedef std::list <LPUDPSENDBUCKET> netmsgudplist;
typedef std::list <LPUDPSENDBUCKET>::iterator netmsgudplistitor;

END_NAMESPACE