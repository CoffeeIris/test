#if !defined(AFX_DLGINFO_H__067764D5_929C_4B3F_8262_1CAE1FA8919D__INCLUDED_)
#define AFX_DLGINFO_H__067764D5_929C_4B3F_8262_1CAE1FA8919D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgInfo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgInfo dialog

class CDlgInfo : public CDialog
{
// Construction
public:
	CDlgInfo(CWnd* pParent = NULL);   // standard constructor
	void UpdateIDFY(int device); 
// Dialog Data
	//{{AFX_DATA(CDlgInfo)
	enum { IDD = IDD_DIALOG_INFO };
	CButton	m_devicereset;
	CButton	m_cfaset;
	CButton	m_nop;
	CButton	m_writebuf;
	CButton	m_readbuf;
	CButton	m_addressset;
	CButton	m_apowerset;
	CButton	m_securityset;
	CButton	m_removeset;
	CButton	m_powerset;
	CButton	m_packetset;
	CButton	m_smartset;
	CEdit	m_dieinfo;
	CEdit	m_channleinfo;
	CEdit	m_planeinfo;
	CEdit	m_flashtype;
	CEdit	m_flashvender;
	CEdit	m_dmasinfo;
	CEdit	m_dmainfo;
	CEdit	m_capinfo;
	CEdit	m_sninfo;
	CEdit	m_fwinfo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgInfo)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedStatic11();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGINFO_H__067764D5_929C_4B3F_8262_1CAE1FA8919D__INCLUDED_)
