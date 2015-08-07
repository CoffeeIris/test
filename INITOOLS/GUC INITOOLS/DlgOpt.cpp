// DlgOpt.cpp : implementation file
//

#include "stdafx.h"
#include "GUC INITOOLS.h"
#include "DlgOpt.h"
#include "SW80API.h"
#include "OPini.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDlgOpt *pdlgopt=NULL;
/////////////////////////////////////////////////////////////////////////////
// CDlgOpt dialog


CDlgOpt::CDlgOpt(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgOpt::IDD, pParent)
	
	, m_retrycnt(0)
{
	//{{AFX_DATA_INIT(CDlgOpt)
	m_incphyblock = 0;
	m_respage = 0;
	m_resratio = 0;
	m_sn = _T("");
	m_model = _T("");
	m_ssdcap = 0;
	m_maxpu = 0;
	m_eccthreshold = 0;
	m_functlword = 0;
	m_purgetime=0;
	//}}AFX_DATA_INIT
}


void CDlgOpt::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgOpt)
	DDX_Control(pDX, IDC_EDITCN2, m_multibootsize);
	DDX_Control(pDX, IDC_CHECK2, m_access);
	DDX_Control(pDX, IDC_COMBO3, m_interface);
	DDX_Control(pDX, IDC_CHECK5, m_twoplane);
	DDX_Control(pDX, IDC_CHECK1, m_largemode);
	DDX_Control(pDX, IDC_COMBO2, m_channelnum);
	DDX_Text(pDX, IDC_EDITCN3, m_incphyblock);
	DDX_Text(pDX, IDC_EDITSTRIPE, m_respage);
	DDX_Text(pDX, IDC_EDITFLASH, m_resratio);
	DDX_Text(pDX, IDC_EDITSN, m_sn);
	DDX_Text(pDX, IDC_EDITMODEL, m_model);
	DDX_Text(pDX, IDC_EDITSSD, m_ssdcap);
	DDX_Text(pDX, IDC_EDITCN4, m_maxpu);
	DDX_Text(pDX, IDC_EDITCN5, m_eccthreshold);
	DDX_Text(pDX, IDC_EDITCN, m_functlword);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_CHECK3, m_writeprt);
	DDX_Control(pDX, IDC_CHECK14, m_diskrestore);
	DDX_Control(pDX, IDC_EDITMODEL2, m_mirrorsize);
	DDX_Control(pDX, IDC_CHECK13, m_trim);
	DDX_Control(pDX, IDC_CHECK6, m_purge);
	DDX_Control(pDX, IDC_CHECK15, m_datatrap);
	DDX_Control(pDX, IDC_CHECK4, m_opcapacity);
	DDX_Control(pDX, IDC_EDITSSD, m_ssdcontrol);
	DDX_Control(pDX, IDC_CHECK16, m_biosbind);
	DDX_Text(pDX, IDC_EDITCN6, m_purgetime);
	DDX_Text(pDX, IDC_EDITCN7, m_retrycnt);
	DDX_Control(pDX, IDC_CHECK17, m_c2purge);
	DDX_Control(pDX, IDC_CHECK18, m_restartflag);
}


BEGIN_MESSAGE_MAP(CDlgOpt, CDialog)
	//{{AFX_MSG_MAP(CDlgOpt)
	ON_BN_CLICKED(IDC_BUTTON1, OnSave)
	ON_BN_CLICKED(IDC_BUTTON5, OnSaveas)
	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_BUTTON6, OnReadFromSSD)
	ON_BN_CLICKED(IDC_BUTTON7, OnSaveToSSD)
	ON_BN_CLICKED(IDC_BUTTON3, OnCancel)
	ON_BN_CLICKED(IDC_CHECK2, OnEnableMultipartition)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_CHECK4, &CDlgOpt::OnBnClickedCheck4)
	ON_STN_CLICKED(IDC_STATIC7, &CDlgOpt::OnStnClickedStatic7)
	ON_STN_DBLCLK(IDC_STATIC7, OnStnDblclickStatic7)
	ON_EN_CHANGE(IDC_EDITSSD, &CDlgOpt::OnEnChangeEditssd)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgOpt message handlers

