// GCDlg.cpp : implementation file
//

#include "stdafx.h"
#include "guc initools.h"
#include "GCDlg.h"
#include "SW80API.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGCDlg dialog


CGCDlg::CGCDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGCDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGCDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CGCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGCDlg)
	DDX_Control(pDX, IDC_MSFLEXGRID1, m_gcgrid);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGCDlg, CDialog)
	//{{AFX_MSG_MAP(CGCDlg)
	ON_BN_CLICKED(IDC_BUTTON1, OnRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGCDlg message handlers

void CGCDlg::OnRefresh() 
{
	BYTE buf[512];
	int i;
	DWORD tmp;
	CString str,str1;
	ULONGLONG vendorlba;

	vendorlba=SW80GetVendorLBA(GlobalDevice);
	str.Format("vendor: %X",vendorlba);
	AddInfo(GlobalDevice,str,1);
	if(SW80SSDReadDiskinfo(GlobalDevice,vendorlba,buf))
	{
		for(i=0;i<8;i++)
		{
			tmp=(buf[2*i+1]<<8)|(buf[2*i]);
			if(tmp>=0 && tmp!=0xFFFF)
			{
				str.Format("%d ",tmp);
				str1+=str;
			}
		}
	
		m_gcgrid.SetRow(1);
		m_gcgrid.SetCol(2);
		m_gcgrid.SetText(str1);
		m_gcgrid.SetCellAlignment(1);
		
		for(i=0;i<8;i++)
		{
			tmp=buf[0x020+i];
			str.Format("%X",tmp);
			str1.Empty();
			str1+=str;
		}

		m_gcgrid.SetRow(2);
		m_gcgrid.SetCol(2);
		m_gcgrid.SetText(str1);
		m_gcgrid.SetCellAlignment(1);

		for(i=0;i<6;i++)
		{
			tmp=(buf[0x080+2*i+1]<<8)|(buf[0x080+2*i]);
			str.Format("0x%X",tmp);
			m_gcgrid.SetRow(i+3);
			m_gcgrid.SetCol(2);
			m_gcgrid.SetText(str);
			m_gcgrid.SetCellAlignment(1);
		}

		for(i=0;i<11;i++)
		{
			tmp=(buf[0x090+2*i+1]<<8)|(buf[0x090+2*i]);
			str.Format("0x%X",tmp);
			m_gcgrid.SetRow(i+9);
			m_gcgrid.SetCol(2);
			m_gcgrid.SetText(str);
			m_gcgrid.SetCellAlignment(1);
		}

		for(i=0;i<13;i++)
		{
	
			tmp=(buf[0x0b0+4*i+3]<<24)|(buf[0x0b0+4*i+2]<<16)|(buf[0x0b0+4*i+1]<<8)|(buf[0x0b0+4*i]);
			str.Format("0x%X ",tmp);
			m_gcgrid.SetRow(20+i);
			m_gcgrid.SetCol(2);
			m_gcgrid.SetText(str);
			m_gcgrid.SetCellAlignment(1);
		}

		
		tmp=(buf[0x153]<<8)|(buf[0x152]);
		str.Format("0x%X ",tmp);
		m_gcgrid.SetRow(33);
		m_gcgrid.SetCol(2);
		m_gcgrid.SetText(str);
		m_gcgrid.SetCellAlignment(1);

		for(i=0;i<3;i++)
		{
	
			tmp=(buf[0x154+4*i+3]<<24)|(buf[0x154+4*i+2]<<16)|(buf[0x154+4*i+1]<<8)|(buf[0x154+4*i]);
			str.Format("0x%X ",tmp);
			m_gcgrid.SetRow(34+i);
			m_gcgrid.SetCol(2);
			m_gcgrid.SetText(str);
			m_gcgrid.SetCellAlignment(1);
		}
		
		tmp=(buf[0x050+3]<<24)|(buf[0x050+2]<<16)|(buf[0x050+1]<<8)|(buf[0x050]);
		str.Format("0x%X ",tmp);
		m_gcgrid.SetRow(37);
		m_gcgrid.SetCol(2);
		m_gcgrid.SetText(str);
		m_gcgrid.SetCellAlignment(1);

		tmp=(buf[0x074]<<24)|(buf[0x074+1]<<16)|(buf[0x074+2]<<8)|(buf[0x074+3]);
		str.Format("0x%X ",tmp);
		m_gcgrid.SetRow(38);
		m_gcgrid.SetCol(2);
		m_gcgrid.SetText(str);
		m_gcgrid.SetCellAlignment(1);

		tmp=(buf[0x8D]<<8)|(buf[0x8C]);
		str.Format("0x%X ",tmp);
		m_gcgrid.SetRow(39);
		m_gcgrid.SetCol(2);
		m_gcgrid.SetText(str);
		m_gcgrid.SetCellAlignment(1);

	}	
}

BOOL CGCDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_features[0]="Logic Die到Physical Die的映射";
	m_features[1]="磁盘DNA号";
	m_features[2]="使用MLC标志";
	m_features[3]="使用two plane标志";
	m_features[4]="使用SATA接口标志";
	m_features[5]="ECC 报警设置";
	m_features[6]="Flash vender";
	m_features[7]="原始坏块数";
	m_features[8]="每个block含有的page数偏移";
	m_features[9]="page含有的sector的个数偏移";
	m_features[10]="每个DIE下block的个数偏移";
	m_features[11]="每个PU下DIE的个数偏移";

	m_features[12]="每个CE下DIE个数偏移";
	m_features[13]="每个Channel下CE个数偏移";
	m_features[14]="Channel 数偏移";
	m_features[15]="保留比例（以x/128为单位）";
	m_features[16]="每个logic block保留的page数";

	m_features[17]="每个logic block映射到的物理块的个数";
	m_features[18]="每个logic block下总共可以挂的物理块的个数";
	m_features[19]="总物理块数";
	m_features[20]="总保留的物理块数";
	m_features[21]="总逻辑块数";

	m_features[22]="给用户的最大LBA";
	m_features[23]="给用户的最大SLB超级逻辑块号";
	m_features[24]="PMT表的page数";
	m_features[25]="保留给表的SLB个数";
	m_features[26]="保留给表的LBA大小";
	m_features[27]="总LBA";
	m_features[28]="总PMT表页数";
	m_features[29]="Max DISK LBA";
	m_features[30]="最大PE数(低32位)";
	m_features[31]="最大PE数(高32位)";
	m_features[32]="Rebuild找到的table block的个数";
	m_features[33]="Rebuild找到的坏块的个数";
	m_features[34]="Rebuild找到的空块的个数";
	m_features[35]="Rebuild找到的data块的个数";
	
	m_features[36]="标志";
	m_features[37]="Flash ID";
	m_features[38]="功能控制字";


	m_gcgrid.SetRow(0);
	m_gcgrid.SetCol(0);
	m_gcgrid.SetText("SN");
	m_gcgrid.SetCellAlignment(1);
	m_gcgrid.SetColWidth(0,500);

	m_gcgrid.SetRow(0);
	m_gcgrid.SetCol(1);
	m_gcgrid.SetText("Features");
	m_gcgrid.SetCellAlignment(1);
	m_gcgrid.SetColWidth(1,4500);

	m_gcgrid.SetRow(0);
	m_gcgrid.SetCol(2);
	m_gcgrid.SetText("Value");
	m_gcgrid.SetCellAlignment(1);
	m_gcgrid.SetColWidth(2,6400);	

	int i;
	CString str;
	for(i=0;i<39;i++)
	{
		m_gcgrid.SetRow(i+1);
		m_gcgrid.SetRowHeight(i+1,357);
	}
	for(i=0;i<39;i++)
	{
		m_gcgrid.SetRow(i+1);
		m_gcgrid.SetCol(0);
		str.Format("%d",i);
		m_gcgrid.SetText(str);
		m_gcgrid.SetCellAlignment(1);
		m_gcgrid.SetCol(1);
		m_gcgrid.SetText(m_features[i]);
		m_gcgrid.SetCellAlignment(1);
	}	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
