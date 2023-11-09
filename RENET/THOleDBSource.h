#pragma once

#include "DBStruct.h"
#include "OleDBSource.h"
#include "MsgQueue.h"

START_NAMESPACE

class COleDBSession;

class RENET_API CTHOleDBSource : public COleDBSource
{
public:
								CTHOleDBSource(void);
	virtual						~CTHOleDBSource(void);
	virtual	BOOL				InitDBModule( DB_INITIALIZE_DESC* desc );
	virtual	COleDBSession*		ReserveNewOleDBSession( DWORD dwThreadID );
	static	unsigned __stdcall	WorkerThread( void *lpParamaeter );
			void				GetDBResult();
			void				SetDBResultEvent( HANDLE val )			{ m_hResultEvent = val;	}
			void				SetPerformanceReport( BOOL bEnable )	{ m_bEnableReport = bEnable; }
			void				THAcceptQueryInput()					{ ProtectInputMessage(FALSE); }
			void				THIgnoreQueryInput()					{ ProtectInputMessage(TRUE); }
			DBBINDING*			THCreateParamInfo( WORD wParamNum );
			void				THReleaseParamInfo( DBBINDING* pBinding );
			BOOL				THReleaseRecordset( DBRECEIVEDATA* pResultData );
			BOOL				THChangeDB( char* szDbName, BYTE bReturnResult, DWORD dwQueryUID );
			void				THOpenRecord( char* szQuerySQL, DWORD dwQueryUID, void* pData, DWORD dwMaxNumRows );
			void				THExecuteSQL( char* szQuerySQL, BYTE bReturnResult, DWORD dwQueryUID, void* pData );
			void				THExecuteSQLByParam( char* szQuerySQL, DBBINDING* pBinding, void* pParamValue, DWORD dwParamValueSize,
														BYTE bParamNum, BYTE bReturnResult, DWORD dwQueryUID, void* pData );
private:
			BOOL				PostDBMessage( LPDBCMDMSG pMsg );
			void				InformDBInput();
			void				InformDBOutput( DBRECEIVEDATA* pResult );
			void				ProcessQuery();
			void				ProcessSelectQuery( LPDBCMDMSG pMsg );
			void				ProcessExecuteQuery( LPDBCMDMSG pMsg );
			void				ProcessExecuteWithParamQuery( LPDBCMDMSG pMsg );
			void				ProcessChangeDBQuery( LPDBCMDMSG pMsg );
			BOOL				KSCToUnicode( char *pKsc, WCHAR *pUni );
			BOOL				UnicodeToKSC( WCHAR *pUni, char *pKsc, int nKscSize );			
			void				ProtectInputMessage( BOOL bProtect )	{ m_bCloseQueryForThread = bProtect; }
			void				SwitchInputQueue()						{ m_pInputQueue->SwitchQueues(); }
			void				SwitchOutputQueue()						{ m_pOutputQueue->SwitchQueues(); }
			void				OutputReport( char* szMsg )				{ m_InitDesc.ReportFunc(szMsg);	}
private:
			CMsgQueue*			m_pInputQueue;
			CMsgQueue*			m_pOutputQueue;

			HANDLE				m_hDBThread;			// DBThread 핸들
			HANDLE				m_hDBKillEvent;			// DBThead 종료 이벤트.
			HANDLE				m_hResultEvent;			// DB 작업 결과가 있음을 통보할 이벤트.

			BOOL				m_bCloseQueryForThread;	// Thread에 쿼리 메세지 입력을 막을때
			BOOL				m_bEnableReport;		// Report Flag
};

END_NAMESPACE