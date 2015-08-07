#if !defined(AFX_DLGDATA_H__F8571A0E_7975_40E8_B421_8F6F0A5B4E5E__INCLUDED_)
#define AFX_DLGDATA_H__F8571A0E_7975_40E8_B421_8F6F0A5B4E5E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgData.h : header file
//
#include "hexedit.h"
/////////////////////////////////////////////////////////////////////////////
// CDlgData dialog

class CDlgData : public CDialog
{
// Construction
public:
	CDlgData(CWnd* pParent = NULL);   // standard constructor
	void ImportFromFile(CString strFile);
	void ExportToFile(CString strFile);
// Dialog Data
	//{{AFX_DATA(CDlgData)
	enum { IDD = IDD_DATAVIEW };
	CEdit	m_oob;
	CHexEdit	m_edhex;
	int		m_channel;
	int		m_die;
	int		m_block;
	int		m_page;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgData)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgData)
	afx_msg void OnIDFY();
	afx_msg void OnPageRead();
	afx_msg void OnPageWrite();
	afx_msg void OnSaveFile();
	afx_msg void OnReadFile();
	afx_msg void OnForward();
	afx_msg void OnBack();
	afx_msg void OnReadwithECC();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGDATA_H__F8571A0E_7975_40E8_B421_8F6F0A5B4E5E__INCLUDED_)
