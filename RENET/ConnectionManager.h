#pragma once

#include "Singleton.h"
#include "Synchronized.h"
#include "DynamicMemoryPool.h"

START_NAMESPACE

class CConnection;
class CRelaySession;

class RENET_API CConnectionManager : public CSingleton <CConnectionManager>  
{
public:
								CConnectionManager(void);
	virtual						~CConnectionManager(void);
			bool				Create( LPCTSTR lpszAddress, int nPort );
			void				Destroy();
			CConnection*		GetSession( SOCKADDR_IN* pAddress );
			void				RemoveActiveSessions();
			char*				Alloc()					{ return m_pMemoryPool->Alloc(); }
			void				Free( char* pTarget )	{ m_pMemoryPool->Free( pTarget ); }
	const	HANDLE				GetKillHandle()			{ return m_hEvent; }

	template <typename S>
	void CreateSessions( int nCapacity )
	{
		S::InitMessageMap();
		for ( int i = 0; i < nCapacity; i++ )
			AddSession( new S( this ) );		
	};

	template <typename S>
	void RemoveSessions()
	{
		Synchronized so( &m_soList );

		peerlistitor itor;
		for ( itor = m_listActive.begin(); itor != m_listActive.end(); ++itor )
			delete (S*)(*itor);

		for ( itor = m_listInActive.begin(); itor != m_listInActive.end(); ++itor )
			delete (S*)(*itor);
	};
protected:
	static	unsigned __stdcall	WorkerThread( void *lpParamaeter );
			void				AddSession( CConnection* pSess );
			void				AddActiveSession( CConnection* pSess, SOCKADDR_IN* pAddress );
			CConnection*		FindActiveSession( SOCKADDR_IN* pAddress );
			CConnection*		EnterSession( SOCKADDR_IN* pAddress );
			void				LeaveSession( CConnection* pSess );
			void				Run();
protected:
	typedef std::map < SOCKADDR_IN*, CConnection*, LessAddress<SOCKADDR_IN*> > peermap;
	typedef std::map < SOCKADDR_IN*, CConnection*, LessAddress<SOCKADDR_IN*> >::iterator peermapitor;
	typedef std::list < CConnection* > peerlist;
	typedef std::list < CConnection* >::iterator peerlistitor;
			
			peermap				m_mapActive;
			peerlist			m_listActive;
			peerlist			m_listInActive;	
			SectionObject		m_soList;
			SectionObject		m_soMap;
			CRelaySession*		m_pSession;
			CDynamicMemoryPool*	m_pMemoryPool;
			HANDLE				m_hThread;
			HANDLE				m_hEvent;
};	

END_NAMESPACE