// ================================================================================
//	Note
//	----
//
//	Char Type :	���� Ÿ��.
//				Character Type
//				Ex) �������ΰ�? �ѱ۹����ΰ�? �ѹ������ΰ�?....
//
//	Byte Type :	2����Ʈ �ϼ��� �ڵ��� First Byte / Second Byte
//				Byte Type of Wide Byte
//				Ex) 2����Ʈ ������ ù��° ����Ʈ �ΰ�? �ι�° ����Ʈ �ΰ�?
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
	enum	ECType	{	ctypeOther1= 0,			// �� ���� 1 ����Ʈ �ڵ�
						ctypeOther2,			// �� ���� 2 ����Ʈ �ڵ�
						ctypeSpace,				// �ƽ�Ű �ڵ� ����
						ctypeNumber,			// �ƽ�Ű �ڵ� ����
						ctypeEnglish,			// �ƽ�Ű �ڵ� ������
						ctypeHangle,			// �ϼ��� �ڵ� �ѱ�
						ctypeHanja,				// �ϼ��� �ڵ� ����
						ctypeSpecial };			// �ϼ��� �ڵ� Ư������

	// Enumeration of Byte Type of Wide Byte
	enum	EBType	{	btypeNone= 0,			// 2 ����Ʈ �ڵ� �ƴ�
						btypeFirst,				// 2 ����Ʈ �ڵ��� ù��°
						btypeSecond };			// 2 ����Ʈ �ڵ��� �ι�°

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
