#include "StdAfx.h"
#include "ConnectionManager.h"
#include "Connection.h"
#include "RelaySession.h"
#include "IOCPManager.h"
#include "SockSystem.h"
#include "Packet.h"
#include "Profiler.h"

START_NAMESPACE

CConnectionManager::CConnectionManager(void)
{	
	m_pSession = NULL;
	m_pMemoryPool = NULL;
	m_hThread = INVALID_HANDLE_VALUE;
	m_hEvent = INVALID_HANDLE_VALUE;		
}

CConnectionManager::~CConnectionManager(void)
{
	SAFE_DELETE( m_pSession );
	SAFE_DELETE( m_pMemoryPool );

	CPacket::ReleaseSendPacketBufferDynamicMemoryPool();
}

unsigned __stdcall CConnectionManager::WorkerThread( void* lpParamaeter )
{
	CConnectionManager* pCM = (CConnectionManager*)lpParamaeter;

	while (true)
	{
		if ( ::WaitForSingleObject( pCM->GetKillHandle(), 1 ) != WAIT_TIMEOUT )
			break;

		pCM->Run();
	}

	return 0;
}

void CConnectionManager::Run()
{
	Synchronized so( &m_soMap );

	peermapitor iter_end( m_mapActive.end() );
	for ( peermapitor iter = m_mapActive.begin(); iter != iter_end; )
	{
		CConnection* pSess = (*iter).second;

		if ( pSess->Run() )
			++iter;
		else
		{
			m_mapActive.erase( iter++ );
			LeaveSession( pSess );
		}
	}
}

bool CConnectionManager::Create( LPCTSTR lpszAddress, int nPort )
{
	CPacket::InitSendPacketBufferDynamicMemoryPool();

	m_pSession = new CRelaySession( this );

	bool bRUSocketCreate = m_pSession->Create( lpszAddress, nPort );	
	bool bAddIOPort = IOCP->AddIOPort( (HANDLE)m_pSession->GetSocket(), (DWORD)m_pSession );	
	int nReceiveResult = m_pSession->PrepareReceive();

	if ( bRUSocketCreate && bAddIOPort && ( nReceiveResult == ERROR_NONE ) )
	{
		m_hEvent = ::CreateEvent( NULL, FALSE, FALSE, NULL );

		unsigned dwID = 0;
		m_hThread = BEGINTHREADEX( NULL, 0, WorkerThread, this, 0, &dwID );	
		if ( INVALID_HANDLE_VALUE == m_hThread )
			return false;

		m_pMemoryPool = new CDynamicMemoryPool( 2, 5000, NETWORK_UDP_SENDBUFFER_SIZE, FALSE );	
	}
	else
	{
		return false;
	}

	return true;
}

void CConnectionManager::Destroy()
{
	::SetEvent( m_hEvent );
	if ( ::WaitForSingleObject( m_hThread, 5000 ) == WAIT_TIMEOUT )
		::TerminateThread( m_hThread, 0 );

	SAFE_CLOSEHANDLE( m_hThread );
	SAFE_CLOSEHANDLE( m_hEvent );
}

void CConnectionManager::AddActiveSession( CConnection* pSess, SOCKADDR_IN* pAddress )
{
	Synchronized so( &m_soMap );
	
	pSess->PostCreate( m_pSession, pAddress );	

	m_mapActive.insert( peermap::value_type( pSess->GetAddress(), pSess ) );	
}

CConnection* CConnectionManager::FindActiveSession( SOCKADDR_IN* pAddress )
{
	Synchronized so( &m_soMap );

	peermapitor iter = m_mapActive.find( pAddress );
	if ( iter != m_mapActive.end() )
		return (*iter).second;

	return NULL;
}

void CConnectionManager::RemoveActiveSessions()
{
	Synchronized so( &m_soMap );

	peermapitor iter_end( m_mapActive.end() );
	for ( peermapitor iter = m_mapActive.begin(); iter != iter_end; )
	{
		CConnection* pSess = (*iter).second;
		pSess->OnClose();
		m_mapActive.erase( iter++ );
	}

	m_mapActive.clear();
}

void CConnectionManager::AddSession( CConnection* pSess )
{
	m_listInActive.push_back( pSess );
}

CConnection* CConnectionManager::GetSession( SOCKADDR_IN* pAddress )
{	
	CConnection* pSess = NULL;	
	
	pSess = FindActiveSession( pAddress );
	if ( pSess )
		return pSess;

	std::string str;
	CSockSystem::AddressToString( str, pAddress );
//	Log( _T("EnterSession : %s"), str.c_str() );

	pSess = EnterSession( pAddress );
	if ( NULL == pSess )
		return pSess;

	AddActiveSession( pSess, pAddress );

	return pSess;		
}

CConnection* CConnectionManager::EnterSession( SOCKADDR_IN* pAddress )
{
	CConnection* pSess = NULL;

	Synchronized so( &m_soList );

	if ( m_listInActive.size() > 0 )
	{
		pSess = m_listInActive.front();
		m_listInActive.pop_front();
	}
	else
	{
		Log( _T("!!!! Peer Pool Underfloor !!!!") );
		return NULL;
	}

	m_listActive.push_back( pSess );

#ifdef PROFILER_SUPPORT
	DWORD dwID = 0;
	char buffer[65] = {0,};
	_itoa( (int)pSess, buffer, 10 );
	PROFILER->CreateProfileSession( buffer, &dwID );
	pSess->SetProfileID( dwID );
#endif

	return pSess;
}

void CConnectionManager::LeaveSession( CConnection* pSess )
{
	Synchronized so( &m_soList );

	peerlistitor itor = std::find( m_listActive.begin(), m_listActive.end(), pSess );
	if ( itor != m_listActive.end() )
	{
		(*itor)->OnClose();

		m_listInActive.push_back( *itor );
		m_listActive.erase( itor );

#ifdef PROFILER_SUPPORT
	char buffer[65] = {0,};
	_itoa( (int)pSess, buffer, 10 );
	PROFILER->DeleteProfileSession( buffer );
	pSess->SetProfileID( 0 );
#endif
	}
}

END_NAMESPACE