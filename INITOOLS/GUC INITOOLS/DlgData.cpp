// DlgData.cpp : implementation file
//

#include "stdafx.h"
#include "GUC INITOOLS.h"
#include "DlgData.h"
#include "SW80API.h"
#include  <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgData dialog


CDlgData::CDlgData(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgData::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgData)
	m_channel = 0;
	m_die = 0;
	m_block = 0;
	m_page = 0;
	//}}AFX_DATA_INIT
}


void CDlgData::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgData)
	DDX_Control(pDX, IDC_EDIT6, m_oob);
	DDX_Control(pDX, IDC_EDIT1, m_edhex);
	DDX_Text(pDX, IDC_EDIT2, m_channel);
	DDX_Text(pDX, IDC_EDIT3, m_die);
	DDX_Text(pDX, IDC_EDIT4, m_block);
	DDX_Text(pDX, IDC_EDIT5, m_page);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgData, CDialog)
	//{{AFX_MSG_MAP(CDlgData)
	ON_BN_CLICKED(IDC_BUTTON1, OnIDFY)
	ON_BN_CLICKED(IDC_BUTTON2, OnPageRead)
	ON_BN_CLICKED(IDC_BUTTON8, OnPageWrite)
	ON_BN_CLICKED(IDC_BUTTON10, OnSaveFile)
	ON_BN_CLICKED(IDC_BUTTON9, OnReadFile)
	ON_BN_CLICKED(IDC_BUTTON11, OnForward)
	ON_BN_CLICKED(IDC_BUTTON12, OnBack)
	ON_BN_CLICKED(IDC_BUTTON13, OnReadwithECC)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgData message handlers

void CDlgData::OnIDFY() 
{
	BYTE databuf[512];
	if(IDFY_IDE(GlobalDevice,databuf)==0)
		AfxMessageBox("Identify failed !");
	m_edhex.SetContent(databuf,512);
}

void CDlgData::OnPageRead() 
{
	
	BYTE databuf[32*512];
	DWORD pagenum;
	memset(databuf,0,32*512);
	UpdateData(TRUE);
	pagenum=m_block*flash_type[GlobalDevice].pages_per_block+m_page;
	if(SW80PageRead(GlobalDevice,m_channel,m_die,pagenum,1,databuf)==0)
		AfxMessageBox("Page read failed !");
	else
		m_edhex.SetContent(databuf+512,flash_type[GlobalDevice].size_page*1024);
	
}

void CDlgData::OnPageWrite() 
{
	BYTE databuf[16*512],oob[24];
	DWORD pagenum;

	memset(databuf,0,flash_type[GlobalDevice].size_page*1024);
	memset(oob,0,24);
	m_edhex.GetContent(databuf,flash_type[GlobalDevice].size_page*1024);
	UpdateData(TRUE);

	pagenum=m_block*flash_type[GlobalDevice].pages_per_block+m_page;
	if(SW80PageWrite(GlobalDevice,m_channel,m_die,pagenum,oob,databuf)==0)
		AfxMessageBox("Page Write failed !");
	

}

void CDlgData::OnSaveFile() 
{
	CFileDialog fdlg(false,NULL,NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,"Hex Data Files(*.hex)|*.hex|All Files(*.*)|*.*||",NULL);
	if(fdlg.DoModal()==IDOK)
	{
		CString m_strDesFile=fdlg.GetPathName();
		m_strDesFile.TrimRight();
		if (m_strDesFile.Right(4).Compare(".hex")!=0)
		{
			m_strDesFile+=".hex";
		}
		ExportToFile(m_strDesFile);
	}	
}

void CDlgData::ExportToFile(CString strFile)
{
	CString strLine="",str;
	CStdioFile fileNew(strFile,CFile::modeCreate|CFile::modeReadWrite);
	BYTE buffer[512*8] ;
	int i;
	memset(buffer, 0, 512*8);
	
	m_edhex.GetContent(buffer,8*512);
	for(i = 0;i< m_edhex.map_row*16;i++)
	{
		
		//line info
		if ( ((i+1)%16) == 0)
			str.Format("%X\r\n",buffer[i]);
		else
			str.Format("%X ",buffer[i]);
		strLine+=str;

	
		fileNew.WriteString(strLine);
		strLine="";
	}

	fileNew.Close();
}

void CDlgData::OnReadFile() 
{
	CFileDialog fdlg(true,NULL,NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,"Hex Data Files(*.hex)|*.hex|All Files(*.*)|*.*||",NULL);
	if(fdlg.DoModal()==IDOK)
	{
		CString m_strDesFile=fdlg.GetPathName();
		m_strDesFile.TrimRight();
		ImportFromFile(m_strDesFile);
	}	
}

void CDlgData::ImportFromFile(CString strFile)
{
	char buf[512*32] ;
	int i=0;
	CString strLine="";
	memset(buf, 0, 512*32);

	if((_access(strFile,0)   ==   -1))//´æÔÚ£¬-1²»´æÔÚ¡£// 
	{
		AfxMessageBox("No Hex data file£¡");
		return;
	}
	CStdioFile fileNew(strFile,CFile::modeRead);

	while(fileNew.ReadString(strLine))
	{
		sscanf(strLine.GetBuffer(1),"%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\r\n",&buf[16*i],&buf[16*i+1],&buf[16*i+2],
			&buf[16*i+3],&buf[16*i+4],&buf[16*i+5],&buf[16*i+6],&buf[16*i+7],&buf[16*i+8],&buf[16*i+9],&buf[16*i+10],&buf[16*i+11]
			,&buf[16*i+12],&buf[16*i+13],&buf[16*i+14],&buf[16*i+15]);
		i++;
	}
	

	fileNew.Close();
	m_edhex.SetContent(buf,16*512);
}


void CDlgData::OnForward() 
{
	if(m_edhex.map_row > 0)
		m_edhex.OnVScroll(SB_PAGEDOWN,32,NULL);	
}

void CDlgData::OnBack() 
{
	if(m_edhex.map_row > 0)
		m_edhex.OnVScroll(SB_PAGEUP,32,NULL);	
}

void CDlgData::OnReadwithECC() 
{
	BYTE databuf[32*512];
	DWORD pagenum;
	CString str,str1;
	int i;
	memset(databuf,0,32*512);
	UpdateData(TRUE);
	pagenum=m_block*flash_type[GlobalDevice].pages_per_block+m_page;
	if(SW80PageRead(GlobalDevice,m_channel,m_die,pagenum,0,databuf)==0)
		AfxMessageBox("Page read failed !");
	else
	{
		m_edhex.SetContent(databuf+512,flash_type[GlobalDevice].size_page*1024);	
		for(i=23;i<47;i++)
		{
			str1.Format("%x",databuf[i]);
			str+=str1;
		}
	//	str.Format("%x",str);
		m_oob.SetWindowText(str);
	}


}
