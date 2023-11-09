#pragma once

#include "Singleton.h"

START_NAMESPACE

struct OVERLAPPED_BASE;

class RENET_API CIOCPManager : public CSingleton <CIOCPManager>
{
public:
								CIOCPManager();
	virtual						~CIOCPManager();	
	static	unsigned __stdcall	WorkerThread( void *lpParamaeter );
	const	HANDLE				GetIOCPHandle()	{ return m_hIOCP; }
	const	HANDLE				GetKillHandle()	{ return m_hKillManager; }
			DWORD				WaitForSignal();
			bool				AddIOPort( HANDLE IOHandle, DWORD dwKeyValue );
			bool				PostCompletion( DWORD dwCompletionKey, OVERLAPPED_BASE* pContext );
			bool				Create( int nWorkerThreadCount );
			void				Destroy();	
private:	
			HANDLE				m_hIOCP;
			HANDLE				m_hKillManager;
			PHANDLE				m_arrWorkerThread;
			int					m_nWorkerThreadCount;
};

#define IOCP	CIOCPManager::GetInstance()

END_NAMESPACE