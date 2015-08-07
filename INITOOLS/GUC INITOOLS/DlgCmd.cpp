// DlgCmd.cpp : implementation file
//

#include "stdafx.h"
#include "GUC INITOOLS.h"
#include "DlgCmd.h"
#include "SW80API.h"
#include "GUC INITOOLSDlg.h"
#include "DlgOpt.h"
#include "OPini.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDlgCmd *pdlgcmd=NULL;
extern CGUCINITOOLSDlg *pgucdlg;
extern CDlgOpt *pdlgopt;
TEST_STRU parm;
/////////////////////////////////////////////////////////////////////////////
// CDlgCmd dialog


CDlgCmd::CDlgCmd(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCmd::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgCmd)
	m_channel = 0;
	m_die = 0;
	m_block = 0;
	//}}AFX_DATA_INIT
}


void CDlgCmd::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgCmd)
	DDX_Control(pDX, IDC_COMBO1, m_selmode);
	DDX_Control(pDX, IDC_EDIT1, m_editresult);
	DDX_Text(pDX, IDC_EDIT2, m_channel);
	DDX_Text(pDX, IDC_EDIT3, m_die);
	DDX_Text(pDX, IDC_EDIT5, m_block);
	DDX_Control(pDX, IDC_PROGRESS1, m_progress);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgCmd, CDialog)
	//{{AFX_MSG_MAP(CDlgCmd)
	ON_BN_CLICKED(IDC_BUTTON1, OnEnterVND)
	ON_BN_CLICKED(IDC_BUTTON8, OnDownSRAM)
	ON_BN_CLICKED(IDC_BUTTON13, OnRresh)
	ON_BN_CLICKED(IDC_BUTTON14, OnEraseblock)
	ON_BN_CLICKED(IDC_BUTTON2, OnOutVND)
	ON_BN_CLICKED(IDC_BUTTON12, OnUpdateloader)
	ON_BN_CLICKED(IDC_BUTTON9, OnInitial)
	ON_WM_CREATE()
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeCombo1)
	ON_BN_CLICKED(IDC_BUTTON7, OnUpdateFW)
	ON_BN_CLICKED(IDC_BUTTON11, OnReadFW)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgCmd message handlers

void CDlgCmd::OnEnterVND() 
{
	EnterVND(GlobalDevice); 
}

void CDlgCmd::OnDownSRAM() 
{
	int i;
	unsigned char buf[512];
	CString str;
	DownSRAM(GlobalDevice,str); 
/*	Sleep(2000);
	for(i=0;i<100;i++)
	{
		Sleep(500);
 		Read_Rebuild(GlobalDevice,buf);
	}
*/
}

UINT CDlgCmd::ThreadSSDTask(LPVOID pParam)
{
	TEST_STRU *pDevice=(TEST_STRU*)pParam;	
	int device=pDevice->device;	
	int ret;
	CDlgCmd   *dlg   =   (CDlgCmd   *)(AfxGetApp()->GetMainWnd());  
	ret=SSDInitialize(device); 
	return 1;
}

void CDlgCmd::OnRresh() 
{
	BYTE flashid[5];
	memset(flashid,0,5);

	int i;
	for(i=0;i<gldie;i++)
		DIEMAP[GlobalDevice][i]=0xFFFF;
	CString str;

	CString file; 
	file=SLWCONFINI;
	ReadConfigureIni(file);
	CloseSW80SSDHandle();
	pgucdlg->ReadPhysicalDriveInNTWithAdminRights();
	
	UpdateData(TRUE);
	if(m_channel>3)
	{
		AfxMessageBox("channel的取值范围是0 ~ 3,当前值超出范围，请重新输入 !");
		return ;
	}
	if(m_die>15)
	{
		AfxMessageBox("die的取值范围是0 ~ 15,当前值超出范围，请重新输入 !");
		return ;
	}
	
	ULONGLONG vendorlba;
	vendorlba=SW80GetVendorLBA(GlobalDevice);
	str.Format("%X",vendorlba);
	AddInfo(GlobalDevice,str,1);
	if(vendorlba==0xF800000)
	{
		ReadFlashID(GlobalDevice);
		if(SW80SetNFC(GlobalDevice)==0)
		{
			AddInfo(GlobalDevice,"****Configure NFC Parameters fail!\r\n",3);
		}
	}
}

