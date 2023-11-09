#include "stdafx.h"
#include "Win32Dialog.h"

START_NAMESPACE

CWin32Dialog::CWin32Dialog()
{
	m_hInstance			= NULL;
	m_nTemplateID		= 0;
	m_bDialog			= true;
	m_nDoModalResult	= 0;
	m_hParent			= NULL;
}

CWin32Dialog::CWin32Dialog(HINSTANCE hInstance,UINT TemplateID,CWindowBase *pWndParent)
{
	m_hInstance			= hInstance;
	m_nTemplateID		= TemplateID;
	m_pWndParent		= pWndParent;
	m_hParent			= NULL;
	m_bDialog			= true;
	m_nDoModalResult	= 0;
}

CWin32Dialog::~CWin32Dialog()
{

}

// The CreateDialogParam function creates a modeless dialog box from a dialog box template resource
bool CWin32Dialog::Create(HINSTANCE hInstance,UINT TemplateID,CWindowBase *pWndParent)
{
	m_hInstance = hInstance;
	
	if( pWndParent == NULL)
		m_hWnd = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(TemplateID), ::GetDesktopWindow(), (DLGPROC)GlobalWindowProc, (LPARAM)this);
	else
		m_hWnd = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(TemplateID), pWndParent->GetHWnd(), (DLGPROC)GlobalWindowProc, (LPARAM)this);

	if( m_hWnd == NULL)
		return false;

	return true;
}

// The CreateDialogParam function creates a modeless dialog box from a dialog box template resource
bool CWin32Dialog::Create(HINSTANCE hInstance,UINT TemplateID,HWND hParentWnd)
{
	m_hInstance = hInstance;

	if( hParentWnd == NULL)
		m_hWnd = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(TemplateID), ::GetDesktopWindow(), (DLGPROC)GlobalWindowProc, (LPARAM)this);
	else
		m_hWnd = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(TemplateID), hParentWnd, (DLGPROC)GlobalWindowProc, (LPARAM)this);

	if( m_hWnd == NULL)
		return false;

	return true;
}

// The DialogBoxParam function creates a modal dialog box from a dialog box template resource
int CWin32Dialog::DoModal()
{
	_ASSERTE(m_hInstance);
	_ASSERTE(m_nTemplateID);

	RegHandler( WM_INITDIALOG, (WIN32_MESSAGE_HANDLER)&CWin32Dialog::OnInitDialog);
	RegHandler( WM_TIMER, (WIN32_MESSAGE_HANDLER)&CWin32Dialog::OnTimer);
	RegCommand( IDOK, BN_CLICKED, (WIN32_COMMAND_HANDLER)&CWin32Dialog::OnOk);
	RegCommand( IDCANCEL, BN_CLICKED, (WIN32_COMMAND_HANDLER)&CWin32Dialog::OnCancel);
	m_nDoModalResult = DialogBoxParam( m_hInstance, MAKEINTRESOURCE(m_nTemplateID), m_hParent, (DLGPROC)GlobalWindowProc, (LPARAM)this);

	return m_nDoModalResult;
}

LRESULT CWin32Dialog::OnTimer(WPARAM wParam,LPARAM lParam)
{
	return 0;
}

LRESULT CWin32Dialog::OnInitDialog(WPARAM wParam,LPARAM lParam)
{
	CenterWindow();
	return 0;
}

LRESULT CWin32Dialog::OnOk(WPARAM wParam,LPARAM lParam)
{
	EndDialog(IDOK);
	return 0;
}

LRESULT CWin32Dialog::OnCancel(WPARAM wParam,LPARAM lParam)
{
	EndDialog(IDCANCEL);
	return 0;
}

void CWin32Dialog::EndDialog(int exit_code)
{
	m_nDoModalResult = exit_code;
	::EndDialog( m_hWnd, exit_code);
}

bool CWin32Dialog::SetDlgItemInt(UINT id,int value,bool bSigned)
{
	_ASSERTE( IsWindow( m_hWnd));	
	return ::SetDlgItemInt( m_hWnd, id, value, bSigned) ? true : false;
}

bool CWin32Dialog::SetDlgItemFloat(UINT id,float value)
{
	TCHAR buff[ MAX_PATH];
	wsprintf( buff, _T("%.2f"), value );
	SetDlgItemText( id, buff);
	return true;
}

int  CWin32Dialog::GetDlgItemInt(UINT id,bool bSigned)
{
	_ASSERTE( IsWindow( m_hWnd));
	return ::GetDlgItemInt( m_hWnd, id, NULL, bSigned);
}

bool CWin32Dialog::SetDlgItemText(UINT id,LPCTSTR str)
{
	_ASSERTE( IsWindow( m_hWnd));
	return ::SetDlgItemText( m_hWnd, id, str) ? true : false;
}

std::string CWin32Dialog::GetDlgItemTextA(UINT id)
{
	_ASSERTE( IsWindow( m_hWnd));

	char buff[255];
	::GetDlgItemTextA( m_hWnd, id, buff, 255 );
	std::string r(buff);

	return r;
}

std::wstring CWin32Dialog::GetDlgItemTextW(UINT id)
{
	_ASSERTE( IsWindow( m_hWnd));

	wchar_t buff[255];
	::GetDlgItemTextW( m_hWnd, id, buff, 255 );
	std::wstring r(buff);

	return r;
}

HWND CWin32Dialog::GetDlgItem(UINT id)
{
	_ASSERTE( IsWindow( m_hWnd));
	return ::GetDlgItem( m_hWnd, id);
}

void CWin32Dialog::SetCheck(UINT id,bool bCheck)
{
	_ASSERTE( IsWindow( m_hWnd));	
	::SendMessage( GetDlgItem( id), BM_SETCHECK, bCheck, 0);
}

bool CWin32Dialog::GetCheck(UINT id)
{
	_ASSERTE( IsWindow( m_hWnd));	
	return ::SendMessage( GetDlgItem( id), BM_GETCHECK, 0, 0) ? true : false;
}

int CWin32Dialog::Combo_AddString(UINT id,LPCTSTR str)
{
	_ASSERTE( IsWindow( m_hWnd));
	return (int)::SendMessage( GetDlgItem( id), CB_ADDSTRING, 0, (LPARAM)str);
}

void CWin32Dialog::Combo_DeleteAll(UINT id)
{
	_ASSERTE( IsWindow( m_hWnd));
	::SendMessage( GetDlgItem( id), CB_RESETCONTENT, 0, 0);
}

void CWin32Dialog::EnableDlgItem(UINT id,bool bEnable)
{
	_ASSERTE( IsWindow( m_hWnd));
	::EnableWindow( GetDlgItem( id), bEnable);
}

int CWin32Dialog::Combo_GetCurSel(UINT id)
{
	_ASSERTE( IsWindow( m_hWnd));
	return (int)::SendMessage( GetDlgItem( id), CB_GETCURSEL, 0, 0);
}

END_NAMESPACE