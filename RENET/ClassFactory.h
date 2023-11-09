#if !defined(AFX_CLASSFACTORY_H__CA6330BC_AF6C_460C_AED0_0E76E9D96D55__INCLUDED_)
#define AFX_CLASSFACTORY_H__CA6330BC_AF6C_460C_AED0_0E76E9D96D55__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 

struct _tcsless
{
	bool operator()(const TCHAR* tcFirst, const TCHAR* tcSecond) const
	{
		return (_tcsicmp(tcFirst, tcSecond) < 0);
	}
};

template <typename _Key, typename _Base, typename _Predicator = std::less<_Key> >
class CClassFactory
{
public:
	CClassFactory()		{};
	~CClassFactory()	{};

	typedef _Base* (*CreatorFunction) (void);
	typedef void (*DestroyFunction) (_Base*);	
	typedef std::map<_Key, CreatorFunction, _Predicator> _mapFactory;
	typedef std::map<_Key, DestroyFunction, _Predicator> _mapRemover;

	// called at the beginning of execution to register creation functions
	static _Key RegisterCreatorFunction(_Key idKey, CreatorFunction classCreator)
	{
		get_mapFactory()->insert(std::pair<_Key, CreatorFunction>(idKey, classCreator));
		return idKey;
	}

	static _Key RegisterDestroyerFunction(_Key idKey, DestroyFunction classDestroyer)
	{
		get_mapRemover()->insert(std::pair<_Key, DestroyFunction>(idKey, classDestroyer));
		return idKey;
	}

	// Tries to create instance based on the key using creator function (if provided)
	static _Base* CreateInstance(_Key idKey)
	{
		_mapFactory::iterator it = get_mapFactory()->find(idKey);
		if (it != get_mapFactory()->end())
		{
			if (it->second)
			{
				return it->second();
			}
		}

		return NULL;
	}

	static void DestroyInstance(_Key idKey, _Base* pBase)
	{
		_mapRemover::iterator it = get_mapRemover()->find(idKey);
		if (it != get_mapRemover()->end())
		{
			if (it->second)
			{
				return it->second(pBase);
			}
		}
	}

protected:
	// Map where the construction info is stored.
	// To prevent inserting into map before initialisation takes place,
	// actual map is a static member, so it will be initialised
	// at the first call
	static _mapFactory * get_mapFactory()
	{
		static _mapFactory m_sMapFactory;
		return &m_sMapFactory;
	}

	static _mapRemover * get_mapRemover()
	{
		static _mapRemover m_sMapRemover;
		return &m_sMapRemover;
	}
};

#endif 