void CDlgCmd::OnEraseblock() 
{		
	UpdateData(TRUE);
	if(m_channel>3)
	{
		AfxMessageBox("channel的取值范围是0 ~ 3,当前值超出范围，请重新输入 !");
		return ;
	}
	if(m_die>15)
	{
		AfxMessageBox("die的取值范围是0 ~ 15,当前值超出范围，请重新输入 !");
		return ;
	}
	if(m_block>8191)
	{
		AfxMessageBox("channel的取值范围是0 ~ 8191,当前值超出范围，请重新输入 !");
		return ;
	}

	if(SW80Eraseblock(GlobalDevice,m_channel,m_die,m_block)==0)
		AddInfo(GlobalDevice,"Erase block failed!",3);
	else
		AddInfo(GlobalDevice,"Erase block successfully!",1);
	
	/*
	BYTE blockaddr[16],status[8];				
	memset(blockaddr,0,16);
	memset(status,0,8);
	Setblockaddr(GlobalDevice,m_die,m_block,blockaddr);

	SW80Eraseblock4CH(GlobalDevice,blockaddr,status);
		*/
}

void CDlgCmd::OnOutVND() 
{
	OutVND(GlobalDevice);
}

void CDlgCmd::OnUpdateloader() 
{
	Updateloader(GlobalDevice,0) ;
}


void CDlgCmd::OnInitial() 
{
	int i;
	for(i=0;i<gldie;i++)
		DIEMAP[GlobalDevice][i]=0xFFFF;
	parm.device=GlobalDevice;
	CString file;
	file=SLWCONFINI;
	pdlgopt->SaveConfigure(file);

	AfxBeginThread(ThreadSSDTask,(LPVOID)&parm);
}

int CDlgCmd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	pdlgcmd=this;
	
	return 0;
}

void CDlgCmd::OnSelchangeCombo1() 
{
	IniMode=m_selmode.GetCurSel();
}

void CDlgCmd::OnUpdateFW() 
{
	unsigned char result;
	ULONGLONG vendorlba;
	int ret;
	PIOFlag[GlobalDevice]=0;
	vendorlba=SW80GetVendorLBA(GlobalDevice);
	if(SW80ReadCMDStatus(GlobalDevice,vendorlba,&result)>0)
	{	
		ret=SW80SSDUpdateFW(GlobalDevice,vendorlba);
		if(ret==0)
		{
			AddInfo(GlobalDevice,"Retry update fw !",1);
			PIOFlag[GlobalDevice]=1;
			SW80SSDUpdateFW(GlobalDevice,vendorlba);
		}
	}
	else
	{
		ret=SW80UpdateFWInRamdisk(GlobalDevice);
		if(ret==0)
		{
			AddInfo(GlobalDevice,"Retry update fw !",1);
			PIOFlag[GlobalDevice]=1;
			SW80UpdateFWInRamdisk(GlobalDevice);
		}
	}
}

void CDlgCmd::OnReadFW() 
{
/*		
	ULONGLONG vendorlba;
	vendorlba=SW80GetVendorLBA(GlobalDevice);
	unsigned char buf[512*96*2];
	SW80SSDReadFW(GlobalDevice,vendorlba,buf);

*/

	CString strDlgFile;

	CFileDialog dlgLoad(								
						TRUE, 0, 0,
						OFN_FILEMUSTEXIST| OFN_ALLOWMULTISELECT,
						"Intel Hex|*.*||");

	if(dlgLoad.DoModal() != IDOK)
	{
		return ;		
	}		
	strDlgFile = dlgLoad.GetPathName();

	PUCHAR DATABuffer=NULL;
	DWORD datasize;

	CString str;
	CFile fl;
	int ret;

	if( !fl.Open(strDlgFile, CFile::modeRead, NULL) )
	{
		AfxMessageBox("Open File Failed !");

		return ;		
	}

	datasize=fl.GetLength();
	if(datasize%512!=0)
		datasize=(fl.GetLength()/512+1)*512;

	//datasize=1024*1024*500;
	DATABuffer=(PUCHAR)malloc(datasize);
	if(DATABuffer==NULL)
		return ;
	memset(DATABuffer, 0, datasize);
	if( !fl.Read(DATABuffer, fl.GetLength()) )
	{
		AfxMessageBox("Read loadfw file Failed !");
		free(DATABuffer);
		fl.Close();
		return ;	
	}
	fl.Close();    
	HANDLE hTempFile;
	hTempFile = CreateFile("H:\\SW80FW_V1131SA80.rar",
        GENERIC_READ | GENERIC_WRITE,
        0,  
        NULL,                       
        CREATE_ALWAYS,                
        FILE_ATTRIBUTE_NORMAL,     
        NULL);   

	LARGE_INTEGER li;
	BOOL bRet;
	li.QuadPart = 20000*512;
	SetFilePointer(Devicehandle[GlobalDevice], li.LowPart, &li.HighPart, FILE_BEGIN);
	DWORD dwCB;

	bRet = WriteFile(hTempFile, DATABuffer, datasize, &dwCB, NULL);


}


