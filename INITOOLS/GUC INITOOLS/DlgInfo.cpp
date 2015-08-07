// DlgInfo.cpp : implementation file
//

#include "stdafx.h"
#include "GUC INITOOLS.h"
#include "DlgInfo.h"
#include "SW80API.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgInfo dialog


CDlgInfo::CDlgInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgInfo)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgInfo)
	DDX_Control(pDX, IDC_CHECK12, m_devicereset);
	DDX_Control(pDX, IDC_CHECK11, m_cfaset);
	DDX_Control(pDX, IDC_CHECK10, m_nop);
	DDX_Control(pDX, IDC_CHECK9, m_writebuf);
	DDX_Control(pDX, IDC_CHECK8, m_readbuf);
	DDX_Control(pDX, IDC_CHECK7, m_addressset);
	DDX_Control(pDX, IDC_CHECK6, m_apowerset);
	DDX_Control(pDX, IDC_CHECK5, m_securityset);
	DDX_Control(pDX, IDC_CHECK4, m_removeset);
	DDX_Control(pDX, IDC_CHECK3, m_powerset);
	DDX_Control(pDX, IDC_CHECK2, m_packetset);
	DDX_Control(pDX, IDC_CHECK1, m_smartset);
	DDX_Control(pDX, IDC_EDITCHANNEL2, m_dieinfo);
	DDX_Control(pDX, IDC_EDITCHANNEL, m_channleinfo);
	DDX_Control(pDX, IDC_EDITPLANE, m_planeinfo);
	DDX_Control(pDX, IDC_EDITCHANNEL4, m_flashtype);
	DDX_Control(pDX, IDC_EDITCHANNEL3, m_flashvender);
	DDX_Control(pDX, IDC_EDITCHANNEL6, m_dmasinfo);
	DDX_Control(pDX, IDC_EDITCHANNEL5, m_dmainfo);
	DDX_Control(pDX, IDC_EDITCHANNEL9, m_capinfo);
	DDX_Control(pDX, IDC_EDITCHANNEL8, m_sninfo);
	DDX_Control(pDX, IDC_EDITCHANNEL7, m_fwinfo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgInfo, CDialog)
	//{{AFX_MSG_MAP(CDlgInfo)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_STATIC11, &CDlgInfo::OnBnClickedStatic11)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgInfo message handlers

