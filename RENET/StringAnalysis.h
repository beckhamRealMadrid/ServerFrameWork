// ================================================================================
//	Note
//	----
//
//	Char Type :	문자 타입.
//				Character Type
//				Ex) 영문자인가? 한글문자인가? 한문문자인가?....
//
//	Byte Type :	2바이트 완성형 코드의 First Byte / Second Byte
//				Byte Type of Wide Byte
//				Ex) 2바이트 문자의 첫번째 바이트 인가? 두번째 바이트 인가?
// ================================================================================

#pragma once

START_NAMESPACE

class RENET_API CStringAnalysis
{
// Constructor & Destructor
public:
	CStringAnalysis();
	CStringAnalysis(char *pSrc);
	~CStringAnalysis();

// Attribute
public:
	// Enumeration of Character Type
	enum	ECType	{	ctypeOther1= 0,			// 그 밖의 1 바이트 코드
						ctypeOther2,			// 그 밖의 2 바이트 코드
						ctypeSpace,				// 아스키 코드 공백
						ctypeNumber,			// 아스키 코드 숫자
						ctypeEnglish,			// 아스키 코드 영문자
						ctypeHangle,			// 완성형 코드 한글
						ctypeHanja,				// 완성형 코드 한자
						ctypeSpecial };			// 완성형 코드 특수문자

	// Enumeration of Byte Type of Wide Byte
	enum	EBType	{	btypeNone= 0,			// 2 바이트 코드 아님
						btypeFirst,				// 2 바이트 코드의 첫번째
						btypeSecond };			// 2 바이트 코드의 두번째

private:
	char		*m_pBuf;
	ECType		*m_pctypeBuf;
	EBType		*m_pbtypeBuf;
	int			m_nTotalChars[ctypeSpecial+1];	// Enum Hack Technique

public:
	void		SetString(char *pSrc);
	char *		GetString()					{ return m_pBuf; }

// Operation
private:
	void		Update();

// Implementation
public:
	int			GetTotalChars(ECType ctype);
	ECType		GetCharType(int nByteIndex);
	EBType		GetByteType(int nByteIndex);
};

END_NAMESPACE
