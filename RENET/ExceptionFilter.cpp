#include "stdafx.h"
#include "ExceptionFilter.h"

START_NAMESPACE

#pragma pack(push, exception)
#pragma pack(8)
#include <imagehlp.h>
#pragma pack(pop, exception)

#pragma comment(lib, "imagehlp.lib")

LPFUNCFOREXCEPTION g_pFuncForException = NULL;
int g_nAcceptPort = 0;

LONG RENET_API __stdcall ExceptionFilter( _EXCEPTION_POINTERS* pExceptionInfo ) 
{
	// Initialize symbols..
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);

	if( SymInitialize(GetCurrentProcess(), NULL, TRUE) == FALSE ) 
		return EXCEPTION_EXECUTE_HANDLER;

    // create dump file..
	FILE* pFile = NULL;
	TCHAR strPath[256] = {0,};
	TCHAR strStackDumpPath[256] = {0,};
	
	::GetModuleFileName( NULL, strPath, 256 );
	for ( int i = (int)_tcslen(strPath) - 1 ; i >= 0 ; i-- )
	{
		if ( strPath[i] == '\\' )
		{
			strPath[i] = '\0';
			break;
		}
	}

	::StringCbPrintf( strStackDumpPath, _MAX_PATH, _T("%s\\ExceptionStackDump%d"), strPath, g_nAcceptPort );
	::SetCurrentDirectory( strStackDumpPath );

	if( (pFile = fopen("ExceptionStackDump.txt", "at")) == NULL ) 
		return EXCEPTION_EXECUTE_HANDLER;		

	const char* msg = NULL;

	switch ( pExceptionInfo->ExceptionRecord->ExceptionCode )
    {
		case EXCEPTION_ACCESS_VIOLATION:         msg = "EXCEPTION_ACCESS_VIOLATION"; break;
		case EXCEPTION_DATATYPE_MISALIGNMENT:    msg = "EXCEPTION_DATATYPE_MISALIGNMENT"; break;
		case EXCEPTION_BREAKPOINT:               msg = "EXCEPTION_BREAKPOINT"; break;
		case EXCEPTION_SINGLE_STEP:              msg = "EXCEPTION_SINGLE_STEP"; break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    msg = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"; break;
		case EXCEPTION_FLT_DENORMAL_OPERAND:     msg = "EXCEPTION_FLT_DENORMAL_OPERAND"; break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:       msg = "EXCEPTION_FLT_DIVIDE_BY_ZERO"; break;
		case EXCEPTION_FLT_INEXACT_RESULT:       msg = "EXCEPTION_FLT_INEXACT_RESULT"; break;
		case EXCEPTION_FLT_INVALID_OPERATION:    msg = "EXCEPTION_FLT_INVALID_OPERATION"; break;
		case EXCEPTION_FLT_OVERFLOW:             msg = "EXCEPTION_FLT_OVERFLOW"; break;
		case EXCEPTION_FLT_STACK_CHECK:          msg = "EXCEPTION_FLT_STACK_CHECK"; break;
		case EXCEPTION_FLT_UNDERFLOW:            msg = "EXCEPTION_FLT_UNDERFLOW"; break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:       msg = "EXCEPTION_INT_DIVIDE_BY_ZERO"; break;
		case EXCEPTION_INT_OVERFLOW:             msg = "EXCEPTION_INT_OVERFLOW"; break;
		case EXCEPTION_PRIV_INSTRUCTION:         msg = "EXCEPTION_PRIV_INSTRUCTION"; break;
		case EXCEPTION_IN_PAGE_ERROR:            msg = "EXCEPTION_IN_PAGE_ERROR"; break;
		case EXCEPTION_ILLEGAL_INSTRUCTION:      msg = "EXCEPTION_ILLEGAL_INSTRUCTION"; break;
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: msg = "EXCEPTION_NONCONTINUABLE_EXCEPTION"; break;
		case EXCEPTION_STACK_OVERFLOW:           msg = "EXCEPTION_STACK_OVERFLOW"; break;
		case EXCEPTION_INVALID_DISPOSITION:      msg = "EXCEPTION_INVALID_DISPOSITION"; break;
		case EXCEPTION_GUARD_PAGE:               msg = "EXCEPTION_GUARD_PAGE"; break;
		case EXCEPTION_INVALID_HANDLE:           msg = "EXCEPTION_INVALID_HANDLE"; break;
//		case EXCEPTION_POSSIBLE_DEADLOCK:        msg = "EXCEPTION_POSSIBLE_DEADLOCK"; break;
		case CONTROL_C_EXIT:                     msg = "CONTROL_C_EXIT"; break;
		case 0xE06D7363:                         msg = "Microsoft C++ Exception"; break;
		default:                                 msg = "UNKNOWN"; break;
    }

    char tszTemp[256] = {0,};
	fprintf(pFile, "*============================================================*\n");
	fprintf(pFile, "\tDate / Time\t\t\t: %s   ", _strdate(tszTemp));
	fprintf(pFile, "%s\n", _strtime(tszTemp));
	fprintf(pFile, "\tProcessID / ThreadID: 0x%08X / ", GetCurrentProcessId());
	fprintf(pFile, "0x%08X\n", GetCurrentThreadId());
	fprintf(pFile, "\tExceptionCode\t\t: 0x%08X\n", pExceptionInfo->ExceptionRecord->ExceptionCode);
	fprintf(pFile, "*============================================================*\n");

	HANDLE hProcess		= GetCurrentProcess();
	HANDLE hThread		= GetCurrentThread();
	CONTEXT& context	= *pExceptionInfo->ContextRecord;

	STACKFRAME stackFrame = {0,};
	stackFrame.AddrPC.Offset	= context.Eip;
	stackFrame.AddrPC.Mode		= AddrModeFlat;
	stackFrame.AddrStack.Offset	= context.Esp;
	stackFrame.AddrStack.Mode	= AddrModeFlat;
	stackFrame.AddrFrame.Offset	= context.Ebp;
	stackFrame.AddrFrame.Mode	= AddrModeFlat;

	// stackwalk!!
	for(int i = 0; i < 512; i++) 
	{ 
		if( stackFrame.AddrPC.Offset == 0 ) break;

		if( StackWalk( IMAGE_FILE_MACHINE_I386, hProcess, hThread, &stackFrame, &context, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL) != FALSE ) 
		{
			DWORD dwDisplacement = 0;
			char chSymbol[256] = {0,};
			PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL)chSymbol;

			pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
			pSymbol->MaxNameLength = sizeof(chSymbol) - sizeof(PIMAGEHLP_SYMBOL) + 1;

			if(SymGetSymFromAddr(hProcess, stackFrame.AddrPC.Offset, &dwDisplacement, pSymbol))
				fprintf(pFile, "0x%08X - %s() + %xh\n", stackFrame.AddrPC.Offset, pSymbol->Name, stackFrame.AddrPC.Offset-pSymbol->Address);
			else
				fprintf(pFile, "0x%08X - [Unknown Symbol:Error %u]\n", stackFrame.AddrPC.Offset, GetLastError());

			IMAGEHLP_MODULE module = {sizeof(IMAGEHLP_MODULE), 0,};
			if(SymGetModuleInfo(hProcess, stackFrame.AddrPC.Offset, &module) != FALSE) 
			{
				fprintf(pFile, "\tImageName: %s\n", module.ImageName);
				fprintf(pFile, "\tLoadedImageName: %s\n", module.LoadedImageName);
			}

			IMAGEHLP_LINE line = {sizeof(IMAGEHLP_LINE), 0,};
			for(int i = 0; i < 512; ++i) 
			{
				if(SymGetLineFromAddr(hProcess, stackFrame.AddrPC.Offset - i, &dwDisplacement, &line) != FALSE) 
				{
					fprintf(pFile, "\tFile: %s, %u Line\r\n", line.FileName, line.LineNumber);
					break;
				}
			}
		} 
		else break;
	} 

	fprintf(pFile, "\n");
	fclose(pFile);

	if(NULL != g_pFuncForException)
	{
		g_pFuncForException( _T("ExceptionStackDump.txt") );
		fprintf(pFile, "\n### Function For Exception Process Was Called\n");
	}

	SymCleanup(hProcess);

	HANDLE hCurrentProcess = ::GetCurrentProcess();
	::TerminateProcess( hCurrentProcess, 0 );
	SAFE_CLOSEHANDLE( hCurrentProcess );

	return EXCEPTION_EXECUTE_HANDLER;
}

void RENET_API SetFuncForException( LPFUNCFOREXCEPTION pFunc, int nAcceptPort )
{
	g_pFuncForException = pFunc;
	g_nAcceptPort = nAcceptPort;
}

END_NAMESPACE