#pragma once

START_NAMESPACE

class CSessionManager;

#define CHECK_HEARTBEAT	10000	// 10초에 한번씩 체크한다.	// 기획팀과 협의하여 30초로 바꾼다.
#define CHECK_COUNT		6		// 1분이 되면, 살아있는 지 문의.
#define NOT_ALIVE		9		// 1분 30초동안 수신된 내용이 없으면 죽인다.

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