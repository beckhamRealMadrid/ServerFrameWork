#pragma once

#ifndef _M_IX86
	#pragma message("CAuxThunk/CAuxStdThunk is implemented for X86 only!")
#endif

START_NAMESPACE

#ifdef _MT
	typedef UINT (WINAPI *PTHREAD_ROUTINE)(LPVOID lpThreadParameter);
	typedef PTHREAD_ROUTINE	LPTHREAD_ROUTINE;
#endif

#pragma pack(push, 1)

template <class T>
class CAuxThunk
{
	BYTE    m_mov;          
	DWORD   m_this;         
	BYTE    m_jmp;          
	DWORD   m_relproc;      
public:
	typedef void (T::*TMFP)();
	void InitThunk(TMFP method, const T* pThis)
	{
		union 
		{
			DWORD	func;
			TMFP	method;
		}addr;

		addr.method	= method;
		m_mov		= 0xB9;
		m_this		= (DWORD)pThis;
		m_jmp		= 0xE9;
		m_relproc	= addr.func - (DWORD)(this+1);

		FlushInstructionCache(GetCurrentProcess(), this, sizeof(*this));
	}

	FARPROC GetThunk() const 
	{
		_ASSERT(m_mov == 0xB9);
		return (FARPROC)this; 
	}
};

template <class T>
class CAuxStdThunk
{
	BYTE    m_mov;
	DWORD   m_this;
	DWORD   m_xchg_push;
	BYTE    m_jmp;
	DWORD   m_relproc;
public:
	typedef void (__stdcall T::*TMFP)();
	void InitThunk(TMFP method, const T* pThis)
	{
		union 
		{
			DWORD	func;
			TMFP	method;
		} addr;

		addr.method = method;
		m_mov		= 0xB8;
		m_this		= (DWORD)pThis;
		m_xchg_push = 0x50240487;
		m_jmp		= 0xE9;
		m_relproc	= addr.func - (DWORD)(this+1);

		FlushInstructionCache(GetCurrentProcess(), this, sizeof(*this));
	}

	FARPROC GetThunk() const 
	{
		_ASSERT(m_mov == 0xB8);
		return (FARPROC)this; 
	}
};

#pragma pack(pop)

END_NAMESPACE