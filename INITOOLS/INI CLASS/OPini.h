// OPini.h: interface for the COPini class.
//
////////////////////////////////////////////////////////////////////// 

// ***************************************************************
//  OPini   version:  1.0   ? date: 07/19/2007
//  -------------------------------------------------------------
//  这个类用来读取exe下ini文件的内容。
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// *************************************************************** 

#if !defined(AFX_OPINI_H__CE3F8B7B_1ACA_46CC_A91C_F8E23FA9B063__INCLUDED_)
#define AFX_OPINI_H__CE3F8B7B_1ACA_46CC_A91C_F8E23FA9B063__INCLUDED_ 

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000 

#include <afxwin.h>
class COPini  
{
public:
    static DWORD ReadString (char *section, char * key,  char stringtoread[],  char * filename);
    static BOOL WriteString(LPCTSTR section, LPCTSTR key,char* stringtoadd, char *filename);
    COPini();
    virtual ~COPini(); 

}; 

#endif // !defined(AFX_OPINI_H__CE3F8B7B_1ACA_46CC_A91C_F8E23FA9B063__INCLUDED_) 