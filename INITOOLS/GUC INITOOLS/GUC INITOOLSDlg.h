// GUC INITOOLSDlg.h : header file
//

#if !defined(AFX_GUCINITOOLSDLG_H__4B21629B_1BA7_4AF3_994C_CF786146D794__INCLUDED_)
#define AFX_GUCINITOOLSDLG_H__4B21629B_1BA7_4AF3_994C_CF786146D794__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "DlgData.h"
#include "DlgOpt.h"
#include "DlgCmd.h"
#include "DlgInfo.h"
#include "NVDlg.h"
#include "GCDlg.h"
#include "afxwin.h"
/////////////////////////////////////////////////////////////////////////////
// CGUCINITOOLSDlg dialog

class CGUCINITOOLSDlg : public CDialog
{
// Construction
public:
	CGUCINITOOLSDlg(CWnd* pParent = NULL);	// standard constructor
	CDlgOpt DlogConf;
	CDlgData DlogDataView;
	CDlgCmd	DlogCmd;
	CDlgInfo DlogInfo;
	NVDlg DlgNV;
	CGCDlg DlgGC;
	int ReadPhysicalDriveInNTWithAdminRights (void);
	BOOL RegFlexgrid();

// Dialog Data
	//{{AFX_DATA(CGUCINITOOLSDlg)
	enum { IDD = IDD_GUCINITOOLS_DIALOG };
	CComboBox	m_combo2;
	CTabCtrl	m_tab;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGUCINITOOLSDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CGUCINITOOLSDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeCombo1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GUCINITOOLSDLG_H__4B21629B_1BA7_4AF3_994C_CF786146D794__INCLUDED_)
