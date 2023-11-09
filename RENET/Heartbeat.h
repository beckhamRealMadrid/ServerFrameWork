#pragma once

START_NAMESPACE

class CSessionManager;

#define CHECK_HEARTBEAT	10000	// 10�ʿ� �ѹ��� üũ�Ѵ�.	// ��ȹ���� �����Ͽ� 30�ʷ� �ٲ۴�.
#define CHECK_COUNT		6		// 1���� �Ǹ�, ����ִ� �� ����.
#define NOT_ALIVE		9		// 1�� 30�ʵ��� ���ŵ� ������ ������ ���δ�.

class CHeartbeat
{
public:
								CHeartbeat( CSessionManager* pSM, UINT nTime = 1000 );
	virtual						~CHeartbeat(void);
	static	unsigned __stdcall	WorkerThread( void *lpParamaeter );
			bool				StartHeartbeat();
			void				StopHeartbeat();
			void				HeartbeatCheck();
			void				CheckAndDrop();
private:
			HANDLE				m_hThread;
			HANDLE				m_hEventExit;
			UINT				m_nTime;
			bool				m_bInService;
			CSessionManager*	m_pSessionManager;
};

END_NAMESPACE