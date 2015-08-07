// GUC INITOOLS.h : main header file for the GUC INITOOLS application
//

#if !defined(AFX_GUCINITOOLS_H__B8B91A1F_CF77_471C_85EA_1A79B52F4A4A__INCLUDED_)
#define AFX_GUCINITOOLS_H__B8B91A1F_CF77_471C_85EA_1A79B52F4A4A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CGUCINITOOLSApp:
// See GUC INITOOLS.cpp for the implementation of this class
//

class CGUCINITOOLSApp : public CWinApp
{
public:
	CGUCINITOOLSApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGUCINITOOLSApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CGUCINITOOLSApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GUCINITOOLS_H__B8B91A1F_CF77_471C_85EA_1A79B52F4A4A__INCLUDED_)
