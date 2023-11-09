#include "stdafx.h"
#include "WindowBase.h"

START_NAMESPACE

GLOBALWINDOWLIST CWindowBase::m_GlobalWindowList;
CWindowBase::WIN32_PROCESS_MESSAGE_HANDLEMAP	CWindowBase::m_ProcessMessageHandleMap;

LRESULT CALLBACK CWindowBase::GlobalWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	GLOBALWINDOWLIST::iterator it;

	LRESULT lResult = Process_ProcessMessage(uMsg, wParam, lParam);

	if( lResult != (LRESULT)-1)
		return lResult;
	
	it = m_GlobalWindowList.find( hwnd);

	if( it == m_GlobalWindowList.end())
	{
		if( uMsg == WM_INITDIALOG)
		{
			m_GlobalWindowList.insert( GLOBALWINDOWLIST::value_type( hwnd, (CWindowBase*)lParam));
			
			it = m_GlobalWindowList.find( hwnd);

			((CWindowBase*)lParam)->SetHWnd( hwnd);
		}
		else
		{
			return ::DefWindowProc( hwnd, uMsg, wParam, lParam);
		}
	}

	CWindowBase *pBase = (*it).second;

	return pBase->WindowProc( uMsg,wParam,lParam);
}

LRESULT CWindowBase::Process_ProcessMessage(UINT message,WPARAM wParam,LPARAM lParam)
{
	WIN32_PROCESS_MESSAGE_HANDLEMAP::iterator it;

	it = m_ProcessMessageHandleMap.find( message);
	if( it != m_ProcessMessageHandleMap.end())
	{
		sProcessMessageHandle& handle = (*it).second;

		return (*handle.m_pWindow.*handle.m_pHandler)(wParam, lParam);
	}

	return -1;
}

CWindowBase::CWindowBase()
{
	m_hWnd = NULL;
	m_pWndParent = NULL;
	m_hInstance = NULL;
	m_bDialog = false;
	
	m_ChildAnchorList.clear();
}

CWindowBase::~CWindowBase()
{

}

bool CWindowBase::RegisterClass(LPCTSTR lpszClassName,UINT style,HCURSOR hCursor,HBRUSH hBackground,HICON hIcon,HINSTANCE hInstance)
{
	WNDCLASS wndclass;

	ZeroMemory( &wndclass, sizeof( WNDCLASS));
	wndclass.style = style;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = hIcon;
	wndclass.hCursor = hCursor;
	wndclass.hbrBackground = hBackground;
	wndclass.lpszClassName = lpszClassName;
	wndclass.lpfnWndProc = GlobalWindowProc;

	return ::RegisterClass( &wndclass) != 0;
}

bool CWindowBase::CreateWindowBase(DWORD dwExStyle,LPCTSTR lpClassName,LPCTSTR lpWindowName,DWORD dwStyle,RECT rcWindow,HWND hParent,HMENU hMenu,HINSTANCE hInstance)
{
	CREATESTRUCT cs;

	ZeroMemory( &cs, sizeof( CREATESTRUCT));
	cs.hInstance	= hInstance;
	cs.hMenu		= hMenu;
	cs.hwndParent	= hParent;
	cs.cx			= RECT_WIDTH( rcWindow);
	cs.cy			= RECT_HEIGHT( rcWindow);
	cs.x			= rcWindow.left;
	cs.y			= rcWindow.top;
	cs.style		= dwStyle;
	cs.lpszName		= lpWindowName;
	cs.lpszClass	= lpClassName;
	cs.dwExStyle	= dwExStyle;

	m_hWnd = ::CreateWindowEx( dwExStyle, lpClassName, lpWindowName, dwStyle, rcWindow.left, rcWindow.top, 
		RECT_WIDTH( rcWindow), RECT_HEIGHT( rcWindow), hParent, hMenu, hInstance, &cs);

	if( m_hWnd == NULL)
		return false;

	m_GlobalWindowList.insert( GLOBALWINDOWLIST::value_type( m_hWnd, this));
	m_hInstance = hInstance;

	return true;
}

