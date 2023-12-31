#pragma once

START_NAMESPACE

#define MAXKEYBYTES 	56		// 448 bits max
#define NPASS           16		// SBox passes
#define ENC_KEY_LEN		8

//====================================================================
//  [ BOLOWFISH Algorithm ]
//	Initialize - person Key Setting
//	person Key - Encorde & Decorde
//====================================================================
class CBlowFish
{
public:
					CBlowFish();
	virtual			~CBlowFish();

			void 	Initialize( BYTE key[], int keybytes );
			DWORD	GetOutputLength( DWORD lInputLong );
			DWORD	Encode( BYTE* pInput, BYTE* pOutput, DWORD lSize );
			void	Decode( BYTE* pInput, BYTE* pOutput, DWORD lSize );
private:
			void 	Blowfish_encipher( DWORD* xl, DWORD* xr );
			void 	Blowfish_decipher( DWORD* xl, DWORD* xr );
private:
			DWORD*	m_pArray;
			DWORD	(*m_pSBoxes)[256];
};

#define ORDER_DCBA	// chosing Intel in this case

#ifdef ORDER_DCBA  	// DCBA - little endian - intel
	union aword 
	{
		DWORD		dword;
		BYTE		byte[4];
		struct 
		{
			unsigned int	byte3:8;
			unsigned int	byte2:8;
			unsigned int	byte1:8;
			unsigned int	byte0:8;
		}w;
	};
#endif

#ifdef ORDER_ABCD  	// ABCD - big endian - motorola
	union aword 
	{
		DWORD		dword;
		BYTE		byte[4];
		struct 
		{
			unsigned int	byte0:8;
			unsigned int	byte1:8;
			unsigned int	byte2:8;
			unsigned int	byte3:8;
		}w;
	};
#endif

#ifdef ORDER_BADC  	// BADC - vax
	union aword 
	{
		DWORD		dword;
		BYTE		byte[4];
		struct 
		{
			unsigned int	byte1:8;
			unsigned int	byte0:8;
			unsigned int	byte3:8;
			unsigned int	byte2:8;
		}w;
};
#endif

END_NAMESPACE