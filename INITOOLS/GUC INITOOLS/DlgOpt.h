#include "afxwin.h"
#if !defined(AFX_DLGOPT_H__349AEE00_EE8B_4681_B1C8_30B2AF476BCF__INCLUDED_)
#define AFX_DLGOPT_H__349AEE00_EE8B_4681_B1C8_30B2AF476BCF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgOpt.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgOpt dialog

class CDlgOpt : public CDialog
{
// Construction
public:
	CDlgOpt(CWnd* pParent = NULL);   // standard constructor
	BOOL SaveConfigure(CString FilePath);
	BOOL ReadConfigure(CString FilePath);
// Dialog Data
	//{{AFX_DATA(CDlgOpt)
	enum { IDD = IDD_DIALOG_OPT };
	CEdit	m_multibootsize;
	CButton	m_access;
	CComboBox	m_interface;
	CButton	m_twoplane;
	CButton	m_largemode;
	CComboBox	m_channelnum;
	int		m_incphyblock;
	int		m_respage;
	int		m_resratio;
	int		m_mapphyblock;
	CString	m_sn;
	CString	m_model;
	DWORD	m_ssdcap;
	int		m_maxpu;
	int		m_eccthreshold;
	unsigned int		m_functlword;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgOpt)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgOpt)
	afx_msg void OnSave();
	afx_msg void OnCancel();
	afx_msg void OnSaveas();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL OnInitDialog();
	afx_msg void OnReadFromSSD();
	afx_msg void OnSaveToSSD();
	afx_msg void OnEnableMultipartition();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	CButton m_writeprt;
	CButton m_diskrestore;
	CEdit m_mirrorsize;
	CButton m_trim;
	CButton m_purge;
public:
	CButton m_datatrap;
public:
	afx_msg void OnBnClickedCheck4();
public:
	CButton m_opcapacity;
public:
	CEdit m_ssdcontrol;
public:
	CButton m_biosbind;
public:
	int m_purgetime;
public:
	int m_retrycnt;
public:
	afx_msg void OnStnClickedStatic7();
public:
	afx_msg void OnStnDblclickStatic7();
public:
	CButton m_c2purge;
public:
	afx_msg void OnEnChangeEditssd();
public:
	CButton m_restartflag;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGOPT_H__349AEE00_EE8B_4681_B1C8_30B2AF476BCF__INCLUDED_)