void CDlgOpt::OnSave() 
{
	CString file;
	file=SLWCONFINI;
	SaveConfigure(file);	
}

void CDlgOpt::OnCancel() 
{
	CString file;
	file=SLWCONFINI;
	ReadConfigure(file);
}

BOOL CDlgOpt::ReadConfigure(CString FilePath)
{
	char str[128];
	char filename[128];
	COPini ini;
	int chnum,type;
    GetModuleFileName(NULL,filename,128); 
    //Scan a string for the last occurrence of a character.
    (strrchr(filename,'\\'))[1] = 0; 
    strcat(filename,FilePath);
	int lcheck;

	ini.ReadString("Device Setting","Model Number",str,filename);
	m_model=str;

	ini.ReadString("Device Setting","Serial Number",str,filename);
	m_sn=str;

	ini.ReadString("Device Setting","Channel Number",str,filename);
	chnum=atol(str);
	if(chnum==1)
		m_channelnum.SetCurSel(0);
	else if(chnum==2)
		m_channelnum.SetCurSel(1);
	else if(chnum==3)
		m_channelnum.SetCurSel(2);
	else
		m_channelnum.SetCurSel(3);

	
	ini.ReadString("Device Setting","Interface Type",str,filename);
	type=atol(str);
	m_interface.SetCurSel(type);

	ini.ReadString("FW Setting","SSD Capability",str,filename);
	m_ssdcap=atol(str);

	ini.ReadString("FW Setting","Reserve Ratio",str,filename);
	m_resratio=atol(str);

	ini.ReadString("FW Setting","Reserve Page Per 64 Pages",str,filename);
	m_respage=atol(str);

	ini.ReadString("FW Setting","Purge Time",str,filename);
	m_purgetime=atol(str);

	ini.ReadString("FW Setting","Function Control Word",str,filename);
	m_functlword=atol(str);

	ini.ReadString("FW Setting","USB Retry Counter",str,filename);
	m_retrycnt=atol(str);

	ini.ReadString("FW Setting","Restore SSD Restart Flag",str,filename);
	m_restartflag.SetCheck(atol(str));

	unsigned int a,b,c,d,e,f,h,g;
	a=m_functlword & 0x0001;
	b=m_functlword & 0x0002;
	c=m_functlword & 0x0004;
	d=m_functlword & 0x0008;
	e=m_functlword & 0x0010;
	f=m_functlword & 0x0020;
	h=m_functlword & 0x0040;
	g=m_functlword & 0x0080;
	m_trim.SetCheck(a);
	m_purge.SetCheck(b);
	m_access.SetCheck(c);
	m_diskrestore.SetCheck(d);
	m_largemode.SetCheck(e);
	m_datatrap.SetCheck(f);
	m_biosbind.SetCheck(h);
	m_c2purge.SetCheck(g);
	ini.ReadString("FW Setting","Logic Block Include Phy Block",str,filename);
	m_incphyblock=atol(str);

	ini.ReadString("FW Setting","Max PU Num",str,filename);
	m_maxpu=atol(str);

	ini.ReadString("FW Setting","Acess Control Flag",str,filename);
	lcheck=atol(str);
	m_access.SetCheck(lcheck);

	ini.ReadString("FW Setting","Enable Two Plane",str,filename);
	lcheck=atol(str);
	m_twoplane.SetCheck(lcheck);

	ini.ReadString("FW Setting","ECC Alarm Threshold",str,filename);
	m_eccthreshold=atol(str);
	 
/*
	ini.ReadString("FW Setting","WWN_Status",str,filename);
	m_wwnconf=atol(str);
	ini.ReadString("FW Setting","SMART_Status",str,filename);
	m_smartconf=atol(str);

	ini.ReadString("Option","SSD INITIONALIZE",str,filename);
	checkinit=atoi(str);
	ini.ReadString("Option","UPDATE FW",str,filename);
	checkupdatefw=atoi(str);
	ini.ReadString("Option","UPDATE FPGA",str,filename);
	checkupdatefpga=atoi(str);
*/


	UpdateData(FALSE);
	return 1;
}

