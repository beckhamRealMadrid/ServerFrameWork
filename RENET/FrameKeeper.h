#pragma once

START_NAMESPACE

#define DEFAULT_FPS		60
#define MAX_DELTA_TIME	1
#define CLAMP(x, min, max)  (x) = ((x) < (min) ? (min) : (x) < (max) ? (x) : (max));

class CFrameKeeper
{
public:
	CFrameKeeper()	{ Init(); }
	~CFrameKeeper() {}

protected:
	float	m_Timer;
	float	m_fSPF;
	int		m_FPS;

public:
	void Init()
	{
		SetFPS(DEFAULT_FPS);
		m_Timer = 0.0f;
	}
	
	void SetFPS(int fps) 
	{ 
		m_FPS  = fps;
		m_fSPF = 1.0f / fps; 
	}

	BOOL ItsTimeToUpdate(float delta)
	{
		m_Timer += delta;		
		if (m_Timer < m_fSPF) 
			return FALSE;
		
		m_Timer -= m_fSPF;
		return TRUE;
	}

	BOOL HaveToSkip() 
	{ 
		return (m_Timer < m_fSPF); 
	}

	float GetSPF() { return m_fSPF; }
	float GetTimer(){ return m_Timer;}
};

END_NAMESPACE
