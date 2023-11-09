#pragma once

START_NAMESPACE

#define PROFILER_SUPPORT

#define INI_DB_FILEPATH					"ConnectDB.ini"
#define INI_DB_SECTION					"DB"
#define INI_DB_KEY_IP					"IP"
#define INI_DB_KEY_IP_DEFAULT			"211.47.193.159"
#define INI_DB_KEY_DBNAME				"Name"
#define INI_DB_KEY_DBNAME_DEFAULT		"GameDB"
#define INI_DB_KEY_ID					"ID"
#define INI_DB_KEY_ID_DEFAULT			"beckham"
#define INI_DB_KEY_PASSWORD				"Password"
#define INI_DB_KEY_PASSWORD_DEFAULT		"final6263"
#define INI_DB_SECTION_TABLE			"TABLE"
#define INI_DB_KEY_TABLENAME			"Name"
#define INI_DB_KEY_TABLENAME_DEFAULT	"ServerTable"

#define NETWORK_SENDBUFFER_SIZE			2048
#define NETWORK_RECVBUFFER_SIZE			8192

#define IOCP_ERROR						-1
#define IOCP_RECV						1
#define IOCP_SEND						2
#define IOCP_ACCEPT						3
#define IOCP_CONNECT					4	
#define IOCP_CLOSE						5
#define IOCP_POSTSEND					6
#define IOCP_TRANSMITFILE_COMPLETED		7
#define IOCP_TRANSMITFILE_RECV			8

#define ERROR_ACCEPT					-1
#define ERROR_NONE						0
#define ERROR_INVALIDPACKET				1
#define ERROR_INVALIDSOCKET				2
#define ERROR_SEND						3
#define ERROR_RECV						4
#define ERROR_TRANS						5
#define ERROR_FORCELOGOUT				6
#define ERROR_LOGINFAILED				7
#define ERROR_HEARTBEAT_TIMEOUT			8
#define ERROR_TRANSMITFILE				9
#define ERROR_IDENTIFY					10

#define NETWORK_UDP_SENDBUFFER_SIZE		512
#define NETWORK_UDP_RECVBUFFER_SIZE		8192

#define	SEND_BUFFER_HEADER_SIZE			5

#define HEADER_SENDTIME					0
#define HEADER_STATE					4
#define HEADER_FLAG						5
#define HEADER_SEQID					6
#define HEADER_WRITE					10

#define RECV_HEADER_FLAG				0 
#define RECV_HEADER_SEQID				1
#define RECV_HEADER_SIZE				5
#define RECV_HEADER_ID					7

#define MAX_CAPACITY					4

#define	DEFAULT_RESEND_TIME				5000
#define	SOCKET_CLOSE_WAIT_TIME			5000
#define	SOCKET_CONNECT_WAIT_TIME		5000
//#define	SOCKET_KEEPALIVE_TIME			5000
#define	SOCKET_KEEPALIVE_TIME			1000
#define	SOCKET_ALIVE_TIME				SOCKET_KEEPALIVE_TIME * 30
#define CONNECT_AGAIN_MAX_COUNT			3

#define	MAX_PACKET_SIZE					512
#define	STRING_PACKET_SIZE				200
#define	PACKET_BUFFER_SIZE				256
#define	PACKET_DUMMY_SIZE				64
#define	PACKET_HEADER_SIZE				4
#define	PACKET_TAIL_SIZE				2
#define	UDP_HEADER_SIZE					5
#define	ADDRESSINFO_SIZE				16

#ifdef _DEBUG
#	define CHECK_MEMORY()				if( !_CrtCheckMemory( ) ) __asm int 3
#else
#	define CHECK_MEMORY()
#endif

#ifndef SAFE_DELETE
#	define	SAFE_DELETE( p )			{ if ( p ) delete p; p = NULL; } 
#endif

#ifndef SAFE_DELETE_ARRAY
#	define	SAFE_DELETE_ARRAY( p )		{ if ( p ) delete []p; p = NULL; }
#endif

#ifndef SAFE_RELEASE
#	define	SAFE_RELEASE( p )			{ if ( p ) { p->Release(); p = NULL; } }
#endif

#define FAIL_VALUE(x1,x2)				{ x1 = x2; __leave; }
#define FAIL_VALUE_GOTO(x1,x2,label)	{ x1 = x2; goto label; }
#define FAIL_GOTO(label)				{ goto label; }
#define SAFE_DELETE(x)					if (x) { delete (x); x = NULL; }
#define SAFE_CLOSEHANDLE(x)				if ((x) != NULL) { CloseHandle( x ); x = NULL; }
#define TRUTH_DECISION(ConditionExp)	(ConditionExp)? true:false;

typedef unsigned (__stdcall *PTHREAD_START)(void*);