void CDlgOpt::OnSaveas() 
{
	
}

BOOL CDlgOpt::SaveConfigure(CString FilePath)
{	
	char str[128];
	char filename[128];
	COPini ini;
	CString lstr;
	int chnum,type;
	int lcheck;

	UpdateData(TRUE);
    GetModuleFileName(NULL,filename,128); 
    //Scan a string for the last occurrence of a character.
    (strrchr(filename,'\\'))[1] = 0; 
    strcat(filename,FilePath);
	
	m_model.TrimRight();
	if(m_model.GetLength()>40)
	{
		AfxMessageBox("Model Number 的最大长度是40个字节,当前值超出范围，请重新输入!");
		return 0;
	}

	sprintf(str,"%s",m_model.GetBuffer(0));
	ini.WriteString("Device Setting","Model Number",str,filename);
	
	m_sn.TrimRight();
	if(m_sn.GetLength()>20)
	{
		AfxMessageBox("Serial Number的最大长度是20个字节,当前值超出范围，请重新输入!");
		return 0;
	}
	sprintf(str,"%s",m_sn.GetBuffer(0));
	ini.WriteString("Device Setting","Serial Number",str,filename);
	
	memset(str,0,128);
	chnum=m_channelnum.GetCurSel();
	if(chnum==0)
		str[0]='1';
	else if(chnum==1)
		str[0]='2';
	else if(chnum==2)
		str[0]='3';
	else 
		str[0]='4';

	ini.WriteString("Device Setting","Channel Number",str,filename);

	type=m_interface.GetCurSel();
	sprintf(str,"%d",type);
	ini.WriteString("Device Setting","Interface Type",str,filename);

	sprintf(str,"%d",m_ssdcap);	
	ini.WriteString("FW Setting","SSD Capability",str,filename);

	lcheck=m_twoplane.GetCheck();
	sprintf(str,"%d",lcheck);
	ini.WriteString("FW Setting","Enable Two Plane",str,filename);

	lcheck=m_access.GetCheck();
	sprintf(str,"%d",lcheck);
	ini.WriteString("FW Setting","Acess Control Flag",str,filename);

	sprintf(str,"%d",m_purgetime);	
	ini.WriteString("FW Setting","Purge Time",str,filename);

	sprintf(str,"%d",m_retrycnt);	
	ini.WriteString("FW Setting","USB Retry Counter",str,filename);

	sprintf(str,"%d",m_restartflag.GetCheck());	
	ini.WriteString("FW Setting","Restore SSD Restart Flag",str,filename);

	/*
	lcheck=m_hardwarereset.GetCheck();
	sprintf(str,"%d",lcheck);
	ini.WriteString("FW Setting","Enable Hardware Reset",str,filename);

	lcheck=m_securityset.GetCheck();
	sprintf(str,"%d",lcheck);
	ini.WriteString("FW Setting","Enable Security Feature Set",str,filename);

	sprintf(str,"%u",m_mutiboot);
	ini.WriteString("FW Setting","Enable Multi Boot Feature",str,filename);
*/
	sprintf(str,"%d",m_resratio);
	ini.WriteString("FW Setting","Reserve Ratio",str,filename);

	sprintf(str,"%d",m_respage);
	ini.WriteString("FW Setting","Reserve Page Per 64 Pages",str,filename);

	unsigned int a,b,c,d,e,f,h,g;
	a=m_trim.GetCheck();
	b=m_purge.GetCheck();
	c=m_access.GetCheck();
	d=m_diskrestore.GetCheck();
	e=m_largemode.GetCheck();
	f=m_datatrap.GetCheck();
	h=m_biosbind.GetCheck();
	g=m_c2purge.GetCheck();
	m_functlword=a | b<<1 | c<<2 | d<<3 | e<<4 | f<<5 | h<<6 | g<<7;
	sprintf(str,"%d",m_functlword);
	ini.WriteString("FW Setting","Function Control Word",str,filename);

	sprintf(str,"%d",m_incphyblock);
	ini.WriteString("FW Setting","Logic Block Include Phy Block",str,filename);

	sprintf(str,"%d",m_maxpu);
	ini.WriteString("FW Setting","Max PU Num",str,filename);

	sprintf(str,"%d",m_eccthreshold);
	ini.WriteString("FW Setting","ECC Alarm Threshold",str,filename);

	m_multibootsize.GetWindowText(lstr);
	PartionSize=atol(lstr)*2048;		//将 MB 转换为 LBA
	if(PartionSize<32)
		PartionSize=32*2048;
	if(c>0)
		MirrorSize=PartionSize;
	if(d>0)
	{
		m_mirrorsize.GetWindowText(lstr);
		MirrorSize=atol(lstr)*2048;		//将 MB 转换为 LBA
		if(MirrorSize<PartionSize)
			MirrorSize=PartionSize;
	}

	return 1;
}

