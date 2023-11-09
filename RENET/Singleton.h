#pragma once

START_NAMESPACE

template <class T>
class CSingleton  
{
protected:
	static T* ms_pSingleton;

public:
	CSingleton()
	{
		assert( !ms_pSingleton );
		int offset = (int)(T*)1 - (int)(CSingleton <T> *)(T*)1;
		ms_pSingleton = (T *)((int)this + offset);
	}

	~CSingleton()
	{
		assert( ms_pSingleton );
		ms_pSingleton = NULL;
	}

	static T* GetInstance()
	{ 
		assert( ms_pSingleton );
		return ms_pSingleton;
	}
};

template <class T> T* CSingleton <T>::ms_pSingleton = NULL;

END_NAMESPACE
