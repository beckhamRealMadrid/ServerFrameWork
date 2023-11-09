#pragma once

#include "WindowBase.h"

START_NAMESPACE

class RENET_API CWin32Dialog : public CWindowBase
{
public:
							CWin32Dialog();
							CWin32Dialog(HINSTANCE hInstance,UINT TemplateID,CWindowBase *pWndParent = NULL);
	virtual					~CWin32Dialog();
	virtual LRESULT			OnInitDialog(WPARAM wParam,LPARAM lParam);
	virtual LRESULT			OnOk(WPARAM wParam,LPARAM lParam);
	virtual LRESULT			OnCancel(WPARAM wParam,LPARAM lParam);
	virtual LRESULT			OnTimer(WPARAM wParam,LPARAM lParam);
	virtual HWND			GetDlgItem(UINT id);
	virtual void			EnableDlgItem(UINT id,bool bEnable = true);
	virtual bool			SetDlgItemInt(UINT id,int value,bool bSigned = true);
	virtual bool			SetDlgItemFloat(UINT id,float value);
	virtual int				GetDlgItemInt(UINT id,bool bSigned = true);
	virtual bool			SetDlgItemText(UINT id,LPCTSTR str);
	virtual std::string		GetDlgItemTextA(UINT id);
	virtual std::wstring	GetDlgItemTextW(UINT id);
	virtual void			SetCheck(UINT id,bool bCheck);
	virtual bool			GetCheck(UINT id);
	virtual int				Combo_AddString(UINT id,LPCTSTR str);
	virtual void			Combo_DeleteAll(UINT id);
	virtual int				Combo_GetCurSel(UINT id);
	virtual void			EndDialog(int exit_code);	
			bool			Create(HINSTANCE hInstance,UINT TemplateID,CWindowBase *pWndParent = NULL);
			bool			Create(HINSTANCE hInstance,UINT TemplateID,HWND hParentWnd);
			int				DoModal();			
protected:
			UINT			m_nTemplateID;
			int				m_nDoModalResult;
			HWND			m_hParent;
};

END_NAMESPACE