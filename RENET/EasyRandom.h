#pragma once

START_NAMESPACE

class RENET_API CEasyRandom
{
public:
			CEasyRandom( int a = 0, int b = 1 );
	void	SetInterval( int a, int b );
	int		DrawRandomNumber();
	void	SetTimerSeed();
private:
	int		GetLow();
	int		GetHigh();
private:
	int		Low, High;
};

int RENET_API GetRandomNumber( int nInterval );
int RENET_API GetRandomNumber( int nLow, int nHigh );

END_NAMESPACE