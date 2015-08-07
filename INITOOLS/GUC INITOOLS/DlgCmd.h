#if !defined(AFX_DLGCMD_H__75F0801C_7C1D_4112_89A8_38258860F6D0__INCLUDED_)
#define AFX_DLGCMD_H__75F0801C_7C1D_4112_89A8_38258860F6D0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgCmd.h : header file
//
#include "TextProgressCtrl.h"
/////////////////////////////////////////////////////////////////////////////
// CDlgCmd dialog
#include "Resource.h"
class CDlgCmd : public CDialog
{
// Construction
public:
	CDlgCmd(CWnd* pParent = NULL);   // standard constructor
	static UINT ThreadSSDTask(LPVOID pParam);
	int InitDisk(void);
// Dialog Data
	//{{AFX_DATA(CDlgCmd)
	enum { IDD = IDD_DIALOG_CMD };
	CComboBox	m_selmode;
	CEdit	m_editresult;
	int		m_channel;
	int		m_die;
	int		m_block;
	CTextProgressCtrl m_progress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgCmd)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgCmd)
	afx_msg void OnEnterVND();
	afx_msg void OnDownSRAM();
	afx_msg void OnRresh();
	afx_msg void OnEraseblock();
	afx_msg void OnOutVND();
	afx_msg void OnUpdateloader();
	afx_msg void OnInitial();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSelchangeCombo1();
	afx_msg void OnUpdateFW();
	afx_msg void OnReadFW();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGCMD_H__75F0801C_7C1D_4112_89A8_38258860F6D0__INCLUDED_)
