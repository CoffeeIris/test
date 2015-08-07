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
	
	m_features[0]="Logic Die��Physical Die��ӳ��";
	m_features[1]="����DNA��";
	m_features[2]="ʹ��MLC��־";
	m_features[3]="ʹ��two plane��־";
	m_features[4]="ʹ��SATA�ӿڱ�־";
	m_features[5]="ECC ��������";
	m_features[6]="Flash vender";
	m_features[7]="ԭʼ������";
	m_features[8]="ÿ��block���е�page��ƫ��";
	m_features[9]="page���е�sector�ĸ���ƫ��";
	m_features[10]="ÿ��DIE��block�ĸ���ƫ��";
	m_features[11]="ÿ��PU��DIE�ĸ���ƫ��";

	m_features[12]="ÿ��CE��DIE����ƫ��";
	m_features[13]="ÿ��Channel��CE����ƫ��";
	m_features[14]="Channel ��ƫ��";
	m_features[15]="������������x/128Ϊ��λ��";
	m_features[16]="ÿ��logic block������page��";

	m_features[17]="ÿ��logic blockӳ�䵽�������ĸ���";
	m_features[18]="ÿ��logic block���ܹ����Թҵ������ĸ���";
	m_features[19]="���������";
	m_features[20]="�ܱ������������";
	m_features[21]="���߼�����";

	m_features[22]="���û������LBA";
	m_features[23]="���û������SLB�����߼����";
	m_features[24]="PMT���page��";
	m_features[25]="���������SLB����";
	m_features[26]="���������LBA��С";
	m_features[27]="��LBA";
	m_features[28]="��PMT��ҳ��";
	m_features[29]="Max DISK LBA";
	m_features[30]="���PE��(��32λ)";
	m_features[31]="���PE��(��32λ)";
	m_features[32]="Rebuild�ҵ���table block�ĸ���";
	m_features[33]="Rebuild�ҵ��Ļ���ĸ���";
	m_features[34]="Rebuild�ҵ��Ŀտ�ĸ���";
	m_features[35]="Rebuild�ҵ���data��ĸ���";
	
	m_features[36]="��־";
	m_features[37]="Flash ID";
	m_features[38]="���ܿ�����";


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
