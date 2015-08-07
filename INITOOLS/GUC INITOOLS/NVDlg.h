//{{AFX_INCLUDES()
#include "msflexgrid.h"
//}}AFX_INCLUDES
#if !defined(AFX_NVDLG_H__7B4B173E_2D38_4645_A31D_B0EB1C42209D__INCLUDED_)
#define AFX_NVDLG_H__7B4B173E_2D38_4645_A31D_B0EB1C42209D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NVDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// NVDlg dialog

class NVDlg : public CDialog
{
// Construction
public:
	NVDlg(CWnd* pParent = NULL);   // standard constructor
	CString m_features[27];
	void ReadNV();
// Dialog Data
	//{{AFX_DATA(NVDlg)
	enum { IDD = IDD_NV };
	CMSFlexGrid	m_nvgrid;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(NVDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(NVDlg)
	afx_msg void OnStart();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NVDLG_H__7B4B173E_2D38_4645_A31D_B0EB1C42209D__INCLUDED_)
