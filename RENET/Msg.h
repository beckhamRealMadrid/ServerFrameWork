#pragma once

#include "MemoryBlockManager.h"
#include "NetMsg.h"

START_NAMESPACE

class CMsgRecv;
class CMsgSend;
class CSession;

class RENET_API CMSGStreamObj
{
public:
	virtual void WriteObject(CMsgSend& msg) {};
	virtual void ReadObject(CMsgRecv& msg) {};
};

class RENET_API CCircularQueue
{
public:	
				CCircularQueue(int nSize);
	virtual		~CCircularQueue();	
	void		Clear();		
protected:
	LPBYTE		m_pBuf;
	int			m_nBufSize;
	int			m_nHead;
	int			m_nTail;
	int			m_nDataSize;
	WSABUF		m_wsaBuf[2];
public:
	LPBYTE		GetBuf();
	LPWSABUF	MakeWSABuf( int* pnBufCount );	
	void		Decrypt();
	int			GetStringLength( int nIndex );
	bool		GetDataAt(int nIndex, LPVOID pData, int nSize);
	bool		IsEmpty();	
	bool		IsFull();	
	int			GetDataSize();
	int			GetEmptySize();
	void		IncHeadPos( int nAmount );	
	void		IncTailPos( int nAmount );	
};

class RENET_API CMsgRecv
{
public:
	CMsgRecv( CCircularQueue* pQue );

protected:
	CCircularQueue* m_pCQue;
	int			m_nReadPos;
public:
	void		Clear();
	bool		Decrypt();
	WORD		ID();
	WORD		GetSize();
	void		ReadData( LPVOID pData, int n );
	void		ReadString( LPSTR data, int nBufferSize );	
	CMsgRecv& 	operator >> (CMSGStreamObj& obj);
	CMsgRecv& 	operator >> (CMSGStreamObj* pObj);
	CMsgRecv& 	operator >> (bool& arg)				{ReadData(&arg, sizeof(bool));		return *this;};
	CMsgRecv& 	operator >> (char& arg)				{ReadData(&arg, sizeof(char));		return *this;};
	CMsgRecv& 	operator >> (BYTE& arg)				{ReadData(&arg, sizeof(BYTE));		return *this;};
	CMsgRecv& 	operator >> (short& arg)			{ReadData(&arg, sizeof(short));		return *this;};
	CMsgRecv& 	operator >> (WORD& arg)				{ReadData(&arg, sizeof(WORD));		return *this;};
	CMsgRecv& 	operator >> (int& arg)				{ReadData(&arg, sizeof(int));		return *this;};
	CMsgRecv& 	operator >> (DWORD& arg)			{ReadData(&arg, sizeof(DWORD));		return *this;};
	CMsgRecv& 	operator >> (float& arg)			{ReadData(&arg, sizeof(float));		return *this;};
	CMsgRecv& 	operator >> (double& arg)			{ReadData(&arg, sizeof(double));	return *this;};
	CMsgRecv& 	operator >> (unsigned __int64& arg)	{ReadData(&arg, sizeof(unsigned __int64));	return *this;};
	
	friend class CSession;
};

class RENET_API CMsgSend
{
public:
	CMsgSend();
	CMsgSend(CNetMsg* pNetMsg); 
	CMsgSend(CMsgSend& msg);
	~CMsgSend();
protected:
	LPBYTE		m_pBuf;
	LPWORD		m_pSize;
	LPWORD		m_pID;
	LPBYTE		m_pWrite;
	CNetMsg*	m_pNetMsg;
	bool		m_bEncrypted;
protected:
	bool		Attach(CNetMsg* pNetMsg);
	void		Detach();
	void		AllocBuf();	
public:
	void		WriteData(void* pData, int n);
	LPBYTE		GetBuffer();
	WORD		GetSize();
	void		Clear();
	bool		Encrypt();
	CMsgSend&	ID(WORD wID);
	CMsgSend& 	operator << (bool arg)				{WriteData(&arg, sizeof(bool));		return *this;};
	CMsgSend& 	operator << (char arg)				{WriteData(&arg, sizeof(char));		return *this;};
	CMsgSend& 	operator << (BYTE arg)				{WriteData(&arg, sizeof(BYTE));		return *this;};
	CMsgSend& 	operator << (short arg)				{WriteData(&arg, sizeof(short));	return *this;};
	CMsgSend& 	operator << (WORD arg)				{WriteData(&arg, sizeof(WORD));		return *this;};
	CMsgSend& 	operator << (int arg)				{WriteData(&arg, sizeof(int));		return *this;};
	CMsgSend& 	operator << (DWORD arg)				{WriteData(&arg, sizeof(DWORD));	return *this;};
	CMsgSend& 	operator << (float arg)				{WriteData(&arg, sizeof(float));	return *this;};
	CMsgSend& 	operator << (double arg)			{WriteData(&arg, sizeof(double));	return *this;};
	CMsgSend&	operator << (unsigned __int64 arg)	{WriteData(&arg, sizeof(unsigned __int64));	return *this;};
	CMsgSend& 	operator << (LPCSTR data);
	CMsgSend& 	operator << (LPCWSTR data);
	CMsgSend& 	operator << (CMSGStreamObj& obj);
	CMsgSend& 	operator << (CMSGStreamObj* pObj);	

	friend class CSession;
};

END_NAMESPACE
