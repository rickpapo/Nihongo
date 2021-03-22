
// Honyaku_No_HojoDlg.h : header file
//

#pragma once

#include "DlgResizeHelper.h"

// CHonyaku_No_HojoDlg dialog
class CHonyaku_No_HojoDlg : public CDialog
{
// Construction
public:
	CHonyaku_No_HojoDlg(CWnd* pParent = NULL);	// standard constructor
	bool fSized;
	POINT InitialSize;
	DlgResizeHelper m_DlgResizeHelper;

// Dialog Data
	enum { IDD = IDD_HONYAKU_NO_HOJO_DIALOG };
	CStatic m_StaticPrompt ;
	CEdit   m_EditInputText ;
	CButton m_ButtonTranslate ;
	CButton m_CheckboxJuman ;
	CEdit   m_EditResultText ;

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO *lpMMI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedTranslate();
	CString InputText;
	CString ResultText;
};
