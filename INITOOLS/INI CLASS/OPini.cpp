// OPini.cpp: implementation of the COPini class.
//
////////////////////////////////////////////////////////////////////// 
#include "stdafx.h"
#include "OPini.h" 

/********************************************************************
    created:    2007/07/19
    created:    19:7:2007   10:13
    filename:     OPini.cpp
    file path:    
    file base:    OPini
    file ext:    cpp
    author:        alantop
    purpose:    ��ȡINI�ļ���
*********************************************************************/ 

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
////////////////////////////////////////////////////////////////////// 

COPini::COPini()
{
} 

COPini::~COPini()
{
}
/*****************************************************************************
Function:       // 
Description:    // д�ַ�����INI�ļ�
Calls:          // 
Called By:      // 
Table Accessed: // 
Table Updated:  // 
Input:          // 
Output:         // 
Return:         // �ɹ������棬ʧ�ܷ��ؼ�.ʧ�ܺ󣬿���DWORD GetLastError(VOID)
                   ��ѯʧ��ԭ��
Others:         // 
author:         // alantop
date:           // 2007.07.19
******************************************************************************/
/*
void error(LPSTR lpszFunction) 
{ 
    CHAR szBuf[80]; 
    DWORD dw = GetLastError(); 
    sprintf(szBuf, "%s failed: GetLastError returned %u\n", 
        lpszFunction, dw); 
    MessageBox(NULL, szBuf, "Error", MB_OK); 
    ExitProcess(dw); 
} 

*/
BOOL COPini::WriteString(LPCTSTR section, LPCTSTR key, char *stringtoadd, char *filename)
{
	CHAR FilePath[255]; 
    strcpy(FilePath,filename);
    return ::WritePrivateProfileString(section,key,stringtoadd,FilePath);
} 

/*****************************************************************************
Function:       // 
Description:    // ��INI�ļ��ж�ȡ�ַ���
Calls:          // 
Called By:      // 
Table Accessed: // 
Table Updated:  // 
Input:          // 
Output:         // 
Return:         // ��ȡ�˶��ٸ��ֽڵ��ַ�
Others:         // 
author:         // alantop
date:           // 2007.07.19
******************************************************************************/
DWORD COPini::ReadString(char *section, char * key,  char stringtoread[],  char * filename)
{
    CHAR FilePath[255]; 

	strcpy(FilePath,filename);
    return ::GetPrivateProfileString(section, key,NULL,stringtoread,255,FilePath);
} 