HWND CWindowBase::CreateControl(HINSTANCE hInstance,LPCTSTR szClassName,LPCTSTR szWindowName,DWORD dwStyle,RECT rcControl,UINT nID,DWORD dwExStyle)
{
	_ASSERTE( m_hWnd != NULL);

	return ::CreateWindowEx( dwExStyle, szClassName, szWindowName, dwStyle, rcControl.left, rcControl.top,
		RECT_WIDTH( rcControl), RECT_HEIGHT( rcControl), m_hWnd, (HMENU)nID, hInstance, NULL);
}

bool CWindowBase::DestroyWindowBase()
{
	::DestroyWindow( m_hWnd);
	
	GLOBALWINDOWLIST::iterator it;

	it = m_GlobalWindowList.find( m_hWnd);

	if( it != m_GlobalWindowList.end())
		m_GlobalWindowList.erase( it);

	return false;
}

LRESULT CWindowBase::WindowProc(UINT iMsg,WPARAM wParam,LPARAM lParam)
{
	switch( iMsg)
	{
		case WM_DESTROY:
			if( !m_bDialog)
				PostQuitMessage(0);
			break;
		case WM_COMMAND:
			return ProcessUserCommand( iMsg, wParam, lParam);
	}

	return ProcessUserHandle( iMsg, wParam, lParam);
}