#define BEGINTHREADEX(psa, cbStack, pfnStartAddr, pvParam, fdwCreate, pdwThreadId) \
	((HANDLE)_beginthreadex((void*)(psa), \
	(unsigned)(cbStack), (PTHREAD_START)(pfnStartAddr),\
	(void*)(pvParam), (unsigned)(fdwCreate), (unsigned*)(pdwThreadId)))

template< class __T >
struct LessAddress : public std::binary_function< __T, __T, bool>
{
	bool operator()(const __T& x, const __T& y) const
	{
		if ( ( !x ) || ( !y ) )
			return true;

		return  ( memcmp( (void*)x, (void*)y, ADDRESSINFO_SIZE ) < 0 ? 1 : 0 );
	}
};

enum ePEER_STATE
{
	LISTEN			= 1,
	WAIT			= 2,
	CONNECTING		= 3,
	ESTABLISHED		= 4,
	CLOSE			= 5,		
};

enum eUDPSOCKET_SEND_FLAG
{
	FLAG_PULSE				= 1,
	FLAG_TRANS_SUCCESS,
	FLAG_RESEND,
	FLAG_PACKET,
	FLAG_CONNECTING,
	FLAG_ESTABLISHED,
	FLAG_CLOSE,
	FLAG_MAX,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define	DECLARE_PARSING_MSG_HANDLER(_class_name)														\
protected:																								\
	typedef bool (_class_name::*PARSING_MSG_HANDLER)(DWORD);											\
	static PARSING_MSG_HANDLER	_class_name##ParsingMsgHandler[0x0fff];									\
	friend class _class_name##MsgMapper;

#define BEGIN_PARSING_MSG_MAP(_class_name, except_handler)												\
	_class_name::PARSING_MSG_HANDLER _class_name::_class_name##ParsingMsgHandler[0x0fff] = { NULL, };	\
	class _class_name##MsgMapper																		\
	{																									\
	public:																								\
		_class_name##MsgMapper()																		\
		{																								\
			for ( int i = 0; i < 0x0fff; ++i )															\
			{																							\
				_class_name::_class_name##ParsingMsgHandler[i] = &_class_name::except_handler;			\
			}
		
#define BIND_PARSING_MSG_HANDLER(_class_name, _id, _handler)											\
		_class_name::_class_name##ParsingMsgHandler[_id] = &_class_name::_handler;

#define END_PARSING_MSG_MAP(_class_name)																\
		}																								\
	};																									\
	_class_name##MsgMapper	g##_class_name##MsgMapper;

#define CALL_PARSING_MSG_HANDLER(_class_name, _id, _recvsize )											\
	((this)->*_class_name::_class_name##ParsingMsgHandler[_id])(_recvsize);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_MESSAGEMAP()										\
public:																\
	static void InitMessageMap();									\
protected:															\
	static CSession::msgmap _mapMessage;							\
	virtual int ProcessMessage(CMsgRecv& msg)						\
	{																\
		Synchronized so( &m_soRecv );								\
		CSession::msgmapitor itor = _mapMessage.find(msg.ID());		\
		if (itor != _mapMessage.end())								\
			(this->*(*itor).second) ( msg );						\
		if (!IsActive())											\
			return ERROR_INVALIDSOCKET;								\
		else														\
			return ERROR_NONE;										\
	}															

#define BEGIN_MESSAGEMAP(theClass)									\
	RENET::CSession::msgmap theClass::_mapMessage;					\
	void theClass::InitMessageMap()									\
	{																

#define ON_MESSAGE(ID, DISPATCHER) _mapMessage.insert(msgmap::value_type((ID), (PFMsg)DISPATCHER));

#define END_MESSAGEMAP()	}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_MESSAGE_MAP()										\
public:																\
	static void InitMessageMap();									\
protected:															\
	static CConnection::msgmap _mapMessage;							\
	virtual void ProcessMessage(CPacket& Msg)						\
	{																\
		CConnection::msgmapitor itor = _mapMessage.find(Msg.ID());	\
		if (itor != _mapMessage.end())								\
			(this->*(*itor).second) ( Msg );						\
	}																\
	virtual bool IsOnBindMessageID(WORD id)							\
	{																\
		CConnection::msgmapitor itor = _mapMessage.find(id);		\
		if (itor != _mapMessage.end())								\
			return true;											\
		else														\
			return false;											\
	}								

#define BEGIN_MESSAGE_MAP(theClass)									\
	RENET::CConnection::msgmap theClass::_mapMessage;				\
	void theClass::InitMessageMap()									\
	{																													

#define BIND_MESSAGE(ID, DISPATCHER)	_mapMessage.insert(msgmap::value_type((ID), (PFMsg)&DISPATCHER));	

#define END_MESSAGE_MAP()	}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

END_NAMESPACE