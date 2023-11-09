#include "StdAfx.h"
#include "EasyRandom.h"

START_NAMESPACE

CEasyRandom::CEasyRandom( int a, int b )
{
	SetInterval( a, b );
}

void CEasyRandom::SetInterval( int a, int b )
{
	if ( a > b )
	{
		Low = 0;
		High = 1;
	} 
	else
	{
		Low = a;
		High = b;
	}
}

void CEasyRandom::SetTimerSeed()
{
	time_t SeedTime;
	struct tm SeedDate;
	SeedTime = time(0);
	SeedDate = *localtime(&SeedTime);
	int FinalSeed = SeedTime + SeedDate.tm_mday + (SeedDate.tm_mon+1) + (SeedDate.tm_year+1900);
	srand((unsigned int) FinalSeed);
}

int CEasyRandom::DrawRandomNumber()
{
	int Interval = GetHigh() - GetLow() + 1;
	int RandomOffset = rand() % Interval;
	int RandomNumber = GetLow() + RandomOffset;
	return RandomNumber;
}

int CEasyRandom::GetHigh()
{
	return High;
}

int CEasyRandom::GetLow()
{
	return Low;
}

int RENET_API GetRandomNumber( int nInterval )
{
	CEasyRandom RandomX( 0, nInterval );
	RandomX.SetTimerSeed();	

	return RandomX.DrawRandomNumber();
}

int RENET_API GetRandomNumber( int nLow, int nHigh )
{
	CEasyRandom RandomX( nLow, nHigh );
	RandomX.SetTimerSeed();	

	return RandomX.DrawRandomNumber();
}

END_NAMESPACE