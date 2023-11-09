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
	DWORD			dwRowCount;	// ����� �����ȯ�Ǵ��� 
	DWORD			dwRowSize;	// �� ���� Data Size
	DBBINDING*		pBindings;	// Į�� ����
	BYTE			bColCount;	// Į�� ���� 
	char*			pResult;	// ���� �޾ƿ��� ��� ������ 
};

struct RECEIVEDATA_EXECUTE
{
	DWORD			dwEffected;
};

struct DBRECEIVEDATA
{
	BYTE			bQueryType;	// ���� Ÿ�� 
	DWORD			dwQueryUID;	// ������ȣ 
	int				nError;		// �����̸� Error

	union
	{
		RECEIVEDATA_SELECT	select;
		RECEIVEDATA_EXECUTE execute;
		RECEIVEDATA_EXECUTE execute_by_param;
	} Query;

	void*			pData;		//�ΰ����� 
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
	BYTE		bMaxParamNum;						// ExecuteSqlByParam���� �����Ҽ� �ִ� �ִ� �Ķ���� ����
	WORD		wMaxNumOfProcessMessage_Input;		// DLL���� �ѹ��� �����Ҽ� �ִ� �޼��� ����..  (�޼����� �޸�Ǯ ����)
	WORD		wMaxNumOfProcessMessage_Output;		// DLL���� �ѹ��� �����Ҽ� �ִ� �޼��� ����..  (�޼����� �޸�Ǯ ����)
	WORD		wMaxRowPerRead;						// DB���� �о�帱�� �ѹ��� ���پ� �о�帱���ΰ�.. ( �������� ������ ������ ���� )
	WORD		wMaxReturnedRowNum;					// Select���� ���ü� �ִ� ���� �ִ� ����
	WORD		wMaxReturnedColNum;					// Select���� ���ü� �ִ� Į���� �ִ� ����
	DWORD		dwMaxRowSize;						// ����� �� ���� �ִ� ������ 
	UINT		uMessage;
	HWND		hWndToPostMessage;
	void		(*OutputMessageFunc)(char*);		// �޼��� ����� �Լ� ������ 
	void		(*ReceiveFunc)(DBRECEIVEDATA*);		// ObjectFunction �Լ� ������ 
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