int CDlgOpt::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	pdlgopt=this;
	return 0;
}

BOOL CDlgOpt::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CString file; 
	file=SLWCONFINI;
	ReadConfigure(file);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgOpt::OnReadFromSSD() 
{
	unsigned char result;
	ULONGLONG vendorlba;
	PUCHAR buf;
	long ret =0;
	int datasize=1024*512;
	CString str;
	int addr,i,tmp=0;
	vendorlba=SW80GetVendorLBA(GlobalDevice);
	if(SW80ReadCMDStatus(GlobalDevice,vendorlba,&result)>0)
	{
		buf=(PUCHAR) malloc(datasize);
		memset(buf,0,datasize);
		/*读全局信息*/
		if(SW80SSDReadFW(GlobalDevice,vendorlba,buf)==0)
		{
			AddInfo(GlobalDevice,"****Read global info failed!!!",3);	
			free(buf);
			return ;		
		}
		addr=512;
		m_model.Empty();
		for(i=27;i<47;i++)
		{
			m_model+=buf[addr+2*i+1];
			m_model+=buf[addr+2*i];
		}
		m_model.TrimRight();
		m_sn.Empty();
		for(i=10;i<20;i++)
		{
			m_sn+=buf[addr+2*i+1];
			m_sn+=buf[addr+2*i];
		}
		m_sn.TrimRight();

		m_ssdcap=(buf[addr+200]|buf[addr+201]<<8|buf[addr+202]<<16|buf[addr+203]<<24);

		addr=1024;
		
		DWORD Protectsize=0,Mirrorsize=0;
		Protectsize=buf[addr+0x24]+(buf[addr+0x25]<<8)+(buf[addr+0x26]<<16)+(buf[addr+0x27]<<24);
		str.Format("%d",Protectsize/2048);
		m_multibootsize.SetWindowText(str);
		
		Mirrorsize=buf[addr+0x28]+(buf[addr+0x29]<<8)+(buf[addr+0x2A]<<16)+(buf[addr+0x2B]<<24);
		str.Format("%d",Mirrorsize/2048);
		m_mirrorsize.SetWindowText(str);

		m_purgetime=buf[addr+0x2E]+(buf[addr+0x2F]<<8);
		m_purgetime=m_purgetime/1000;
		
		//密码重试次数
		m_retrycnt=buf[addr+0x32]+(buf[addr+0x33]<<8);
		
		//还原盘重启生效标志
		tmp=buf[addr+0x34]+(buf[addr+0x35]<<8);
		m_restartflag.SetCheck(tmp);

		FunCtlWord=(buf[addr+0x8D]<<8)+buf[addr+0x8C];
		unsigned int a,b,c,d,e,f,h,g;
		a=FunCtlWord & 0x0001;
		b=FunCtlWord & 0x0002;
		c=FunCtlWord & 0x0004;
		d=FunCtlWord & 0x0008;
		e=FunCtlWord & 0x0010;
		f=FunCtlWord & 0x0020;
		h=FunCtlWord & 0x0040;
		g=FunCtlWord & 0x0080;
		m_trim.SetCheck(a);
		m_purge.SetCheck(b);
		m_access.SetCheck(c);
		m_diskrestore.SetCheck(d);
		m_largemode.SetCheck(e);
		m_datatrap.SetCheck(f);
		m_biosbind.SetCheck(h);
		m_c2purge.SetCheck(g);
	
		if(buf[addr+0x2C]==0)
			m_writeprt.SetCheck(0);
		else
			m_writeprt.SetCheck(1);
	}
	UpdateData(FALSE);
}