void CWindowBase::CenterWindow()
{
	int x;
	int y;

	RECT rcRect = GetWindowRect();
	RECT rcParent = GetParentWindowRect();

	x = ( RECT_WIDTH( rcParent) - RECT_WIDTH( rcRect)) / 2;
	y = ( RECT_HEIGHT( rcParent) - RECT_HEIGHT( rcRect)) / 2;

	::SetWindowPos( m_hWnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

void CWindowBase::ShowWindow(int nCmdShow)
{
	_ASSERTE( IsWindow( m_hWnd));
	::ShowWindow( m_hWnd, nCmdShow);
}

void CWindowBase::UpdateWindow()
{
	_ASSERTE( IsWindow( m_hWnd));
	::UpdateWindow( m_hWnd);
}

RECT CWindowBase::GetWindowRect()
{
	RECT rcWindow;
	::GetWindowRect( m_hWnd, &rcWindow);
	
	return rcWindow;
}

RECT CWindowBase::GetClientRect()
{
	RECT rcClient;
	::GetClientRect( m_hWnd, &rcClient);
	
	return rcClient;
}

RECT CWindowBase::GetParentWindowRect()
{
	HWND hParentWnd = ::GetParent( m_hWnd);

	RECT rcWindow;
	
	if( hParentWnd == NULL)
	{
		rcWindow.left = 0;
		rcWindow.top = 0;
		rcWindow.right = GetSystemMetrics( SM_CXFULLSCREEN);
		rcWindow.bottom = GetSystemMetrics( SM_CYFULLSCREEN);
	}
	else
	{
		::GetWindowRect( hParentWnd, &rcWindow);
	}

	return rcWindow;
}

void CWindowBase::RedrawWindow(RECT* rcUpdate,UINT flags)
{
	::RedrawWindow( m_hWnd, rcUpdate, NULL, flags);
}

void CWindowBase::RegHandler(UINT message,WIN32_MESSAGE_HANDLER handle)
{
	_ASSERTE( m_MessageHandleMap.find( message) == m_MessageHandleMap.end());

	m_MessageHandleMap.insert( WIN32_MESSAGE_HANDLEMAP::value_type( message, handle));
}

LRESULT CWindowBase::ProcessUserHandle(UINT message,WPARAM wParam,LPARAM lParam)
{
	WIN32_MESSAGE_HANDLEMAP::iterator it;

	it = m_MessageHandleMap.find( message);

	if( it != m_MessageHandleMap.end())
	{
		WIN32_MESSAGE_HANDLER handle = (*it).second;

		return (*this.*handle)( wParam, lParam);
	}

	return !m_bDialog ? DefWindowProc( m_hWnd, message, wParam, lParam): 0;
}

void CWindowBase::RegCommand(WORD controlID,WORD controlMessage, WIN32_COMMAND_HANDLER handle)
{
	WPARAM wKey = MAKEWPARAM( controlID, controlMessage);

	_ASSERTE( m_CommandHandleMap.find( wKey) == m_CommandHandleMap.end());

	m_CommandHandleMap.insert( WIN32_COMMAND_HANDLEMAP::value_type( wKey, handle));
}

void CWindowBase::RegHandlerProcess(UINT message,WIN32_MESSAGE_HANDLER handler)
{
	_ASSERTE( m_ProcessMessageHandleMap.find( message) == m_ProcessMessageHandleMap.end());

	sProcessMessageHandle handle;
	handle.m_pWindow = this;
	handle.m_pHandler = handler;
	m_ProcessMessageHandleMap.insert( WIN32_PROCESS_MESSAGE_HANDLEMAP::value_type( message, handle));
}

LRESULT CWindowBase::ProcessUserCommand(UINT message,WPARAM wParam,LPARAM lParam)
{
	WIN32_COMMAND_HANDLEMAP::iterator it;

	it = m_CommandHandleMap.find( wParam);

	if( it != m_CommandHandleMap.end())
	{
		WIN32_COMMAND_HANDLER handle = (*it).second;

		return (*this.*handle)( wParam, lParam);
	}

	return 0;
}

void CWindowBase::SetupChildAnchor(HWND hChildWnd,BYTE btMask)
{
	RECT rcWindow = GetWindowRect();
	RECT rcWindowClient = GetClientRect();

	::AdjustWindowRect( &rcWindowClient, GetWindowLong( m_hWnd, GWL_STYLE), FALSE);

	int offset_x,offset_y;
	offset_x = -rcWindowClient.left;
	offset_y = -rcWindowClient.top;

	rcWindow.left += offset_x;
	rcWindow.top += offset_y;
	rcWindow.right = rcWindow.left + RECT_WIDTH( rcWindowClient);
	rcWindow.bottom = rcWindow.top + RECT_HEIGHT( rcWindowClient);
	RECT rcChildWindow;
	::GetWindowRect( hChildWnd, &rcChildWindow);

	sChildWindowAnchor anchor;
	anchor.m_hWnd = hChildWnd;
	anchor.m_btMask = btMask;
			
	anchor.m_AnchorPosition[0] = static_cast<float>(rcChildWindow.left - rcWindow.left);
	anchor.m_AnchorPosition[1] = static_cast<float>(rcWindow.right - rcChildWindow.right);
	anchor.m_AnchorPosition[2] = static_cast<float>(rcChildWindow.top - rcWindow.top);
	anchor.m_AnchorPosition[3] = static_cast<float>(rcWindow.bottom - rcChildWindow.bottom);

	if( btMask & ANCHOR_EDGE_TYPE_MASK_LEFT_RATIO)
		anchor.m_AnchorPosition[ 0] /= (float)RECT_WIDTH( rcWindow);
	
	if( btMask & ANCHOR_EDGE_TYPE_MASK_RIGHT_RATIO)
		anchor.m_AnchorPosition[ 1] /= (float)RECT_WIDTH( rcWindow);
	
	if( btMask & ANCHOR_EDGE_TYPE_MASK_TOP_RATIO)
		anchor.m_AnchorPosition[ 2] /= (float)RECT_HEIGHT( rcWindow);
	
	if( btMask & ANCHOR_EDGE_TYPE_MASK_BOTTOM_RATIO)
		anchor.m_AnchorPosition[ 3] /= (float)RECT_HEIGHT( rcWindow);
	
	m_ChildAnchorList.push_back( anchor);
}

void CWindowBase::UpdateChildAnchor()
{
	RECT rcWindow = GetWindowRect();
	RECT rcWindowClient = GetClientRect();

	::AdjustWindowRect( &rcWindowClient, GetWindowLong( m_hWnd, GWL_STYLE), FALSE);

	int offset_x,offset_y;
	offset_x = -rcWindowClient.left;
	offset_y = -rcWindowClient.top;

	rcWindow.left += offset_x;
	rcWindow.top += offset_y;
	rcWindow.right = rcWindow.left + RECT_WIDTH( rcWindowClient);
	rcWindow.bottom = rcWindow.top + RECT_HEIGHT( rcWindowClient);

	CHILDANCHOR_LIST::iterator it;
	for(it = m_ChildAnchorList.begin(); it != m_ChildAnchorList.end(); ++it)
	{
		sChildWindowAnchor& anchor = (*it);

		RECT rcCurrentRect;
		RECT rcNewRect;

		::GetWindowRect( anchor.m_hWnd, &rcCurrentRect);

		if( anchor.m_btMask & ANCHOR_EDGE_MASK_LEFT)
		{
			if( anchor.m_btMask & ANCHOR_EDGE_TYPE_MASK_LEFT_RATIO)
				rcNewRect.left = anchor.GetEdgeOffset( rcWindow, 0);
			else
				rcNewRect.left = static_cast<LONG>(anchor.m_AnchorPosition[0]);
		}
		else
		{
			rcNewRect.left = rcCurrentRect.left;
		}

		if( anchor.m_btMask & ANCHOR_EDGE_MASK_RIGHT)
		{
			if( anchor.m_btMask & ANCHOR_EDGE_TYPE_MASK_RIGHT_RATIO)
				rcNewRect.right = RECT_WIDTH( rcWindow) - anchor.GetEdgeOffset( rcWindow, 1);
			else
				rcNewRect.right = static_cast<LONG>(RECT_WIDTH( rcWindow) - anchor.m_AnchorPosition[1]);

			if( (anchor.m_btMask & ANCHOR_EDGE_MASK_LEFT) == 0)
				rcNewRect.left = rcNewRect.right - RECT_WIDTH( rcCurrentRect);
		}
		else
		{
			rcNewRect.right = rcNewRect.left + RECT_WIDTH( rcCurrentRect);
		}
			
		if( anchor.m_btMask & ANCHOR_EDGE_MASK_TOP)
		{
			if( anchor.m_btMask & ANCHOR_EDGE_TYPE_MASK_TOP_RATIO)
				rcNewRect.top = anchor.GetEdgeOffset( rcWindow, 2);
			else
				rcNewRect.top = static_cast<LONG>(anchor.m_AnchorPosition[2]);
		}
		else
		{
			rcNewRect.top = rcCurrentRect.top;
		}

		if( anchor.m_btMask & ANCHOR_EDGE_MASK_BOTTOM)
		{
			if( anchor.m_btMask & ANCHOR_EDGE_TYPE_MASK_BOTTOM_RATIO)
				rcNewRect.bottom = RECT_HEIGHT( rcWindow) - anchor.GetEdgeOffset( rcWindow, 3);
			else
				rcNewRect.bottom = static_cast<LONG>(RECT_HEIGHT( rcWindow) - anchor.m_AnchorPosition[3]);

			if( (anchor.m_btMask & ANCHOR_EDGE_MASK_TOP) == 0)
				rcNewRect.top = rcNewRect.bottom - RECT_HEIGHT( rcCurrentRect);
		}
		else
		{
			rcNewRect.bottom = rcNewRect.top + RECT_HEIGHT( rcCurrentRect);
		}
		
		::MoveWindow( anchor.m_hWnd, rcNewRect.left, rcNewRect.top, RECT_WIDTH( rcNewRect), RECT_HEIGHT( rcNewRect), TRUE);
	}
}

END_NAMESPACE