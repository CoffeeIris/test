// GUC INITOOLSDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GUC INITOOLS.h"
#include "GUC INITOOLSDlg.h"
#include "SW80API.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CGUCINITOOLSDlg *pgucdlg=NULL;
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGUCINITOOLSDlg dialog

CGUCINITOOLSDlg::CGUCINITOOLSDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGUCINITOOLSDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGUCINITOOLSDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGUCINITOOLSDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGUCINITOOLSDlg)
	DDX_Control(pDX, IDC_COMBO1, m_combo2);
	DDX_Control(pDX, IDC_TAB1, m_tab);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGUCINITOOLSDlg, CDialog)
	//{{AFX_MSG_MAP(CGUCINITOOLSDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeCombo1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGUCINITOOLSDlg message handlers

BOOL CGUCINITOOLSDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	RegFlexgrid();
	CString file; 
	file=SLWCONFINI;
	ReadConfigureIni(file);

	DlogConf.Create(IDD_DIALOG_OPT,&m_tab);
	DlogDataView.Create(IDD_DATAVIEW,&m_tab);
	DlogInfo.Create(IDD_DIALOG_INFO,&m_tab);
	DlogCmd.Create(IDD_DIALOG_CMD,&m_tab);
	DlgNV.Create(IDD_NV,&m_tab);
	DlgGC.Create(IDD_GC,&m_tab);

	m_tab.InsertItem(0, _T("Option Setting"));
	m_tab.InsertItem(1, _T("PAGE DATA"));
	m_tab.InsertItem(2, _T("SSD Info"));
	m_tab.InsertItem(3, _T("Operating Parameter"));
	m_tab.InsertItem(4, _T("Rebuild Parameter"));
	m_tab.InsertItem(5, _T("SW80 Command"));

	CRect rc;
	m_tab.GetClientRect(rc);
	rc.top += 30;
	rc.bottom -= 8;
	rc.left += 8;
	rc.right -= 8;

    DlogConf.MoveWindow(&rc);
    DlogDataView.MoveWindow(&rc);
	DlogInfo.MoveWindow(&rc);
    DlogCmd.MoveWindow(&rc);
	DlgNV.MoveWindow(&rc);
    DlgGC.MoveWindow(&rc);

	DlogCmd.ShowWindow(SW_SHOW);	
	m_tab.SetCurSel(5);
	DlogCmd.m_selmode.SetCurSel(0);
	memset(LocalPath,0,256);
	GetModuleFileName(NULL,LocalPath,256); 
    //Scan a string for the last occurrence of a character.
    (strrchr(LocalPath,'\\'))[1] = 0; 

	ReadPhysicalDriveInNTWithAdminRights();

	//把读写错误标志位置0,这个标志用于升级FW LOADER这些敏感信息，如果标志位为1，说明可能有读写错误，不能升级
	int i;
	//for(i=0;i<DeviceCount;i++)
	//	RWflag[i]=0;

	CString File;
	File = GLOBALINFOFILE;
	SW80ReadGlobalinfoblockTxt(File);

	ULONGLONG vendorlba;
	vendorlba=SW80GetVendorLBA(GlobalDevice);
	if(vendorlba==0xF800000)
	{
		if(ReadFlashID(0)!=0)
			SW80SetNFC(0);
		else
			return 0;
	}
	pgucdlg=this;
	m_combo2.SetCurSel(0);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGUCINITOOLSDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGUCINITOOLSDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGUCINITOOLSDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

int CGUCINITOOLSDlg::ReadPhysicalDriveInNTWithAdminRights (void)
{
	int done = FALSE;
	int drive = 0;
	INT16U  baseAddress = 0;   //  Base address of drive controller
	BYTE IdOutCmd [sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];
	char HardDriveSerialNumber [1024];
	int id=0;

	int cnt=0;
	cnt=m_combo2.GetCount();
	m_combo2.ResetContent();
	HANDLE handle;
	BYTE idfy[512];

	if(FindGUCDisk()==0)
	{
		//AddInfo("Can not find SSD device!!!");
		return 0;
	}

	for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
	{
	
		handle= Devicehandle[drive];	

		if (handle  != INVALID_HANDLE_VALUE)
		{
			GETVERSIONOUTPARAMS VersionParams;
			DWORD               cbBytesReturned = 0;
			
            // Get the version, etc of PhysicalDrive IOCTL
			memset ((void*) &VersionParams, 0, sizeof(VersionParams));
			
			if ( ! DeviceIoControl (handle , DFP_GET_VERSION,
				NULL, 
				0,
				&VersionParams,
				sizeof(VersionParams),
				&cbBytesReturned, NULL) )
			{         
			}
			
            // If there is a IDE device at number "i" issue commands
            // to the device
			//if (VersionParams.bIDEDeviceMap > 0)
			if(1)
			{
				BYTE             bIDCmd = 0;   // IDE or ATAPI IDENTIFY cmd
				SENDCMDINPARAMS  scip;

				// Now, get the ID sector for all IDE devices in the system.
				// If the device is ATAPI use the IDE_ATAPI_IDENTIFY command,
				// otherwise use the IDE_ATA_IDENTIFY command
				bIDCmd = (VersionParams.bIDEDeviceMap >> drive & 0x10) ? \
IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;
				
				memset (&scip, 0, sizeof(scip));
				memset (IdOutCmd, 0, sizeof(IdOutCmd));
				
				if(IDFY_IDE(drive,idfy))
				{
					USHORT diskdata [256];
					int ijk = 0;
					USHORT *pIdSector = (USHORT *)
						((PSENDCMDOUTPARAMS) IdOutCmd) -> bBuffer;
					
					for (ijk = 0; ijk < 256; ijk++)
					//	diskdata [ijk] = pIdSector [ijk];
					{
						diskdata [ijk] = idfy [2*ijk] | (idfy[2*ijk+1]<<8);
					}
					strcpy (HardDriveSerialNumber, ConvertToString (diskdata, 10, 19));

					CString str,strSn,strInfo;
					strInfo.Format("");
					strSn.Format("%s",HardDriveSerialNumber);
					strSn.TrimLeft();
					str.Format("Drive Model: %s -> SN:%s  ",ConvertToString (diskdata, 27, 46),strSn);
					strInfo+=str;
					strInfo+=GetDriveGeometry(handle);
					ExDriverInfo *pInfo=new ExDriverInfo;
					pInfo->BaseAddr=baseAddress;
					pInfo->DeviceID=(drive % 2==0)?0:0x10;
					pInfo->DriverNum=drive;

					m_combo2.InsertString(id,strInfo);
					m_combo2.SetItemData(id,(DWORD)pInfo);
					id++;

					strInfo="";
					str.Format("DriverNum:%d",drive);							
					done = TRUE;
				}
			}			
		}
	}
	
	return done;
}


