#pragma once

#include "Singleton.h"

START_NAMESPACE

#define MAX_LOG_LENGTH			512
#define MAX_TEMPCOMMAND_SIZE	64
#define NUM_OF_LOG_TYPE			8
#define LOG_FATAL				0
#define LOG_IMPORTANT			1
#define LOG_NORMAL				2
#define LOG_IGNORE				3
#define LOG_DEBUG				4
#define LOG_ALL					5
#define LOG_JUST_DISPLAY		6
#define LOG_JUST_DISPLAY1		7

class RENET_API CMonitor : public CSingleton <CMonitor>
{
public:
								CMonitor(void);
	virtual						~CMonitor(void);

			void				Initialize( LPCTSTR szTitle );
			void				Release();
			void				SetConsoleSize( DWORD col,DWORD row );
			void				GetConsoleKeyInput( PINPUT_RECORD pInput, LPDWORD pResult );
			void				LogToScreen( int type, TCHAR *logmsg, ... );
			void				WriteText( TCHAR* msg, bool type = true );
			void				WaitForKeyInput();
			bool				WaitForConsoleKeyInput();
			void				GhostRemover( void );
			TCHAR*				GetTarget( void ) { return m_strTarget; }
			void				InitTarget( void ) { ZeroMemory( m_strTarget, sizeof( TCHAR ) * MAX_TEMPCOMMAND_SIZE ); }
private:
			FILE*				m_fpLog;
			CRITICAL_SECTION	m_criLog;
			HANDLE				m_hIn;
			HANDLE				m_hOut;
			DWORD				m_dwScreenBufferLastRow;
			int					m_ConsoleLogLevel;
			int					m_FileLogLevel;
			bool				m_bIsRun;
			TCHAR				m_strTarget[MAX_TEMPCOMMAND_SIZE];
};

#define MONITOR							CMonitor::GetInstance()
#define LOG_TO_SCREEN(_type, _logmsg)	MONITOR->LogToScreen(_type, _logmsg);

END_NAMESPACE