void CDlgOpt::OnSaveToSSD() 
{
	unsigned char result;
	ULONGLONG vendorlba;
	PUCHAR buf;
	long ret =0;
	int datasize=1024*512;
	CString str;
	int addr,i;
	char output[256];
	DWORD oobnum=0,tmpnum=0;
	PUCHAR DATABuffer;
	DATABuffer=(PUCHAR)malloc(datasize);

	UpdateData(TRUE);
	vendorlba=SW80GetVendorLBA(GlobalDevice);
	if(SW80ReadCMDStatus(GlobalDevice,vendorlba,&result)>0)
	{
		buf=(PUCHAR) malloc(datasize);
		memset(buf,0,datasize);
		/*读全局信息*/
		Sleep(100);
		if(SW80SSDReadFW(GlobalDevice,vendorlba,buf)==0)
		{
			AddInfo(GlobalDevice,"****Read global info failed!!!",3);	
			free(buf);
			return ;		
		}
		addr=512;

		m_model.TrimRight();
		if(m_model.GetLength()>40)
		{
			AfxMessageBox("Model Number 的最大长度是40个字节,当前值超出范围，请重新输入!");
			free(buf);
			return ;
		}
		sprintf(output,"%s",m_model);
		for(i=0;i<m_model.GetLength();i++)
		{
			buf[addr+54+i]=output[i];
		}
		for(i=m_model.GetLength();i<40;i++)
		{
			buf[addr+54+i]=0x20;
		}
		for(i=27;i<47;i++)
		{
			output[0]=buf[addr+2*i];
			buf[addr+2*i]=buf[addr+2*i+1];
			buf[addr+2*i+1]=output[0];
		}

		if(m_opcapacity.GetCheck())
		{
			DWORD Total,tmp;
			Total=buf[1024+0x0bc]|buf[1024+0x0bd]<<8|buf[1024+0x0be]<<16|buf[1024+0x0bf]<<24;
		}

		int checksum=0;
		for(i=0;i<511;i++)
		{
			checksum+=buf[addr+i];
		}
		buf[addr+511]=~checksum+1;//Bytes 511 为所有值的CheckSum

		addr=1024;
		if(m_writeprt.GetCheck()==0)
			buf[addr+0x2C]=0;
		else
			buf[addr+0x2C]=1;

		buf[addr+0x2E]=m_purgetime*1000;
		buf[addr+0x2F]=(m_purgetime*1000)>>8;

		//密码重试次数
		buf[addr+0x32]=m_retrycnt;
		buf[addr+0x33]=m_retrycnt>>8;

		//还原盘重启生效标志
		buf[addr+0x34]=m_restartflag.GetCheck();		

		unsigned int a,b,c,d,e,f,h,g;
		a=m_trim.GetCheck();
		b=m_purge.GetCheck();
		c=m_access.GetCheck();
		d=m_diskrestore.GetCheck();
		e=m_largemode.GetCheck();
		f=m_datatrap.GetCheck();
		h=m_biosbind.GetCheck();
		g=m_c2purge.GetCheck();
		m_functlword=a | b<<1 |  d<<3  | f<<5 | h<<6 | g<<7;
		buf[addr+0x8C]=buf[addr+0x8C] & 0x14; //先保留访问控制和大容量标志 
		buf[addr+0x8C]=m_functlword | buf[addr+0x8C];

		if(m_opcapacity.GetCheck())
		{
			buf[addr+0x0d8]=m_ssdcap;
			buf[addr+0x0d9]=m_ssdcap>>8;
			buf[addr+0x0da]=m_ssdcap>>16;
			buf[addr+0x0db]=m_ssdcap>>24;
		}

		DWORD Protectsize=0,Mirrorsize=0;
		m_multibootsize.GetWindowText(str);
		Protectsize=atol(str)*2048;
		buf[addr+0x24]=Protectsize;
		buf[addr+0x25]=Protectsize>>8;
		buf[addr+0x26]=Protectsize>>16;
		buf[addr+0x27]=Protectsize>>24;	
		
		m_mirrorsize.GetWindowText(str);
		Mirrorsize=atol(str)*2048;
		buf[addr+0x28]=Mirrorsize;
		buf[addr+0x29]=Mirrorsize>>8;
		buf[addr+0x2A]=Mirrorsize>>16;
		buf[addr+0x2B]=Mirrorsize>>24;	
		Sleep(100);
		if(SW80SSDWriteFW(GlobalDevice,vendorlba,buf)==0)
		{
			AddInfo(GlobalDevice,"Write GC info block fail!\r\n",3);
			free(buf);
			return ;
		}
		free(buf);
		
		memset(DATABuffer, 0, datasize);
		Sleep(100);
		if(SW80SSDReadLoad(GlobalDevice,vendorlba,DATABuffer)==0)
		{
			AddInfo(GlobalDevice,"Read loader fw failed !",3);
			free(DATABuffer);	
			return ;
		}

		addr=512*63;
		if(m_writeprt.GetCheck()==0)
			DATABuffer[addr+0x2C]=0;
		else
			DATABuffer[addr+0x2C]=1;

		DATABuffer[addr+0x2E]=m_purgetime*1000;
		DATABuffer[addr+0x2F]=(m_purgetime*1000)>>8;

		//密码重试次数
		DATABuffer[addr+0x32]=m_retrycnt;
		DATABuffer[addr+0x33]=m_retrycnt>>8;

		//还原盘重启生效标志
		DATABuffer[addr+0x34]=m_restartflag.GetCheck();

		DATABuffer[addr+0x8C]=DATABuffer[addr+0x8C] & 0x14; //先保留访问控制和大容量标志 
		DATABuffer[addr+0x8C]=m_functlword | DATABuffer[addr+0x8C];

		DATABuffer[addr+0x24]=Protectsize;
		DATABuffer[addr+0x25]=Protectsize>>8;
		DATABuffer[addr+0x26]=Protectsize>>16;
		DATABuffer[addr+0x27]=Protectsize>>24;			

		DATABuffer[addr+0x28]=Mirrorsize;
		DATABuffer[addr+0x29]=Mirrorsize>>8;
		DATABuffer[addr+0x2A]=Mirrorsize>>16;
		DATABuffer[addr+0x2B]=Mirrorsize>>24;	
		if(m_opcapacity.GetCheck())
		{
			DATABuffer[addr+0x0d8]=m_ssdcap;
			DATABuffer[addr+0x0d9]=m_ssdcap>>8;
			DATABuffer[addr+0x0da]=m_ssdcap>>16;
			DATABuffer[addr+0x0db]=m_ssdcap>>24;
		}

		for(i=0;i<8*1024;i++)
		{
			tmpnum=DATABuffer[4*i]+(DATABuffer[4*i+1]<<8)+(DATABuffer[4*i+2]<<16)+(DATABuffer[4*i+3]<<24);
			oobnum=oobnum+tmpnum;
		}
		Sleep(100);
		if(SW80SSDWriteLoad(GlobalDevice,vendorlba,oobnum,DATABuffer)==0)
		{
			AddInfo(GlobalDevice,"Write load fw failed !",3);
			free(DATABuffer);
			return ;
		}

		free(DATABuffer);
	}
}

void CDlgOpt::OnEnableMultipartition() 
{

}

void CDlgOpt::OnBnClickedCheck4()
{
	if(m_opcapacity.GetCheck())
		m_ssdcontrol.SetReadOnly(0);
	else
	{
		m_ssdcontrol.SetReadOnly(1);
	}
}

void CDlgOpt::OnStnClickedStatic7()
{
	// TODO: 在此添加控件通知处理程序代码
}

void CDlgOpt::OnStnDblclickStatic7()
{
	// TODO: 在此添加控件通知处理程序代码
}

void CDlgOpt::OnEnChangeEditssd()
{
	// TODO:  如果该控件是 RICHEDIT 控件，则它将不会
	// 发送该通知，除非重写 CDialog::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}