BOOL CDlgInfo::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgInfo::UpdateIDFY(int device) 
{
	int i=0,temp1;
	WORD data[256];
	CString str;
	unsigned char dataBuffer[512];
	DWORD total;

	//---deal idfy 
	char idfy[512];
	char c;

	BYTE buf[512];
	DWORD tmp;
	CString str1;
	ULONGLONG vendorlba;

	vendorlba=SW80GetVendorLBA(GlobalDevice);
	if(vendorlba==0xF800000)
	{
		m_flashvender.SetWindowText(manufacture);
		if(flash_type[device].isMLC)
			m_flashtype.SetWindowText("MLC");
		else
			m_flashtype.SetWindowText("SLC");
		str.Format("%d",channel);
		m_channleinfo.SetWindowText(str);
		str.Format("%d",flash_type[device].plane_per_die);		
		m_planeinfo.SetWindowText(str);
		str.Format("%d",grdie);
		m_dieinfo.SetWindowText(str);
	}

	if(IDFY_IDE(GlobalDevice,dataBuffer)==0)
		AfxMessageBox("Identify failed !");

	for (i=0;i<256;i++)
	{
		idfy[i*2]=dataBuffer[i*2] ;
		idfy[i*2+1]=dataBuffer[i*2+1];
	}

	for (i=0;i<256;i++)
		data[i]=dataBuffer[i*2+1]<<8 | dataBuffer[i*2] ;	

	// S/N: &idfy[20]
	idfy[40] = idfy[41] = 0;
	for (i=20;i<40;i=i+2)
	{
		c = idfy[i+1];
		idfy[i+1] = idfy[i];
		idfy[i]=c;
	}
	m_sninfo.SetWindowText(&idfy[20]);
	
	// FW Rev: &idfy[46]
	idfy[54] = idfy[55] = 0;
	for (i=46;i<54;i=i+2)
	{
		c = idfy[i+1];
		idfy[i+1] = idfy[i];
		idfy[i]=c;
	}
	m_fwinfo.SetWindowText(&idfy[46]);
	
	total=dataBuffer[200]|(dataBuffer[201]<<8)|(dataBuffer[202]<<16)|dataBuffer[203]<<24;
	str.Format("0x%X",total);
	m_capinfo.SetWindowText(str);

	temp1=(dataBuffer[2*(IdfyData[62].word1)]);			
	if (IdfyData[62].word1==88 && IdfyData[62].offset1==8)//UDMA Selected
	{
		if (temp1 & 0x01)
		{
			str.Format("0");
		}
		if (temp1 & 0x02)
		{
			str.Format("1");
		}
		if (temp1 & 0x4)
		{
			str.Format("2");
		}
		if (temp1 & 0x8)
		{
			str.Format("3");
		}
		if (temp1 & 0x10)
		{
			str.Format("4");
		}
		if (temp1 & 0x20)
		{
			str.Format("5");
		}
		if (temp1 & 0x40)
		{
			str.Format("6");
		}
		m_dmainfo.SetWindowText(str);
	}
	temp1=(dataBuffer[2*IdfyData[63].word1+1]);
	if (IdfyData[63].word1==88 && IdfyData[63].offset1==0)//UDMA Support
	{
		if (temp1 & 0x01)
		{
			str.Format("0");
		}
		if (temp1 & 0x02)
		{
			str.Format("1 and below");
		}
		if (temp1 & 0x4)
		{
			str.Format("2 and below");
		}
		if (temp1 & 0x8)
		{
			str.Format("3 and below");
		}
		if (temp1 & 0x10)
		{
			str.Format("4 and below");
		}
		if (temp1 & 0x20)
		{
			str.Format("5 and below");
		}
		if (temp1 & 0x40)
		{
			str.Format("6 and below");
		}
		m_dmasinfo.SetWindowText(str);
	}
		

	temp1=(data[IdfyData[17].word1]&IdfyData[17].Mask1)>>IdfyData[17].offset1;//support
	m_nop.SetCheck(temp1);
	temp1=(data[IdfyData[18].word1]&IdfyData[18].Mask1)>>IdfyData[18].offset1;//support
	m_readbuf.SetCheck(temp1);
	temp1=(data[IdfyData[19].word1]&IdfyData[19].Mask1)>>IdfyData[19].offset1;//support
	m_writebuf.SetCheck(temp1);
	temp1=(data[IdfyData[21].word1]&IdfyData[21].Mask1)>>IdfyData[21].offset1;//support
	m_devicereset.SetCheck(temp1);
	temp1=(data[IdfyData[26].word1]&IdfyData[26].Mask1)>>IdfyData[26].offset1;//support
	m_packetset.SetCheck(temp1);
	temp1=(data[IdfyData[27].word1]&IdfyData[27].Mask1)>>IdfyData[27].offset1;//support
	m_powerset.SetCheck(temp1);
	temp1=(data[IdfyData[28].word1]&IdfyData[28].Mask1)>>IdfyData[28].offset1;//support
	m_removeset.SetCheck(temp1);
	temp1=(data[IdfyData[29].word1]&IdfyData[29].Mask1)>>IdfyData[29].offset1;//support
	m_securityset.SetCheck(temp1);
	temp1=(data[IdfyData[30].word1]&IdfyData[30].Mask1)>>IdfyData[30].offset1;//support
	m_smartset.SetCheck(temp1);
	temp1=(data[IdfyData[34].word1]&IdfyData[34].Mask1)>>IdfyData[34].offset1;//support
	m_addressset.SetCheck(temp1);
	temp1=(data[IdfyData[40].word1]&IdfyData[40].Mask1)>>IdfyData[40].offset1;//support
	m_apowerset.SetCheck(temp1);
	temp1=(data[IdfyData[41].word1]&IdfyData[41].Mask1)>>IdfyData[41].offset1;//support
	m_cfaset.SetCheck(temp1);	
}

void CDlgInfo::OnBnClickedStatic11()
{
	// TODO: 在此添加控件通知处理程序代码
}
