#pragma once

START_NAMESPACE

class RENET_API CFrameSkip
{
public:
	
	// 멤버변수와 생성/소멸자.
			CFrameSkip	()	{ Clear(); }
	virtual	~CFrameSkip	()	{}
	
	inline void Clear()
	{
		SetFramePerSec( 60.0f );
		m_Timer = 0.0f;
	}

	inline void SetFramePerSec( float fps )
	{
		m_SecPerFrame = 1.0f/fps;
	}

	// 원하는 프레임보다 너무 빠르면,
	// false를 반환해서 코드를 동작시키지 않도록 한다.
    // dt는 '초'단위 (밀리초 아님!!!) 
	inline bool Update( float dt )
	{
		m_Timer += dt;

		if ( m_Timer < 0 )
			return false;		
		
		// 한프레임에 해당하는 시간을 뺀다.
		m_Timer -= m_SecPerFrame;

		return true;
	}

	// Update후에 호출해서 frame skip을 수행해야 하는지 검사한다.
	// frame skip이 일어나야 한다면 true를 반환한다.
	inline bool IsFrameSkip()
	{
		return m_Timer >= 0;
	}
	
protected:
	float m_Timer;
	float m_SecPerFrame;
};

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
inline float GetAppTimeDelay()
{
	static bool     m_bTimerInitialized = false;
	static bool     m_bUsingQPF         = false;
	static LONGLONG m_llQPFTicksPerSec  = 0;

	// Initialize the timer
	if( ! m_bTimerInitialized )
	{
		m_bTimerInitialized = true;
		
		// Use QueryPerformanceFrequency() to get frequency of timer.  If QPF is
		// not supported, we will timeGetTime() which returns milliseconds.
		LARGE_INTEGER qwTicksPerSec;
		m_bUsingQPF = (QueryPerformanceFrequency( &qwTicksPerSec )==0) ? false : true;
		if( m_bUsingQPF )
			m_llQPFTicksPerSec = qwTicksPerSec.QuadPart;
	}

	double fTime;
	double fElapsedTime;
	if( m_bUsingQPF )
	{
		static LONGLONG m_llLastElapsedTime = 0;
		LARGE_INTEGER qwTime;
		
		QueryPerformanceCounter( &qwTime );

		// Return the elapsed time
		fElapsedTime = (double) ( qwTime.QuadPart - m_llLastElapsedTime ) / (double) m_llQPFTicksPerSec;
		m_llLastElapsedTime = qwTime.QuadPart;
	}
	else
	{
		// Get the time using timeGetTime()
		static double m_fLastElapsedTime  = 0.0;
		fTime = timeGetTime() * 0.001;

		// Return the elapsed time
		fElapsedTime = (double) (fTime - m_fLastElapsedTime);
		m_fLastElapsedTime = fTime;
	}

	return (float) fElapsedTime;
}

#define	ONEFRAME	0.016666666666666666

END_NAMESPACE