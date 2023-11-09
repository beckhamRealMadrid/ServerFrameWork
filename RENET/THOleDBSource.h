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

			HANDLE				m_hDBThread;			// DBThread �ڵ�
			HANDLE				m_hDBKillEvent;			// DBThead ���� �̺�Ʈ.
			HANDLE				m_hResultEvent;			// DB �۾� ����� ������ �뺸�� �̺�Ʈ.

			BOOL				m_bCloseQueryForThread;	// Thread�� ���� �޼��� �Է��� ������
			BOOL				m_bEnableReport;		// Report Flag
};

END_NAMESPACE