#pragma once

START_NAMESPACE

class RENET_API CFrameSkip
{
public:
	
	// ��������� ����/�Ҹ���.
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

	// ���ϴ� �����Ӻ��� �ʹ� ������,
	// false�� ��ȯ�ؼ� �ڵ带 ���۽�Ű�� �ʵ��� �Ѵ�.
    // dt�� '��'���� (�и��� �ƴ�!!!) 
	inline bool Update( float dt )
	{
		m_Timer += dt;

		if ( m_Timer < 0 )
			return false;		
		
		// �������ӿ� �ش��ϴ� �ð��� ����.
		m_Timer -= m_SecPerFrame;

		return true;
	}

	// Update�Ŀ� ȣ���ؼ� frame skip�� �����ؾ� �ϴ��� �˻��Ѵ�.
	// frame skip�� �Ͼ�� �Ѵٸ� true�� ��ȯ�Ѵ�.
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