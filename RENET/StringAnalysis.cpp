#include "stdafx.h"
#include "StringAnalysis.h"

START_NAMESPACE

CStringAnalysis::CStringAnalysis()
{
	m_pBuf= NULL;
	m_pctypeBuf= NULL;
	m_pbtypeBuf= NULL;
}

CStringAnalysis::CStringAnalysis(char *pSrc)
{
	m_pBuf= NULL;
	m_pctypeBuf= NULL;
	m_pbtypeBuf= NULL;

	SetString(pSrc);
}

CStringAnalysis::~CStringAnalysis()
{
	if(m_pBuf)		delete m_pBuf;
	if(m_pctypeBuf)	delete []m_pctypeBuf;
	if(m_pbtypeBuf)	delete []m_pbtypeBuf;

	m_pctypeBuf= NULL;
	m_pbtypeBuf= NULL;
}

void CStringAnalysis::SetString(char *pSrc)
{
	int	nSize = strlen(pSrc) + 1;

	if(m_pBuf) delete m_pBuf;
	m_pBuf= new char [nSize];

	memcpy(m_pBuf, pSrc, nSize);

	Update();
}

void CStringAnalysis::Update()
{
	// Allocation
	int	nLen = strlen(m_pBuf);

	if(m_pctypeBuf)	delete []m_pctypeBuf;
	if(m_pbtypeBuf)	delete []m_pbtypeBuf;
	m_pctypeBuf= new ECType [nLen];
	m_pbtypeBuf= new EBType [nLen];


	// Initial
	memset(&m_nTotalChars, 0, sizeof(int) * (ctypeSpecial+1));


	// Calculation
	ECType	ctypeBuf;

	for( int i = 0 ; i < nLen ; i++ )
	{
		if(m_pBuf[i] & 0x80)
		{
			unsigned char	cBuf1	= m_pBuf[i];
			unsigned char	cBuf2	= m_pBuf[i+1];
			unsigned short	wBuf	= (unsigned short)cBuf1<<8 | cBuf2;

			if(wBuf >= 0xB0A1  &&  wBuf <= 0xC8FE)		{ ctypeBuf= ctypeHangle;	m_nTotalChars[ctypeHangle]++; }
			else if(wBuf >= 0xCAA1  &&  wBuf <= 0xFDFE)	{ ctypeBuf= ctypeHanja;		m_nTotalChars[ctypeHanja]++; }
			else if(wBuf >= 0xA1A1  &&  wBuf <= 0xACFE)	{ ctypeBuf= ctypeSpecial;	m_nTotalChars[ctypeSpecial]++; }

			m_pctypeBuf[i]		= ctypeBuf;
			m_pctypeBuf[i+1]	= ctypeBuf;
			m_pbtypeBuf[i]		= btypeFirst;
			m_pbtypeBuf[i+1]	= btypeSecond;
			i++;
		}
		else
		{
			unsigned char	cBuf	= m_pBuf[i];

			if(cBuf == ' ')							{ ctypeBuf= ctypeSpace;		m_nTotalChars[ctypeSpace]++; }
			else if(cBuf >= 'a'  &&  cBuf <= 'z')	{ ctypeBuf= ctypeEnglish;	m_nTotalChars[ctypeEnglish]++; }
			else if(cBuf >= 'A'  &&  cBuf <= 'Z')	{ ctypeBuf= ctypeEnglish;	m_nTotalChars[ctypeEnglish]++; }
			else if(cBuf >= '0'  &&  cBuf <= '9')	{ ctypeBuf= ctypeNumber;	m_nTotalChars[ctypeNumber]++; }
			else									{ ctypeBuf= ctypeOther1;	m_nTotalChars[ctypeOther1]++; }

			m_pctypeBuf[i]	= ctypeBuf;	
			m_pbtypeBuf[i]	= btypeNone;
		}
	}
}

int CStringAnalysis::GetTotalChars(CStringAnalysis::ECType ctype)
{
	return m_nTotalChars[ctype];
}

CStringAnalysis::ECType CStringAnalysis::GetCharType(int nByteIndex)
{
	return m_pctypeBuf[nByteIndex];
}

CStringAnalysis::EBType CStringAnalysis::GetByteType(int nByteIndex)
{
	return m_pbtypeBuf[nByteIndex];
}

END_NAMESPACE