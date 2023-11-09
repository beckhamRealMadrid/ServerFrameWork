#pragma once

#include "DBStruct.h"

START_NAMESPACE

class COleDBSource;
class CThreadSafeStaticMemPool;

class RENET_API COleDBSession
{
public:
								COleDBSession( DB_INITIALIZE_DESC* desc, COleDBSource* pParent );
	virtual						~COleDBSession();	
			int					Create();
			int					Create( int nConnectTimeout, int nCommandTimeOut, BOOL bCommandTimeOut );
			int					OpenRecord( char* szQuerySQL, void* pRecordSet, DWORD dwMaxNumRows = DEFAULT_RETURNED_MAX_ROWS );
			DBRECEIVEDATA*		OpenRecordEx( char* szQuerySQL, DWORD dwMaxNumRows = DEFAULT_RETURNED_MAX_ROWS, DWORD dwRowPerRead = DEFAULT_ROWS_PER_READ );
			DBRECEIVEDATA*		OpenRecordExForThread( wchar_t* szQuerySQL, DWORD dwMaxNumRows = DEFAULT_RETURNED_MAX_ROWS, DWORD dwRowPerRead = DEFAULT_ROWS_PER_READ );
            int					ExecuteSQL( char* szQuerySQL );
			int					ExecuteSQLByParam( char* szQuerySQL, DBBINDING* pBinding, void* pParamValue, BYTE bParamNum );
			bool				GetData( void* pReceiveData, DBRECEIVEDATA* pResultData, DWORD dwRowNum, WORD wColumnNum );	
			bool				ReleaseRecordset( DBRECEIVEDATA* pResultData );
			DBBINDING*			CreateParamInfo( WORD wParamNum );
			BOOL				ReleaseParamInfo( DBBINDING* pBinding );
			BOOL				ChangeDB( char* szDbName );
			DBCOLUMNINFO*		GetColumnInfo( char* szQuerySQL, DWORD* pColnum );
			int					QueryDBCatalog( DBSCHEMA* pSchemaBuf, DWORD dwMaxNumRows );
			int					QueryDBTable( DBSCHEMA *pSchemaBuf, DWORD dwMaxNumRows );
			void				ProcessTimeOut();
	static	unsigned __stdcall	WorkerThread( void *lpParamaeter );
protected:
			HANDLE				GetExcuteEvent()		{ m_byTimeOut = DEF_EXCUTE_TIMEOUT; return m_hExcuteEvent; }
			void				SetExcuteInit()			{ m_byTimeOut = DEF_EXCUTE_START; }
			HANDLE				GetExcuteEndEvent()		{ m_byTimeOut = DEF_EXCUTE_END; return m_hExcuteEvent; }
private:
			bool				KSCToUnicode( char *pKsc, WCHAR *pUni );
			BOOL				UnicodeToKSC( WCHAR *pUni, char *pKsc, int nKscSize );
			LONG				SetCommandExecute( wchar_t* szQuerySQL, BYTE byType, IRowsetChange** pIRowsetChange, IRowset** pIRowset, DBPARAMS* dbParams = NULL, BOOL bParam = FALSE, LONG* cNumRows = NULL );	
			void				ErrorDisplay( HRESULT hrErr );
			void				ErrorDisplayEx( HRESULT hrErr, wchar_t* szQuery );
private:
			COleDBSource*		m_pOledbSource;
			IDBInitialize*      m_pIDBInitialize;
			IDBProperties*      m_pIDBProperties;
			IDBCreateSession*   m_pIDBCreateSession;
			IDBCreateCommand*   m_pIDBCreateCommand;
			ICommandText*       m_pICommandText;
			IAccessor*          m_pIAccessor;

			DWORD				m_dwCommandTimeOut;
			BOOL				m_bCommandTimeOut;
			BYTE				m_byTimeOut;

			HANDLE				m_hExcuteEvent;
			HANDLE				m_hDBExcuteThread;	

			CThreadSafeStaticMemPool*	m_pHrowPool;
};

END_NAMESPACE