//{{AFX_INCLUDES()
#include "msflexgrid.h"
//}}AFX_INCLUDES
#if !defined(AFX_GCDLG_H__A1C2FE43_ECD5_456A_8E65_E2E25D825110__INCLUDED_)
#define AFX_GCDLG_H__A1C2FE43_ECD5_456A_8E65_E2E25D825110__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GCDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGCDlg dialog

class CGCDlg : public CDialog
{
// Construction
public:
	CGCDlg(CWnd* pParent = NULL);   // standard constructor
	CString m_features[45];
// Dialog Data
	//{{AFX_DATA(CGCDlg)
	enum { IDD = IDD_GC };
	CMSFlexGrid	m_gcgrid;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGCDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGCDlg)
	afx_msg void OnRefresh();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GCDLG_H__A1C2FE43_ECD5_456A_8E65_E2E25D825110__INCLUDED_)
