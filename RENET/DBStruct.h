#pragma once

#include <initguid.h>
#include <oledb.h>

START_NAMESPACE

#define DBINITCONSTANTS
#define INITGUID

#define DEF_EXCUTE_START				1
#define DEF_EXCUTE_TIMEOUT				2
#define DEF_EXCUTE_END					3

#define DEF_IID_NULL					0
#define DEF_IID_IROWSETCHANGE			1
#define DEF_IID_IROWSET					2

#define QUERY_TYPE_SELECT				1		//select
#define QUERY_TYPE_EXECUTE				2		//update, delete, sp....
#define QUERY_TYPE_EXECUTE_BY_PARAM		3
#define QUERY_TYPE_CHANGE_DB			4

#define MAX_DB_CONNECTION				30

#define DEFAULT_RETURNED_MAX_ROWS		20
#define DEFAULT_ROWS_PER_READ			10
#define DEFAULT_STRING_LENGTH			128

#define MAX_ROW_PER_READ				100
#define MAX_ROW_SIZE					1024
#define MAX_RETURNED_ROW_NUM			20
#define MAX_PARAM_VALUE_SIZE			10240
#define MAX_SQL_STRING_LENGTH			1023
#define MAX_SCHEMA_BUFFER_SIZE			128

#define FAIL_CHECK(ret, x1, x2)			if(FAILED((x1)))	{ (ret) = (x2); __leave; }
#define FAIL_RETURN(x1, ret)			if(FAILED(x1))		{ ErrorDisplay((x1)); return (ret); }

#pragma pack(push,1)
struct RECEIVEDATA_SELECT
{
	DWORD			dwRowCount;	// 결과가 몇행반환되는지 
	DWORD			dwRowSize;	// 한 행의 Data Size
	DBBINDING*		pBindings;	// 칼럼 정보
	BYTE			bColCount;	// 칼럼 갯수 
	char*			pResult;	// 실제 받아오는 결과 데이터 
};

struct RECEIVEDATA_EXECUTE
{
	DWORD			dwEffected;
};

struct DBRECEIVEDATA
{
	BYTE			bQueryType;	// 쿼리 타입 
	DWORD			dwQueryUID;	// 쿼리번호 
	int				nError;		// 음수이면 Error

	union
	{
		RECEIVEDATA_SELECT	select;
		RECEIVEDATA_EXECUTE execute;
		RECEIVEDATA_EXECUTE execute_by_param;
	} Query;

	void*			pData;		//부가정보 
};

struct PARAMVALUE
{
	int		Age;
	double	Eye;
	BYTE	Myimage[16];
};

struct BLOBDATA
{
	DBSTATUS            dwStatus;   
	DWORD               dwLength; 
	ISequentialStream*  pISeqStream;
};

struct DBSCHEMA
{
	char szSchemaBuffer[ MAX_SCHEMA_BUFFER_SIZE ];

	DBSCHEMA()
	{
		memset( szSchemaBuffer, 0, sizeof(szSchemaBuffer) );
	}
};

struct RENET_API DB_INITIALIZE_DESC
{
	BYTE		bUsingThread;
	BYTE		bUsingEventObject;
	BYTE		bMaxParamNum;						// ExecuteSqlByParam에서 수용할수 있는 최대 파라미터 갯수
	WORD		wMaxNumOfProcessMessage_Input;		// DLL에서 한번에 수용할수 있는 메세지 갯수..  (메세지용 메모리풀 갯수)
	WORD		wMaxNumOfProcessMessage_Output;		// DLL에서 한번에 수용할수 있는 메세지 갯수..  (메세지용 메모리풀 갯수)
	WORD		wMaxRowPerRead;						// DB에서 읽어드릴때 한번에 몇줄씩 읽어드릴것인가.. ( 한쿼리에 여러번 나눠서 읽음 )
	WORD		wMaxReturnedRowNum;					// Select에서 나올수 있는 행의 최대 갯수
	WORD		wMaxReturnedColNum;					// Select에서 나올수 있는 칼럼의 최대 갯수
	DWORD		dwMaxRowSize;						// 결과값 한 행의 최대 사이즈 
	UINT		uMessage;
	HWND		hWndToPostMessage;
	void		(*OutputMessageFunc)(char*);		// 메세지 출력할 함수 포인터 
	void		(*ReceiveFunc)(DBRECEIVEDATA*);		// ObjectFunction 함수 포인터 
	void		(*ReportFunc)(char*);				// ReportFunc	
};
#pragma pack(pop)

#pragma pack(push,1)

struct DBMSG_OPENRECORD
{
	DWORD dwMaxNumRows;
	DWORD dwRowPerRead;
};

struct DBMSG_EXECUTESQL
{
	BYTE	bReturnResult;
};

struct DBMSG_EXECUTESQL_BY_PARAM
{
	BYTE		bReturnResult;
	BYTE		bParamNum;
	DBBINDING*	pBinding;
	char		szParamValueBuf[ MAX_PARAM_VALUE_SIZE ];
};

struct DBMSG_CHANGE_DB
{
	BYTE		bReturnResult;
};

typedef struct DBCommandMsg
{
	BYTE	bQueryType;
	DWORD	dwThreadID;
	DWORD	dwQueryUID;
	void*	pData;
	WCHAR	wszQuery[ MAX_SQL_STRING_LENGTH + 1 ];

	union
	{
		DBMSG_OPENRECORD			select;
		DBMSG_EXECUTESQL			execute;
		DBMSG_EXECUTESQL_BY_PARAM	execute_by_param;
		DBMSG_CHANGE_DB				change_db;

	} AddInfo;

} *LPDBCMDMSG;

#pragma pack(pop)

END_NAMESPACE