void CGUCINITOOLSDlg::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	switch(m_tab.GetCurSel())
	{
	case 0:
	 
		DlogConf.ShowWindow(SW_SHOW);
		DlogDataView.ShowWindow(SW_HIDE);
		DlogCmd.ShowWindow(SW_HIDE);
		DlogInfo.ShowWindow(SW_HIDE);
		DlgNV.ShowWindow(SW_HIDE);
		DlgGC.ShowWindow(SW_HIDE);
		break;
		
	case 1:
	
		DlogConf.ShowWindow(SW_HIDE);
		DlogDataView.ShowWindow(SW_SHOW);
		DlogCmd.ShowWindow(SW_HIDE);
		DlogInfo.ShowWindow(SW_HIDE);
		DlgNV.ShowWindow(SW_HIDE);
		DlgGC.ShowWindow(SW_HIDE);
		break;
	
	case 2:
	
		DlogConf.ShowWindow(SW_HIDE);
		DlogDataView.ShowWindow(SW_HIDE);
		DlogInfo.ShowWindow(SW_SHOW);
		DlgNV.ShowWindow(SW_HIDE);
		DlgGC.ShowWindow(SW_HIDE);
		DlogCmd.ShowWindow(SW_HIDE);
		break;
	
	case 3:
	
		DlogConf.ShowWindow(SW_HIDE);
		DlogDataView.ShowWindow(SW_HIDE);
		DlogCmd.ShowWindow(SW_HIDE);
		DlogInfo.ShowWindow(SW_HIDE);
		DlgNV.ShowWindow(SW_SHOW);
		DlgGC.ShowWindow(SW_HIDE);
		break;
	
	case 4:
	
		DlogConf.ShowWindow(SW_HIDE);
		DlogDataView.ShowWindow(SW_HIDE);
		DlogInfo.ShowWindow(SW_HIDE);
		DlogCmd.ShowWindow(SW_HIDE);
		DlgNV.ShowWindow(SW_HIDE);
		DlgGC.ShowWindow(SW_SHOW);
		break;

	case 5:
	
		DlogConf.ShowWindow(SW_HIDE);
		DlogDataView.ShowWindow(SW_HIDE);
		DlogInfo.ShowWindow(SW_HIDE);
		DlgNV.ShowWindow(SW_HIDE);
		DlgGC.ShowWindow(SW_HIDE);
		DlogCmd.ShowWindow(SW_SHOW);
		break;

	default:
		break;	
	}	
	
	*pResult = 0;
}

void CGUCINITOOLSDlg::OnSelchangeCombo1() 
{
	GlobalDevice = m_combo2.GetCurSel();	
	DlogInfo.UpdateIDFY(GlobalDevice);
}

BOOL CGUCINITOOLSDlg::RegFlexgrid()
{
	LPCTSTR   dllName   =   "MSFLXGRD.OCX";   
	HINSTANCE   hLib   =   LoadLibrary(dllName);   
	if(hLib   <   (HINSTANCE)HINSTANCE_ERROR)   
	{   
		printf("不能装载MSFLXGRD.ocx文件\n");   
		return 0;
	}   
	//获取注册函数DllRegisterServer地址   
	FARPROC   DllRegisterServer   =   GetProcAddress(hLib,_T("DllRegisterServer"));   
	if(DllRegisterServer   !=   NULL)   
	{   
		HRESULT   regResult   =   DllRegisterServer();   
		::FreeLibrary(hLib);   
		if(regResult   ==   NOERROR)   
			return 1;   
		else   
			return 0;  
	}
	return 0;
}
