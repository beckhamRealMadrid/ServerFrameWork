#pragma once

#include "ListeningSession.h"

START_NAMESPACE

class CIOCPManager;
class CHeartbeat;

class RENET_API CSessionManager
{
public:
					CSessionManager();
	virtual			~CSessionManager();
	virtual bool	Create( LPCTSTR szAddr, int nPort );
	virtual void	RemoveSession( CSession* pSession );
	virtual void	ErrorOnSession( CSession* pSession, DWORD dwError, int nDetail );
			int		AcceptSocket( SOCKET hSocket, LPCTSTR strIPAddr );
			void	Broadcast( CMsgSend& msgSend, bool bEncrypt = true );
			bool	FindActiveSessionAndSend( LPCTSTR strUserID, CMsgSend& msg );
			void	CloseAllSessions();
			int		GetActiveSessionCount();
			void	RemoveClosedSession();
			void	CheckAndKillSession();
	
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
		sesslistitor itor;
		for ( itor = m_listActive.begin(); itor != m_listActive.end(); ++itor )
			delete (S*)(*itor);
		
		for ( itor = m_listInActive.begin(); itor != m_listInActive.end(); ++itor )
			delete (S*)(*itor);
	};
protected:
			void	AddSession( CSession* pSess );
protected:
	typedef std::list < CSession* > sesslist;
	typedef std::list < CSession* >::iterator sesslistitor;
			SectionObject		m_soList;
			sesslist			m_listInActive;
			sesslist			m_listActive;
			CListeningSession*	m_pListener;
			CHeartbeat*			m_pHeartbeat;
};

END_NAMESPACE