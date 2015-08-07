// NVDlg.cpp : implementation file
//

#include "stdafx.h"
#include "guc initools.h"
#include "NVDlg.h"
#include "SW80API.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// NVDlg dialog


NVDlg::NVDlg(CWnd* pParent /*=NULL*/)
	: CDialog(NVDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(NVDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void NVDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(NVDlg)
	DDX_Control(pDX, IDC_MSFLEXGRID1, m_nvgrid);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(NVDlg, CDialog)
	//{{AFX_MSG_MAP(NVDlg)
	ON_BN_CLICKED(IDC_BUTTON1, OnStart)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// NVDlg message handlers

void NVDlg::OnStart() 
{
	ReadNV(); 
}

void NVDlg::ReadNV() 
{
	BYTE buf[512];
	int i;
	DWORD tmp;
	CString str,str1;
	ULONGLONG vendorlba;

	vendorlba=SW80GetVendorLBA(GlobalDevice);
	if(SW80SSDReadDiskinfo(GlobalDevice,vendorlba,buf))
	{
		for(i=0;i<8;i++)
		{
			tmp=(buf[0x180+4*i+3]<<24)|(buf[0x180+4*i+2]<<16)|(buf[0x180+4*i+1]<<8)|(buf[0x180+4*i]);
			str.Format("0x%X",tmp);
			m_nvgrid.SetRow(i+1);
			m_nvgrid.SetCol(2);
			m_nvgrid.SetText(str);
			m_nvgrid.SetCellAlignment(1);
		}
		for(i=8;i<12;i++)
		{
			tmp=(buf[0x1A0+2*(i-8)+1]<<8)|(buf[0x1A0+2*(i-8)]);
			str.Format("0x%X",tmp);
			m_nvgrid.SetRow(i+1);
			m_nvgrid.SetCol(2);
			m_nvgrid.SetText(str);
			m_nvgrid.SetCellAlignment(1);
		}
		
		for(i=0;i<4;i++)
		{
			tmp=(buf[0x1E8+4*i+3]<<24)|(buf[0x1E8+4*i+2]<<16)|(buf[0x1E8+4*i+1]<<8)|(buf[0x1E8+4*i]);
			str.Format("0x%X",tmp);
			m_nvgrid.SetRow(13+i);
			m_nvgrid.SetCol(2);
			m_nvgrid.SetText(str);
			m_nvgrid.SetCellAlignment(1);
		}

		for(i=0;i<16;i++)
		{
	
			tmp=(buf[0x1A8+4*i+3]<<24)|(buf[0x1A8+4*i+2]<<16)|(buf[0x1A8+4*i+1]<<8)|(buf[0x1A8+4*i]);
			str.Format("0x%X ",tmp);
			str1+=str;
			if(i%4==3)
			{
				m_nvgrid.SetRow(17+i/4);
				m_nvgrid.SetCol(2);
				m_nvgrid.SetText(str1);
				m_nvgrid.SetCellAlignment(1);
				str1="";
			}
		}
	}

}

BOOL NVDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_features[0]="上电次数";
	m_features[1]="上电时间(单位秒)";
	m_features[2]="读命令个数(低32位)";
	m_features[3]="读命令个数(高32位)";
	m_features[4]="写命令个数(低32位)";
	m_features[5]="写命令个数(高32位)";
	m_features[6]="总擦除次数(低32位)";
	m_features[7]="总擦除次数(高32位)";
	m_features[8]="总坏块数";
	m_features[9]="擦除失败的坏块数";
	m_features[10]="写失败坏块数";
	m_features[11]="读失效坏块数";

	m_features[12]="读命令sector计数(低32位)";
	m_features[13]="读命令sector计数(高32位)";
	m_features[14]="写命令sector计数(低32位)";
	m_features[15]="写命令sector计数(高32位)";
	m_features[16]="Read error计数器";

	m_nvgrid.SetRow(0);
	m_nvgrid.SetCol(0);
	m_nvgrid.SetText("SN");
	m_nvgrid.SetCellAlignment(1);
	m_nvgrid.SetColWidth(0,500);

	m_nvgrid.SetRow(0);
	m_nvgrid.SetCol(1);
	m_nvgrid.SetText("Features");
	m_nvgrid.SetCellAlignment(1);
	m_nvgrid.SetColWidth(1,4500);

	m_nvgrid.SetRow(0);
	m_nvgrid.SetCol(2);
	m_nvgrid.SetText("Value");
	m_nvgrid.SetCellAlignment(1);
	m_nvgrid.SetColWidth(2,6400);	

	int i;
	CString str;
	for(i=0;i<17;i++)
	{
		m_nvgrid.SetRow(i+1);
		m_nvgrid.SetRowHeight(i+1,357);
	}
	for(i=0;i<17;i++)
	{
		m_nvgrid.SetRow(i+1);
		m_nvgrid.SetCol(0);
		str.Format("%d",i);
		m_nvgrid.SetText(str);
		m_nvgrid.SetCellAlignment(1);
		m_nvgrid.SetCol(1);
		m_nvgrid.SetText(m_features[i]);
		m_nvgrid.SetCellAlignment(1);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
