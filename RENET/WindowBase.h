#pragma once

START_NAMESPACE

#define ANCHOR_EDGE_MASK_LEFT	(BYTE)0x01
#define ANCHOR_EDGE_MASK_RIGHT	(BYTE)0x02
#define ANCHOR_EDGE_MASK_TOP	(BYTE)0x04
#define ANCHOR_EDGE_MASK_BOTTOM	(BYTE)0x08
#define ANCHOR_EDGE_MASK_ALL	(BYTE)0x0F

#define ANCHOR_EDGE_TYPE_MASK_LEFT_RATIO	(BYTE)0x10
#define ANCHOR_EDGE_TYPE_MASK_RIGHT_RATIO	(BYTE)0x20
#define ANCHOR_EDGE_TYPE_MASK_TOP_RATIO		(BYTE)0x40
#define ANCHOR_EDGE_TYPE_MASK_BOTTOM_RATIO	(BYTE)0x80

#define RECT_WIDTH( rc) (rc.right - rc.left)
#define RECT_HEIGHT( rc) (rc.bottom - rc.top)

#define RECT_CENTER_X(rc) ((rc.right + rc.left) / 2)
#define RECT_CENTER_Y(rc) ((rc.bottom + rc.top) / 2)

#define MAKERECT_BY_CENTER_SIZE( rc, center, size)	\
    rc.left = center.x - size.cx / 2;				\
	rc.top = center.y - size.cy / 2;				\
	rc.right = rc.left + size.cx;					\
	rc.bottom = rc.top + size.cy;

class CWindowBase;
typedef std::map<HWND,CWindowBase *> GLOBALWINDOWLIST;

struct sChildWindowAnchor
{
	HWND	m_hWnd;
	BYTE	m_btMask;
	float	m_AnchorPosition[ 4];

	inline int GetEdgeOffset(RECT& rcWindow,int index)
	{
		switch( index)
		{
			case 0:
			case 1:
				return static_cast<int>((float)( rcWindow.right - rcWindow.left) * m_AnchorPosition[index]);
				break;
			case 2:
			case 3:
				return static_cast<int>((float)( rcWindow.bottom - rcWindow.top) * m_AnchorPosition[index]);
				break;
		}
		return 0;	
	}
};

class RENET_API CWindowBase
{
public:
						CWindowBase();
	virtual				~CWindowBase();
public:
	typedef LRESULT (CWindowBase::*WIN32_MESSAGE_HANDLER)(WPARAM wParam,LPARAM lParam);
	typedef LRESULT (CWindowBase::*WIN32_COMMAND_HANDLER)(WPARAM wParam,LPARAM lParam);
	typedef LRESULT (CWindowBase::*WIN32_MESSAGE_HANDLER)(WPARAM wParam,LPARAM lParam);

	struct sProcessMessageHandle
	{
		CWindowBase* m_pWindow;
		WIN32_MESSAGE_HANDLER m_pHandler;
	};

	typedef std::map<UINT,WIN32_MESSAGE_HANDLER>	WIN32_MESSAGE_HANDLEMAP;
	typedef std::map<WPARAM,WIN32_COMMAND_HANDLER>	WIN32_COMMAND_HANDLEMAP;
	typedef std::map<UINT,sProcessMessageHandle>	WIN32_PROCESS_MESSAGE_HANDLEMAP;
	typedef std::list<sChildWindowAnchor>			CHILDANCHOR_LIST;

			bool		RegisterClass(LPCTSTR lpszClassName,UINT style,HCURSOR hCursor,HBRUSH hBackground,HICON hIcon,HINSTANCE hInstance);
			bool		CreateWindowBase(DWORD dwExStyle,LPCTSTR lpClassName,LPCTSTR lpWindowName,DWORD dwStyle,RECT rcWindow,HWND hParent,HMENU hMenu,HINSTANCE hInstance);
			bool		DestroyWindowBase();
			void		ShowWindow(int nCmdShow);
			void		UpdateWindow();
			void		CenterWindow();
			HWND		CreateControl(HINSTANCE hInstance,LPCTSTR szClassName,LPCTSTR szWindowName,DWORD dwStyle,RECT rcControl,UINT nID,DWORD dwExStyle = 0);
			RECT		GetClientRect();
			RECT		GetWindowRect();
			RECT		GetParentWindowRect();	
			void		RedrawWindow(RECT* rcUpdate,UINT flags);
			void		RegHandler(UINT message,WIN32_MESSAGE_HANDLER handle);
			void		RegCommand(WORD controlID,WORD controlMessage, WIN32_COMMAND_HANDLER handle);	
			void		RegHandlerProcess(UINT message,WIN32_MESSAGE_HANDLER handler);
	inline	HWND		GetHWnd()				{ return m_hWnd; }
	inline	HINSTANCE	GetHInstance()			{ return m_hInstance; }
	inline	void		SetHWnd(HWND hWnd)		{ m_hWnd = hWnd; }
	virtual LRESULT		WindowProc(UINT iMsg,WPARAM wParam,LPARAM lParam);	
	static	LRESULT		CALLBACK GlobalWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static	LRESULT		Process_ProcessMessage(UINT message,WPARAM wParam,LPARAM lParam);
protected:
			void		SetupChildAnchor(HWND hChildWnd,BYTE btMask);
			void		UpdateChildAnchor(); 
			LRESULT		ProcessUserHandle(UINT message,WPARAM wParam,LPARAM lParam);
			LRESULT		ProcessUserCommand(UINT message,WPARAM wParam,LPARAM lParam);
public:
			CWindowBase*					m_pWndParent;
protected:	
			CHILDANCHOR_LIST				m_ChildAnchorList;
			HWND							m_hWnd;
			HINSTANCE						m_hInstance;
			bool							m_bDialog;
			WIN32_MESSAGE_HANDLEMAP			m_MessageHandleMap;
			WIN32_COMMAND_HANDLEMAP			m_CommandHandleMap;
	static GLOBALWINDOWLIST					m_GlobalWindowList;
	static WIN32_PROCESS_MESSAGE_HANDLEMAP	m_ProcessMessageHandleMap;	
};

END_NAMESPACE