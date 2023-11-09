#ifndef __EXCEPTION_FILTER_H__
#define __EXCEPTION_FILTER_H__

#pragma once

START_NAMESPACE

typedef void (*LPFUNCFOREXCEPTION)( LPTSTR szFileName );

#define SET_GLOBAL_EXCEPTION_FILTER()	::SetUnhandledExceptionFilter( ExceptionFilter );
LONG RENET_API __stdcall ExceptionFilter( _EXCEPTION_POINTERS* pExceptionInfo );
void RENET_API SetFuncForException( LPFUNCFOREXCEPTION pFunc, int nAcceptPort );

END_NAMESPACE

#endif 