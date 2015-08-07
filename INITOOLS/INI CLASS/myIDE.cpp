
#include "stdafx.h"

#include "myIDE.h"


#include "OPini.h"
#include <math.h>

#if _MASSINIT 
#include "Resource.h"
#include "SoliWare MPToolDlg.h"
#endif

#if _MASSINIT 
extern CSoliWareMPToolDlg* pSoliWareMPToolDlg;
#endif


#define DiskClassGuid               GUID_DEVINTERFACE_DISK

#define IOCTL_STORAGE_BASE FILE_DEVICE_MASS_STORAGE
#define IOCTL_STORAGE_QUERY_PROPERTY   CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS {
SCSI_PASS_THROUGH Spt;
ULONG             Filler;      // realign buffers to double word boundary
UCHAR             SenseBuf[32];
UCHAR             DataBuf[512*128];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;


typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER {
    SCSI_PASS_THROUGH_DIRECT Spt;
    ULONG             Filler;      // realign buffer to double word boundary
    UCHAR             ucSenseBuf[32];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

Flash_Para flash_type;
BYTE flash_id[128][4];		// 	最多128 die, 每个die 有2 plane
VND_ARGS args;
INT32U ItemId=0;
CString m_result="";
int GlobalDevice;
BOOL checkpower;
BOOL checkpcb;
BOOL checkinit;
BOOL checkupdatefw;
BOOL checkupdatefpga;
BOOL BReadid[DeviceCount];
BOOL BVND[DeviceCount];
TEST_STRU IOtest[DeviceCount];
int Hardwarereset=0;

HANDLE Devicehandle[DeviceCount];
HANDLE	hEventEnumSSD;
Device_Map Devicemap[DeviceCount];
CString BadBlockFilename[DeviceCount];
CString FWFilename;
CString RebuildFilename;
CString	FPGAFilename;
CString	LoadFilename;
CString manufacture[DeviceCount];
CString capacity[DeviceCount];
CString	m_fw[DeviceCount];
CString	m_descriptor[DeviceCount];
CString	DiskInfo;
CString SelectBadblockfile;

int diepercs=1;
DWORD MaxLBA;
DWORD SSDCapacity;
int SSDID;
int IniMode;
BOOL PurgeFlag;
int chargetime;
int badblock[DeviceCount][128][512];		//坏块表,最多128 die,
int xblock[DeviceCount][128];			//坏块个数,最多128 die,
int page4k ;		// 1: 2K		2: 4K	4: 8K
int blocksize; 	// 1: 128K	2: 256k	4: 512k
int sectorperblock;		//block包含sector的数目，MLC 1024 SLC 512 
int channel=4;
int plane=2;
int gldie = 32; //逻辑die,约定一个channel有32个逻辑die,允许为空
int grdie;   //每个channel实际存在的物理die数目，可以根据DIEMAP取
int gcs = 8; //片选数目，一般一个片选有2个die(当前使用的坏块表文件是4个channel,8个cs)
int step =2;				//SS MC:step=2; HY step=1; plane内block地址递增是偶数还是基数
int datablock[DeviceCount][2][128];			//4k		最多128 die, 每个die 有2 plane
int startblock[DeviceCount][2][128];			//4k  	最多128 die, 每个die 有2 plane
int endblock[DeviceCount][2][128];			//4k		最多128 die, 每个die 有2 plane
int swapblock[DeviceCount][2][128];			//4k		最多128 die, 每个die 有2 plane
int replaceblock[DeviceCount][2][128][128];	//4k		每个plane 最多128 个坏块
int replaceblocknum[DeviceCount][2][128];		//4k
int reserveblock[DeviceCount][128];			//4k 坏块保留块,最多128 die

int Multiblock[DeviceCount];
int globleblock[DeviceCount];
int smartblock[DeviceCount];
int securityblock[DeviceCount];
int mapblock[DeviceCount][3][4];	// 4个用于map的block地址
int mapblockcnt[DeviceCount];	// 用于map的block 数
int mapsize[DeviceCount];		//映射表的大小，以sector为单位
DWORD MultiAddr[DeviceCount];
DWORD globleAddr[DeviceCount];
DWORD smartAddr[DeviceCount];
DWORD securityAddr[DeviceCount];
DWORD mapAddr[DeviceCount][3][4];
int DIEMAP[32];
PUCHAR GlobelBuffer[DeviceCount];
PUCHAR MapBuffer[DeviceCount];
int MinReserveblock[DeviceCount];
WORD IDFY32G[256];
WORD MLCFLASH[128];
WORD SLCFLASH[128];
WORD BCKMLCFLASH[128];
WORD BCKSLCFLASH[128];

int ConfNum;
int FLASHtypeNum;
BOOL FlagTime;
BOOL FlagStop;
int InterfaceType[DeviceCount];
int gucflag=0;
int SSDInterface=1;

FLASHFeatures FlashFeature[16];
DWORD Totalbadblock[DeviceCount];		//原始坏块统计值
FLASHENDURANCE FlashEndurance[64];
BOOL EnableTwoplane;
BOOL EnableNewrule;
int Resratio;
int Respage;
int Mapphyblock;
int Incphyblock;
int Datalength;
int MaxPU=16;
int MaptableOffset;
int ECCThreshold=0;
int FunCtlWord=0;

int SSDInitialize(int device)
{
	int ret=0;
	CString str;
	DWORD time = GetTickCount();
#if _INITD 
	pGuiServerExplorer->m_progress.SetPos(1);
#endif

	ret = SLWInitial(device,0);
	if(ret>0)
	{
		LogInfo(device,"Write MapTableToRAM!\r\n",3);
		if(SLWWriteGMBlock(device,0)==0)
		{
			AddInfo("Write MapTableToRAM failed!");
			LogInfo(device,"Write MapTableToRAM failed!\r\n",3);
			return -1;
		}
		LogInfo(device,"Write MapTableToRAM successfully!\r\n",3);
		if( SLWWriteMultiboot(device,0)==0)
		{
			AddInfo("Write Multiboot failed!");
			LogInfo(device,"Write Multiboot failed!\r\n",3);
			return -1;
		}
		if((VNDEraseSPI(device,13)==0)||(VNDEraseSPI(device,14)==0))
		{
			AddInfo("Erase SPI sector 13 or 14 failed!");
			LogInfo(device,"Erase SPI sector 13 or 14 failed!\r\n",3);
			return -1;
		}

#if _INITD 
		pGuiServerExplorer->m_progress.SetPos(90);
#endif
		AddInfo("******Initionalize completed!!!\r\n");
		LogInfo(device,"******Initionalize completed!!!\r\n",1);
	}

	time = GetTickCount() - time;
	time /= 1000;			// seconds
	int min = time / 60;// minute
	time %= 60;			// seconds
	str.Format("Elapse time : %d : %d ", min, time);
	AddInfo(str);

	if(ret>0)
	{
#if _INITD 
		pGuiServerExplorer->m_progress.SetPos(100);
		pGuiServerExplorer->m_progress.SetText("Initionalize completed");
		pGuiServerExplorer->m_progress.SetPos(0);
#endif
		return 1;
	}
	else
	{
#if _INITD 
		pGuiServerExplorer->m_progress.SetPos(100);
		pGuiServerExplorer->m_progress.SetText("Initionalize failed");
		pGuiServerExplorer->m_progress.SetPos(0);
#endif
		return -1;
	}


}

int WriteAll(int device)
{
	CString str;
	int elbn = 0;

	AddInfo("Start Write Logic Block...");
	DWORD time = GetTickCount();

	BYTE buf[8 * 1024];
	IDEREGS regs;

	regs.bFeaturesReg		= SLW_CMD_VND_SUB_WRITELOGICBLOCK ;		// send  command
	regs.bSectorCountReg	= 0;						// max die number per channel
	regs.bSectorNumberReg	= 0 ;					
	regs.bCylLowReg	=		0;							// sector length in block
	regs.bCylHighReg =		0;
	regs.bDriveHeadReg =	0;		
	if( !PIO_VENDOR_OPS(device, true, buf, 0, &regs))
	{
		AddInfo("*SLW_CMD_VND_SUB_WRITELOGICBLOCK command rejected*");
		return 0 ;
	}
	else
	{
		AddInfo("PROGBLOCK command send ok_");
	}

	Sleep(5000);
	int step = 0;
	int errors = 0;

	while(1)
	{
		Sleep(1000);
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= SLW_CMD_VND_SUB_WRITELOGICBLOCK ;  // request command status
		regs.bCylLowReg			= 0x00;
		if( !PIO_VENDOR_OPS(device, false, buf, 0, &regs))
		{
			AddInfo("*READSTA command rejected*");
			return 0;
		}

		if( regs.bCylHighReg != 0xFF)
		{
			int temp = (regs.bCylLowReg << 8) | regs.bSectorNumberReg;
			if( (temp - step) == 0)
			{
				errors++;
				if(errors > 20)
				{
					AddInfo("__Time Out__");
					return 0;
				}
			}
			else
			{
				errors = 0;
			}

			step = temp;
			str.Format("Write at : %d", temp);
			AddInfo(str);
#if _INITD 
			pGuiServerExplorer->m_progress.SetPos(temp*100/3840);
#endif		
		}
		else
		{
			str.Format("PROGBLOCK Command finished. status:%02X",regs.bCylHighReg);
			AddInfo(str);
			break ;
		}
	}
	
	DWORD LBA;
	LBA = (regs.bCylLowReg << 8) | regs.bSectorNumberReg;

	DWORD sc;
	sc = regs.bSectorCountReg;	// bSectorCountReg 记录错误个数， 每个有8 byte

	str.Format("Data LBA: %04X, SC:%d", LBA, sc);
	AddInfo(str);

	if(sc != 0)
	{
		str.Format("Total Errors:%d", sc );
		AddInfo(str);
		if((sc * 8) > 8 * 1024)
		{
			sc = 8 * 1024;
			AddInfo("sc > 8 * 1024");
		}

		int sector = sc * 8 ;
		if( (sector % 512) != 0)
		{
			sector /= 512;
			sector++;
		}
		else
			sector /= 512;

		if( !DMA_OPS( device,buf, sector * 512, LBA, false, ata_READ_DMA))  
		{
			AddInfo("DMA READ ERROR DATA Failed!");
			return 0;
		}

		int ch_num ;
		int die_num;
		int add_des;
		int errType;
		int loc;
		int j;
		BOOL flag;
		for(int i = 0; i < sc; i++)
		{
			flag=0;
			ch_num = buf[i * 8 + 0];
			die_num = buf[i * 8 + 2];
			add_des = (buf[i * 8 + 5] << 8) | buf[i * 8 + 4];
			errType = buf[i * 8 + 6];
			str.Format("CH: %d, die_number: %d, block address: %d   Error:%d", ch_num, die_num, add_des, errType );
			AddInfo(str);
			loc=ch_num*gldie+die_num;
			for(j=0;j<xblock[device][loc];j++)
			{
				if(add_des==badblock[device][loc][j])
				{
					flag=1;
					break;
				}
			}
			if(flag==0)
			{
				badblock[device][loc][xblock[device][loc]] = add_des;   //badblock的第二维对应逻辑die,所以要转换为i+m_row-i%16
				xblock[device][loc]++;
			}
		}
	}
	else
		AddInfo("*** No Write Error Block!!!");

			

	time = GetTickCount() - time;
	time /= 1000;			// seconds

	int min = time / 60;// minute
	time %= 60;			// seconds

	str.Format("Elapse time : %d : %d ", min, time);
	AddInfo(str);

	AddInfo("*** Write Logic Block Passed !!!");
#if _INITD 
	pGuiServerExplorer->m_progress.SetPos(100);
	pGuiServerExplorer->m_progress.SetText("Write Logic Block Passed");
	pGuiServerExplorer->m_progress.SetPos(0);
#endif
	return 1;
}


BOOL VNDReadSPI(int device,BYTE* DATABuffer, int SectorOff)
{

	IDEREGS regs;
	CString str;

	regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSPISECTOR ;	// send  command
	regs.bSectorCountReg	= SectorOff;					// channel 0, die 0
	regs.bSectorNumberReg	= 0 ;						// page address: 0x00
	regs.bCylLowReg	= 0;
	regs.bCylHighReg = 0;
	regs.bDriveHeadReg = 0;		// elbn

	if( !PIO_VENDOR_OPS(device, true, DATABuffer, 0, &regs))
	{
		AddInfo("* command rejected *");
		return 0;
	}

	int timeout = 10;
	while(timeout--)
	{
		Sleep(100);
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= SLW_CMD_VND_SUB_READSPISECTOR;  // request command status
		regs.bCylLowReg			= 0x00;
		if( !PIO_VENDOR_OPS( device,false, DATABuffer, 0, &regs))
		{
			AddInfo("*READSTA command rejected*");
			return 0;
		}

		if( regs.bCylHighReg != 0xFF)
		{
			str.Format(" command status:%02X. SM:%d ADD: 0x%04X",regs.bCylHighReg, regs.bFeaturesReg, (regs.bCylLowReg << 8) | regs.bSectorNumberReg);
			AddInfo(str);
		}
		else
		{
			str.Format("Command finished. status:%02X, SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
			break;
		}
	}
	if(timeout < 0)
	{
		AddInfo("*Command busy and timeout*");
		AddInfo("*cancel command *");
		
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;			// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= SLW_CMD_VND_SUB_READSPISECTOR;	// request command status
		regs.bCylLowReg			= 0x01;								// abort this command
		if( !PIO_VENDOR_OPS(device, false, DATABuffer, 0, &regs))
		{
			AddInfo("*Abort command rejected*");
		}
		return 0;
	}
	
	DWORD LBA;
	LBA = (regs.bCylLowReg << 8) | regs.bSectorNumberReg;

	DWORD sc;
	sc = regs.bSectorCountReg;

	str.Format("Data LBA: %04X, SC:%d", LBA, sc);


	if(sc == 0)
	{
		AddInfo("SC == 0");
		return 0;
	}

	if( !DMA_OPS(device, DATABuffer, sc * 512, LBA, false, ata_READ_DMA))  // LBA > 0x1000000
	{
		AddInfo("DMA READ Failed!");
		return 0;
	}
	
//	pDataView->m_edhex.SetContent(buf, 32 * 1024);

	return 1;

}

BOOL VNDEraseSPI(int device,int SectorOff)
{
	unsigned char DATABuffer[512];
	IDEREGS regs;
	regs.bFeaturesReg		= SLW_CMD_VND_SUB_ERASESPIFLASHSECTOR ;	// send  command
	regs.bSectorCountReg	= SectorOff;						// ram-disk code in spi
	regs.bSectorNumberReg	= 0 ;						
	regs.bCylLowReg	= 0;
	regs.bCylHighReg = 0;
	regs.bDriveHeadReg = 0;		

	if( !PIO_VENDOR_OPS(device, true, DATABuffer, 0, &regs))
	{
		AddInfo("* command rejected*");
		return 0;
	}
	else
	{
		AddInfo("* command send ok *");
	}

	CString str;
	int timeout = 20;
	while(timeout--)
	{
		Sleep(200);
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;			// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= SLW_CMD_VND_SUB_ERASESPIFLASHSECTOR;	// request command status
		regs.bCylLowReg			= 0x00;
		if( !PIO_VENDOR_OPS( device,false, DATABuffer, 0, &regs))
		{
			AddInfo("*READSTA command rejected*");
			return 0;
		}

		if( regs.bCylHighReg != 0xFF)
		{
			str.Format(" command status:%02X. SM:%d ADD: 0x%04X",regs.bCylHighReg, regs.bFeaturesReg, (regs.bCylLowReg << 8) | regs.bSectorNumberReg);
			AddInfo(str);
		}
		else
		{
			str.Format("Command finished. status:%02X, SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
			return 1;
		}
	}
	AddInfo("*** SPI Erase timeout!!!");
	return 0;
}

BOOL VNDWriteSPI(int device,BYTE* DATABuffer, DWORD LBA,int SectorOff)
{
	UINT datasize = 64 * 1024;
	if( !DMA_OPS(device, DATABuffer, datasize,LBA, true, ata_WRITE_DMA))  
	{
		AddInfo("DMA WRITE Failed!");
		return 0;
	}

	IDEREGS regs;
	regs.bFeaturesReg		= SLW_CMD_VND_SUB_WRITESPISECTOR ;	// send  command
	regs.bSectorCountReg	= SectorOff;						// ram-disk code in spi
	regs.bSectorNumberReg	= 0 ;						
	regs.bCylLowReg	= 0;
	regs.bCylHighReg = 0;
	regs.bDriveHeadReg = 0;		// elbn

	if( !PIO_VENDOR_OPS(device, true, DATABuffer, 0, &regs))
	{
		AddInfo("* command rejected*");
		return 0;
	}
	else
	{
		AddInfo("* command send ok *");
	}

	CString str;
	int timeout = 20;
	while(timeout--)
	{
		Sleep(800);
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;			// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= SLW_CMD_VND_SUB_WRITESPISECTOR;	// request command status
		regs.bCylLowReg			= 0x00;
		if( !PIO_VENDOR_OPS( device,false, DATABuffer, 0, &regs))
		{
			AddInfo("*READSTA command rejected*");
			return 0;
		}

		if( regs.bCylHighReg != 0xFF)
		{
			str.Format(" command status:%02X. SM:%d ADD: 0x%04X",regs.bCylHighReg, regs.bFeaturesReg, (regs.bCylLowReg << 8) | regs.bSectorNumberReg);
			AddInfo(str);
		}
		else
		{
			str.Format("Command finished. status:%02X, SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
			AddInfo(str);
			return 1;
		}
	}
	AddInfo("*** SPI Write timeout!!!");
	return 0;
}

/*
Erase block的返回值
-2 command rejected
-1 Erase Block Error,this block is a bad block
1   success
*/
int VNDEraseblk(int device,int channelnum,int dienum,DWORD blocknum)
{
	BYTE buf[512];
	IDEREGS regs;
	CString str;

	regs.bFeaturesReg		= SLW_CMD_VND_SUB_ERASEBLOCK;	// send  command
	regs.bSectorCountReg	= dienum;			
	regs.bSectorNumberReg	= blocknum ;					
	regs.bCylLowReg	= blocknum  >> 8;
	regs.bCylHighReg = 0 ;
	regs.bDriveHeadReg = channelnum;		

	if( !PIO_VENDOR_OPS( device,true, buf, 0, &regs))
	{
		AddInfo("* ERASEBLOCK command rejected*");
		return -2;
	}

	if( !DMA_OPS( device,buf, 512, 0, false, ata_READ_DMA))  // LBA > 0x1000000
	{
		LogInfo(device,"DMA READ Failed!",3);
		return -2;
	}

	if( buf[0]== 0xA1)
	{		
		str.Format("Erase block channel %d,die %d,block %d failed!\r\n",channelnum,dienum,blocknum);
		LogInfo(device,str,3);
		AddInfo(str);
		return -1;
	}
	else if( buf[0]== 0xA0)
		return 1;
	else
	{		
		str.Format("regs.bFeaturesReg:%x!\r\n",buf[0]);
		LogInfo(device,str,3);
		return 1;
	}
}


BOOL VNDReadblock(int device,BYTE* buf,int channelnum,int dienum,DWORD blocknum)
{

	CString str;
	IDEREGS regs;
	BYTE readbuf[512];

	regs.bFeaturesReg		= SLW_CMD_VND_SUB_READBLOCK;	// send  command
	regs.bSectorCountReg	= dienum;			// channel , die
	regs.bSectorNumberReg	= blocknum;					// block address
	regs.bCylLowReg	= blocknum >> 8;
	regs.bCylHighReg = 0;
	regs.bDriveHeadReg = channelnum;		

	if( !PIO_VENDOR_OPS(device, true, buf, 0, &regs))
	{
		AddInfo("*READBLOCK command rejected*");
		return 0;
	}


	DWORD LBA=0,sc=512;
	memset(readbuf,0,512);
	if( !DMA_OPS( device,readbuf, 512, 0, false, ata_READ_DMA))  // LBA > 0x1000000
	{
		LogInfo(device,"DMA READ Failed!",3);
		return 0;
	}

	if( readbuf[0]== 0xA1)
	{	
		LogInfo(device,"*READBLOCK command failed",3);
		return 0;
	}
	else if( readbuf[0]== 0xA0)
	{

		LBA=readbuf[8] | (readbuf[9]<<8) | (readbuf[10]<<16) | (readbuf[11]<<24);
		sc=readbuf[12] | (readbuf[13]<<8) | (readbuf[14]<<16) | (readbuf[15]<<24);
		str.Format("Data LBA: %04X sectors: %d", LBA, sc);
	//	LogInfo(device,str,3);
		if(sc == 0)
		{
			AddInfo("ERROR sector count == 0");
			LogInfo(device,"ERROR sector count == 0!\r\n",3);
			return 0;
		}

		if( !DMA_OPS(device, buf, sc * 512, LBA, false, ata_READ_DMA))  // LBA > 0x1000000
		{
			AddInfo("*** DMA READ Failed!");
			LogInfo(device,"DMA READ Failed!\r\n",3);
			return 0;
		}
		return 1;
	}
	else
	{		
		str.Format("regs.bFeaturesReg:%x!\r\n",buf[0]);
		LogInfo(device,str,3);
		return 0;
	}
}

/*
Write block的返回值

-2 command rejected
-1 write Block Error,this block is a bad block
1   success
*/

int VNDWriteblock(int device,BYTE* buf,int channelnum,DWORD blocknum,int sector)
{


	IDEREGS regs;
	CString str;	
	BYTE buf1[512];
	memset(buf1,0,512);

	if( !DMA_OPS( device,buf, sector * 512, 0, true, ata_WRITE_DMA))  
	{
		AddInfo("DMA write in write block failed!");
		return -2;
	}

	DWORD block_add;		
	block_add = blocknum ;	

	regs.bFeaturesReg		= SLW_CMD_VND_SUB_WRITEBLOCK ;		// send  command
	regs.bSectorCountReg	= block_add;						// channel 0, die 0
	regs.bSectorNumberReg	=  block_add  >> 8;						// page address: 0x00
	regs.bCylLowReg	=		block_add >> 16	;
	regs.bCylHighReg =	block_add >> 24	;
	regs.bDriveHeadReg =channelnum;		
	if( !PIO_VENDOR_OPS(device, true, buf, 0, &regs))
	{
		AddInfo("*PROGBLOCK command rejected*");
		return -2;
	}
	
	if( !DMA_OPS( device,buf1, 512, 0, false, ata_READ_DMA))  // LBA > 0x1000000
	{
		LogInfo(device,"DMA READ Failed!",3);
		return -2;
	}
	if( buf1[0]== 0xA1)
	{		
		LogInfo(device,"PROGBLOCK command failed!",3);
		return -1;
	}
	else if( buf1[0]== 0xA0)
		return 1;
	else
	{		
		str.Format("regs.bFeaturesReg:%x!\r\n",buf1[0]);
		LogInfo(device,str,3);
		return 1;
	}

}

BOOL WriteLoginblock(int device, DWORD total, DWORD dLBA,int sector)
{
	CString drive;
	DWORD i,j;
	DWORD num;
	int k,remainder=0;
	if(FindSSD()==0)
	{
		AddInfo("Can not find SSD device!!!");
		return 0;
	}

	if(total%sector>0)
		remainder=total%sector;

	num=total/sector;
	DWORD time = GetTickCount();
	int datasize = sector*512;	
	BYTE *buf;
	buf = new BYTE [datasize];
	CString str;
	str.Format("TOTAL LBA: %u",total);
	AddInfo(str);

	for(i=0;i<num;i++)
	{

		memset(buf,0,datasize);
		if(DMA_OPS(device,buf,sector*512,dLBA+i*sector,true, ata_WRITE_DMA))
		{
#if _INITD 
			pGuiServerExplorer->m_progress.SetPos(100*i/num);		
#endif
		}		
		else 
		{
			delete [datasize] buf;
			return 0;
		}
	}

	if(remainder>0)
	{
		for(j=0;j<remainder;j++)
		{
			for(k=0;k<512;k++)
				buf[j*512 +k]=0xFF;

		}


		if(!DMA_OPS(device,buf,remainder*512,dLBA+i*sector,true, ata_WRITE_DMA))
		{	
			delete [datasize] buf;
			return 0;
			
		}
	}
	
	time = GetTickCount() - time;
	time /= 1000;			// seconds

	int min = time / 60;// minute
	time %= 60;			// seconds

	str.Format("Elapse time : %d : %d ", min, time);
	AddInfo(str);

	AddInfo("*** Write Logic Block Passed !!!");

	delete [datasize] buf;
	return 1;
}

BOOL VNDReadspisector(int device)
{
	BYTE buf[64 * 1024];
	IDEREGS regs;
	CString str;
	
	BYTE sector = 0;
	if(args.page > 15)
		sector = 15;

	regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSPISECTOR ;	// send  command
	regs.bSectorCountReg	= sector;					// channel 0,die 0
	regs.bSectorNumberReg	= 0 ;						// page address: 0x00
	regs.bCylLowReg	= 0;
	regs.bCylHighReg = 0;
	regs.bDriveHeadReg = 0;		// elbn

	if( !PIO_VENDOR_OPS( device,true, buf, 0, &regs))
	{
		AddInfo("* command rejected*");
		return false;
	}
	else
	{
		AddInfo(" command send ok_");
	}

	int timeout = 10;
	while(timeout--)
	{
		Sleep(500);
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= SLW_CMD_VND_SUB_READSPISECTOR;  // request command status
		regs.bCylLowReg			= 0x00;
		if( !PIO_VENDOR_OPS(device, false, buf, 0, &regs))
		{
			AddInfo("*READSTA command rejected*");
			return false;
		}

		if( regs.bCylHighReg != 0xFF)
		{
			str.Format(" command status:%02X. SM:%d ADD: 0x%04X",regs.bCylHighReg, regs.bFeaturesReg, (regs.bCylLowReg << 8) | regs.bSectorNumberReg);
			AddInfo(str);
		}
		else
		{
			str.Format("Command finished. status:%02X, SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
			AddInfo(str);
			break;
		}
	}
	if(timeout < 0)
	{
		AddInfo("*Command busy and timeout*");
		AddInfo("*cancel command *");
		
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;			// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= SLW_CMD_VND_SUB_READSPISECTOR;	// request command status
		regs.bCylLowReg			= 0xFF;								// abort this command
		if( !PIO_VENDOR_OPS(device, false, buf, 0, &regs))
		{
			AddInfo("*Abort command rejected*");
		}
		return false;
	}
	
	DWORD LBA;
	LBA = (regs.bCylLowReg << 8) | regs.bSectorNumberReg;
	str.Format("Data LBA: %04X sectors: %d", LBA, regs.bSectorCountReg);
	AddInfo(str);

	if( !DMA_OPS(device, buf, regs.bSectorCountReg, LBA, false, ata_READ_DMA_EXT))  // LBA > 0x1000000
	{
		AddInfo("DMA READ Failed!");
		return false;
	}
	
	str.Format("ReadSector: %02X %02X %02X %02X %02X %02X %02X %02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
	AddInfo(str);

//	pDataView->m_edhex.SetContent(buf, 32 * 1024);
	AddInfo("*** Read SPI SECTOR Pass!!!");
	return true;
}

BOOL VNDReadSector(int device,BYTE* buf,int channelnum,int dienum,DWORD page,int sector)
{
	CString str;

	IDEREGS regs;

	if(flash_type.sectors_per_page > 8 || flash_type.sectors_per_page == 0)
	{
		AddInfo("sectors_per_page should below 8 and above 0");
		return false;
	}

	regs.bFeaturesReg		= SLW_CMD_VND_SUB_READPAGE;	// send  command
	regs.bSectorCountReg	= dienum;
	regs.bSectorNumberReg	= page; // page address: 0x00
	regs.bCylLowReg	= page >> 8;
	regs.bCylHighReg = page >> 16;
	regs.bDriveHeadReg = channelnum;	

	if( !PIO_VENDOR_OPS(device, true, buf, 0, &regs))
	{
		AddInfo("* command rejected*");
		return false;
	}
/*
   LARGE_INTEGER litmp; 
   LONGLONG QPart1,QPart2;
   double dfMinus, dfFreq, dfTim; 
   QueryPerformanceFrequency(&litmp);
   dfFreq = (double)litmp.QuadPart;   // 获得计数器的时钟频率
   QueryPerformanceCounter(&litmp);
   QPart1 = litmp.QuadPart;               // 获得初始值
   do
   {
      QueryPerformanceCounter(&litmp);
      QPart2 = litmp.QuadPart;   //获得中止值
      dfMinus = (double)(QPart2-QPart1);
      dfTim = dfMinus / dfFreq;    // 获得对应的时间值，单位为秒
   }while(dfTim<0.000256);

	int timeout = 10;
	while(timeout--)
	{
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= SLW_CMD_VND_SUB_READPAGE;  // request command status
		regs.bCylLowReg			= 0x00;
		if( !PIO_VENDOR_OPS( device,false, buf, 0, &regs))
		{
			AddInfo("*READSTA command rejected*");
			return false;
		}

		if( regs.bCylHighReg != 0xFF)
		{
			str.Format(" command status:%02X. SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
			AddInfo(str);
		}
		else
		{
			str.Format("Command finished. status:%02X, SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
		//	AddInfo(str);
			break;
		}
	}
	if(timeout < 0)
	{
		AddInfo("*Command busy and timeout*");
		AddInfo("*cancel command *");
		
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= 0;  
		regs.bCylLowReg			= 0xFF;						
		if( !PIO_VENDOR_OPS( device,false, buf, 0, &regs))
		{
			AddInfo("*Abort command rejected*");
		}
		return false;
	}
*/
//	Sleep(10);

	regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
	regs.bSectorCountReg	= 0x00;	
	regs.bSectorNumberReg	= SLW_CMD_VND_SUB_READPAGE;  // request command status
	regs.bCylLowReg			= 0x00;
	if( !PIO_VENDOR_OPS( device,false, buf, 0, &regs))
	{
		AddInfo("*READSTA command rejected*");
		return false;
	}

	if( regs.bCylHighReg != 0xFF)
	{
		str.Format(" command status:%02X. SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
		AddInfo(str);
		return false;
	}


	DWORD LBA;
	LBA = (regs.bCylLowReg << 8) | regs.bSectorNumberReg;
	if( !DMA_OPS( device,(BYTE *)buf, 512, LBA, false, ata_READ_DMA))  // LBA > 0x1000000
	{
		AddInfo("DMA READ Failed!");
		return false;
	}
	
#if _INITD 
//	pDataView->m_edhex.SetContent(buf, flash_type.sectors_per_page * 512);
#endif
	return true;
}

BOOL VNDReadpage(int device,BYTE* buf,int channelnum,int dienum,DWORD page)
{
	CString str;

	IDEREGS regs;

	regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
	regs.bSectorCountReg	= 0x00;	
	regs.bSectorNumberReg	= 0;  
	regs.bCylLowReg			= 0xFF;							// abort last command
	if( !PIO_VENDOR_OPS( device,false, buf, 0, &regs))
	{
		AddInfo("*Abort command rejected*");
	}

	if(flash_type.sectors_per_page > 8 || flash_type.sectors_per_page == 0)
	{
		AddInfo("sectors_per_page should below 8 and above 0");
		return false;
	}

	for(DWORD i= 0; i < flash_type.sectors_per_page; i++)
	{
		VNDSetBlockArg(device,i,1,0);
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READPAGE;	// send  command
		regs.bSectorCountReg	= dienum;
		regs.bSectorNumberReg	= page; // page address: 0x00
		regs.bCylLowReg	= page >> 8;
		regs.bCylHighReg = page >> 16;
		regs.bDriveHeadReg = channelnum;	

		if( !PIO_VENDOR_OPS(device, true, buf, 0, &regs))
		{
			AddInfo("* command rejected*");
			return false;
		}

		int timeout = 10;
		while(timeout--)
		{
	//		Sleep(1);
			regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
			regs.bSectorCountReg	= 0x00;	
			regs.bSectorNumberReg	= SLW_CMD_VND_SUB_READPAGE;  // request command status
			regs.bCylLowReg			= 0x00;
			if( !PIO_VENDOR_OPS( device,false, buf, 0, &regs))
			{
				AddInfo("*READSTA command rejected*");
				return false;
			}

			if( regs.bCylHighReg != 0xFF)
			{
				str.Format(" command status:%02X. SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
	//			AddInfo(str);
			}
			else
			{
				str.Format("Command finished. status:%02X, SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
			//	AddInfo(str);
				break;
			}
		}
		if(timeout < 0)
		{
			AddInfo("*Command busy and timeout*");
			AddInfo("*cancel command *");
			
			regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
			regs.bSectorCountReg	= 0x00;	
			regs.bSectorNumberReg	= SLW_CMD_VND_SUB_READPAGE;  // request command status
			regs.bCylLowReg			= 0xFF;							// abort this command
			if( !PIO_VENDOR_OPS( device,false, buf, 0, &regs))
			{
				AddInfo("*Abort command rejected*");
			}
			return false;
		}
	
		DWORD LBA;
		LBA = (regs.bCylLowReg << 8) | regs.bSectorNumberReg;
	//	int i=0;		
		//if( !DMA_OPS( (BYTE *)&buf[i * 512], 512, LBA, false, ata_READ_DMA_EXT))  // LBA > 0x1000000
		if( !DMA_OPS( device,(BYTE *)&buf[i * 512], 512, LBA, false, ata_READ_DMA))  // LBA > 0x1000000
		{
			AddInfo("DMA READ Failed!");
			return false;
		}

	}
	
#if _INITD 
//	pDataView->m_edhex.SetContent(buf, flash_type.sectors_per_page * 512);
#endif
	return true;
}

BOOL VNDSetparam(int device)
{
	CString str;
	
	if(BVND[device] == 1)
		return 1;
	else
		BVND[device] = 1;

	if( !SLW_SSD_DBG(device,true))
	{
		AddInfo("**Can't ENTER VENDOR MODE!**");
		return 0;
	}
	else
		AddInfo("*Enter VENDOR MODE*");
	
	return 1;
}

BOOL VNDSetBlockArg(int device,int offset,int length,int strip)
{
	CString str;
	BYTE buf[512];
	IDEREGS regs;

	regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
	regs.bSectorCountReg	= 0x00;	
	regs.bSectorNumberReg	= 0;  
	regs.bCylLowReg			= 0x01;							// abort last command
	if( !PIO_VENDOR_OPS(device, false, buf, 0, &regs))
	{
		AddInfo("*Abort command rejected*");
		return 0;
	}

	regs.bFeaturesReg		= SLW_CMD_VND_SUB_RW_PARA;	// send  command
	regs.bSectorCountReg	= offset;			
	regs.bSectorNumberReg	= offset>>8;					
	regs.bCylLowReg	= length;
	regs.bCylHighReg = length >> 8;
	regs.bDriveHeadReg = strip;	

	if( !PIO_VENDOR_OPS(device, true, buf, 0, &regs))
	{
		AddInfo("* command rejected*");
		return 0;
	}

	int timeout = 10;
	while(timeout--)
	{
	//	Sleep(1);
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= SLW_CMD_VND_SUB_RW_PARA;  // request command status
		regs.bCylLowReg			= 0x00;
		if( !PIO_VENDOR_OPS(device, false, buf, 0, &regs))
		{
			AddInfo("*READSTA command rejected*");
			return 0;
		}

		if( regs.bCylHighReg != 0xFF)
		{
			str.Format(" command status:%02X. SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
			AddInfo(str);
		}
		else
		{
			str.Format("Command finished. status:%02X, SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
		//	AddInfo(str);
			break;
		}
	}
	if(timeout < 0)
	{
		AddInfo("*Command busy and timeout*");
		AddInfo("*cancel command *");
		
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= SLW_CMD_VND_SUB_RW_PARA;  // request command status
		regs.bCylLowReg			= 0x01;							// abort this command
		if( !PIO_VENDOR_OPS(device, false, buf, 0, &regs))
		{
			AddInfo("*Abort command rejected*");
		}
		return 0;
	}

	return 1;
}


BOOL VNDPadRead(int device,int channelnum,int dienum,DWORD page)
{
	CString str;
	BYTE buf[512];
	IDEREGS regs;

	regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
	regs.bSectorCountReg	= 0x00;	
	regs.bSectorNumberReg	= 0;  // request command status
	regs.bCylLowReg			= 0x01;							// abort last command
	if( !PIO_VENDOR_OPS(device, false, buf, 0, &regs))
	{
		AddInfo("*Abort command rejected*");
		return 0;
	}

	Sleep(10);

	if(flash_type.sectors_per_page > 8 || flash_type.sectors_per_page == 0)
	{
		AddInfo("sectors_per_page should below 8 and above 0");
		return 0;
	}

	regs.bFeaturesReg		= SLW_CMD_VND_SUB_READPAGEPAD;	// send  command
	regs.bSectorCountReg	= dienum;
	regs.bSectorNumberReg	= page; // page address: 0x00
	regs.bCylLowReg	= page >> 8;
	regs.bCylHighReg = page >> 16;
	regs.bDriveHeadReg = channelnum;

	if( !PIO_VENDOR_OPS(device, true, buf, 0, &regs))
	{
		AddInfo("* command rejected*");
		return 0;
	}
	else
	{
		AddInfo("SLW_CMD_VND_SUB_READPAGEPAD command send ok_");
	}

	int timeout = 10;
	while(timeout--)
	{
		Sleep(100);
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= SLW_CMD_VND_SUB_READPAGEPAD;  // request command status
		regs.bCylLowReg			= 0x00;
		if( !PIO_VENDOR_OPS(device, false, buf, 0, &regs))
		{
			AddInfo("*READSTA command rejected*");
			return 0;
		}

		if( regs.bCylHighReg != 0xFF)
		{
			//str.Format(" command status:%02X. SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
			str.Format("READSECTOR command status:%02X. SM:%d ST_REG: 0x%04X",regs.bCylHighReg, regs.bFeaturesReg, regs.bSectorNumberReg | (regs.bCylLowReg << 8));
			AddInfo(str);
		}
		else
		{
			str.Format("Command finished. status:%02X, SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
			AddInfo(str);
			break;
		}
	}
	if(timeout < 0)
	{
		AddInfo("*Command busy and timeout*");
		AddInfo("*cancel command *");
		
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= SLW_CMD_VND_SUB_READPAGEPAD;  // request command status
		regs.bCylLowReg			= 0x01;							// abort this command
		if( !PIO_VENDOR_OPS(device, false, buf, 0, &regs))
		{
			AddInfo("*Abort command rejected*");
		}
		return 0;
	}
	
	DWORD LBA;
	LBA = (regs.bCylLowReg << 8) | regs.bSectorNumberReg;

	DWORD sc;
	sc = regs.bSectorCountReg;

	str.Format("Data LBA: %04X, SC:%d", LBA, sc);
	AddInfo(str);

	if(sc != 1)
	{
		AddInfo("SC != 1");
		return 0;
	}

	if( !DMA_OPS(device, buf, 512, LBA, false, ata_READ_DMA))  // LBA > 0x1000000
	{
		AddInfo("DMA READ Failed!");
		return 0;
	}
	
	str.Format("ReadSector: %02X %02X %02X %02X %02X %02X %02X %02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
	AddInfo(str);

#if _INITD 
	pDataView->m_edhex.SetContent(buf, 512);
#endif

	AddInfo("****Read Page pad Pass!!!\r\n");
	return 1;
}

int SLWFlashIDPos()
{
	int i,j;
	CString str;
	CString IDType[4];

	//当前的Flash ID 类型
	IDType[0]="ECD310A6";
	IDType[1]="ECD551A6";
	IDType[2]="ECD514B6";
	IDType[3]="ECD755B6";

	for(i=0;i<64;i++)
	{
		str.Format("%X%X%X%X",flash_id[i][1],flash_id[i][0],flash_id[i][3],flash_id[i][2]);
		for(j=0;j<4;j++)
			if(str==IDType[j])
				return i;				
	}
}

int SW80FlashIDPos()
{
	int i,j;

	for(i=0;i<64;i++)
	{
		for(j=0;j<16;j++)
		{
			if(flash_id[i][0]==FlashEndurance[j].ID[0] && flash_id[i][1]==FlashEndurance[j].ID[1] && flash_id[i][2]==FlashEndurance[j].ID[2] && flash_id[i][3]==FlashEndurance[j].ID[3]) 
				return j;		
		}
	}
}

BOOL VNDReadflashid(int device)
{
	CString str;
	BYTE buf[512];
	IDEREGS regs;
	BYTE lchannel;
	int ldie;
	int dienum=0;
	int idflag[4];
	int i,j;
	int k;

	int locgrdie=0;
	int locgcs=0;
#if _INITD 
	if(BReadid[device] == 1)
		return 1;
	else
		BReadid[device] = 1;
#endif

	CString file; 
	file=SLWCONFINI;
	ReadConfigureIni(file);
	for(i=0;i<gldie;i++)
		DIEMAP[i]=65535;
	
	
	regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
	regs.bSectorCountReg	= 0x00;	
	regs.bSectorNumberReg	= 0;  // request command status
	regs.bCylLowReg			= 0x01;							// abort last command
	if( !PIO_VENDOR_OPS( device,false, buf, 0, &regs))
	{
		AddInfo("*Abort READATcommand rejected*");
		LogInfo(device,"ReadID Abort READATcommand rejected !\r\n",3);
		return false;
	}
	else
		AddInfo("*Abort Last CMD OK*");

	Sleep(10);
	diepercs=1;
	for(lchannel = 0; lchannel < 4; lchannel++)
	{
	
		for(ldie = 0; ldie < gldie; ldie++)
		{

				if( (ldie<16&&ldie>7) || (ldie<32&&ldie>23) )
					continue;
				regs.bFeaturesReg		= SLW_CMD_VND_SUB_READFLASHID;	// send READFLASHID command
				regs.bSectorCountReg =  ldie;
				regs.bDriveHeadReg = lchannel;
				if( !PIO_VENDOR_OPS(device, true, buf, 0, &regs))
				{
					AddInfo("*READFLASHID command rejected*");
					LogInfo(device,"READFLASHID  command rejected !\r\n",3);
					return false;
				}

				DWORD LBA;
				LBA=0;
				DWORD sc;
				sc = 1;
			
				if(sc == 0)
				{
					AddInfo("SC == 0");
					LogInfo(device,"SC == 0",3);
					return false;
				}

				if( !DMA_OPS(device,buf, sc * 512, LBA, false, ata_READ_DMA))  // LBA > 0x1000000
				{
					AddInfo("DMA READ Failed!");
					LogInfo(device,"DMA READ Failed",3);
					return false;
				}
				str.Format("Read ID: Channel:%d cs:%d = %02X %02X %02X %02X %02X %02X %02X %02X",lchannel, locgcs%16,  
								buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
				AddInfo(str);
				LogInfo(device,str,3);
				memcpy(&(flash_id[lchannel * gldie + ldie ][0]), buf, 4);
				memcpy(&(flash_id[lchannel * gldie + ldie+8][0]), buf, 4);
				if(buf[1] == 0xEC || buf[1] == 0x89 || buf[1] == 0x2C || buf[1] == 0x98)
				{
					locgrdie=locgrdie+2;
					diepercs=flash_id[0][3]&0x01;
				}	
			
				locgcs++;
#if _INITD 
				pGuiServerExplorer->m_progress.SetPos((100*(lchannel*gldie+ldie))/(4*gldie));
#endif
	
		}
	}

	locgrdie=locgrdie/4;

	if(diepercs==0)
	{
		locgrdie=locgrdie/2;
	}
	grdie=locgrdie;


//	i=SLWFlashIDPos();
	i=0;
	if(flash_id[i][1] == 0xEC)
	{

		memcpy(flash_type.ID, &(flash_id[i][0]),4);
		if((flash_type.ID[2] & 0x03) == 0x01)
			flash_type.size_page = 2;
		else if((flash_type.ID[2] & 0x03) == 0x02)
			flash_type.size_page = 4;
		else if((flash_type.ID[2] & 0x03) == 0x03)
			flash_type.size_page = 8;
		else
		{
			AddInfo("ID ERROR");
			return false;
		}

		if((flash_type.ID[2] & 0x30) == 0x10)
			flash_type.size_block = 128;
		else if((flash_type.ID[2] & 0x30) == 0x20)
			flash_type.size_block = 256;
		else if((flash_type.ID[2] & 0x30) == 0x30)
			flash_type.size_block = 512;
		else
		{
			AddInfo("ID ERROR");
			return false;
		}

		if((flash_type.ID[3] & 0x0C) == 0x00)
		{
			flash_type.isMLC = FALSE;
			sectorperblock = 512;
		}
		else
		{
			flash_type.isMLC = TRUE;
			sectorperblock = 1024;
		}

		if(flash_type.ID[0] == 0xD3)
			flash_type.cap_in_gb = 1;
		else if(flash_type.ID[0] == 0xD5)
			flash_type.cap_in_gb = 2;
		else if(flash_type.ID[0] == 0xD7)
			flash_type.cap_in_gb = 4;
		else
		{
			AddInfo("ID ERROR");
			return false;
		}

		if(flash_type.ID[0] == 0xD5 && flash_type.isMLC==1 && GetDIEnumPerChip()/2==4) //特殊的HCG FLASH，ID与GAG一样，但是每个chip有4个DIE
			flash_type.cap_in_gb = 4;
	
		flash_type.blocknum = 4096; //三星的4种FLASH blockperdie都是4096

		//如果ID==0xD7,并且有每个chip有2个片选，说明是MLC 128G,所以有16个DIE,每个chip的容量是8G
		if(flash_type.isMLC==1 && flash_type.cap_in_gb==4&&(flash_id[4][0]==0xD7||flash_id[7][0]==0xD7||flash_id[13][0]==0xD7||flash_id[14][0]==0xD7))	
		{
			flash_type.cap_in_gb = 8;
			//如果ID==0xD7,并且有每个chip有4个片选，说明是MLC 256G,所以有32个DIE,每个chip的容量是16G
			if(gldie==32&&(flash_id[24][0]==0xD7||flash_id[25][0]==0xD7||flash_id[26][0]==0xD7||flash_id[27][0]==0xD7))
				flash_type.cap_in_gb = 16;

		}
		
		//如果ID==0xD5,并且有每个chip有2个片选，说明是SLC 64G,所以有16个DIE,每个chip的容量是4G
		if(flash_type.isMLC==0 && flash_type.cap_in_gb==2&&(flash_id[4][0]==0xD5||flash_id[7][0]==0xD5||flash_id[13][0]==0xD5||flash_id[14][0]==0xD5))	
		{
			flash_type.cap_in_gb = 4;
			//如果ID==0xD5,并且有每个chip有4个片选，说明是SLC 128G,所以有32个DIE,每个chip的容量是8G
			if(gldie==32&&(flash_id[24][0]==0xD5||flash_id[25][0]==0xD5||flash_id[26][0]==0xD5||flash_id[27][0]==0xD5))	
			{
				flash_type.cap_in_gb = 8;
			}
		}
	}
	else
	{
		j=FindFlashtypePos();
		if(j<FLASHtypeNum)
		{
			memcpy(flash_type.ID, &(flash_id[i][0]),4);	
			flash_type.size_page = 	FlashFeature[j].pagesize;
			flash_type.size_block = FlashFeature[j].blocksize;
			flash_type.isMLC = FlashFeature[j].Flashtype;
			sectorperblock = FlashFeature[j].blocksize*2;	
			flash_type.blocknum = FlashFeature[j].blockperdie;
			flash_type.cap_in_gb = FlashFeature[j].blockperdie*FlashFeature[j].blocksize*GetDIEnumPerChip()/(1024*1024);
			if(diepercs==0)
				flash_type.cap_in_gb=flash_type.cap_in_gb/2;
		}	
		else
		{
			AddInfo("ID is ERROR");
			return false;		
		}
	}

	flash_type.pages_per_block = flash_type.size_block / flash_type.size_page;
	flash_type.sectors_per_page = (flash_type.size_page * 1024) / 512;
	flash_type.sectors_per_block = flash_type.sectors_per_page * flash_type.pages_per_block;
	flash_type.total_blocks = (flash_type.cap_in_gb * 1024 * 1024) / flash_type.size_block;


	str.Format("FLASH ID: %02X %02X %02X %02X", flash_type.ID[1], flash_type.ID[0], flash_type.ID[3], flash_type.ID[2]);
	AddInfo(str);

	str.Format("Page size: %d  Block size: %d", flash_type.size_page, flash_type.size_block);
	AddInfo(str);

	str.Format("Pages Per Block: %d ", flash_type.pages_per_block);
	AddInfo(str);

	str.Format("Sectors Per Page: %d ", flash_type.sectors_per_page);
	AddInfo(str);

	str.Format("Total Block: %d ", flash_type.total_blocks);
	AddInfo(str);

	str.Format("TYPE MLC: %d ", flash_type.isMLC );
	AddInfo(str);

	str.Format("Chip Capactiy: %dG ", flash_type.cap_in_gb );
	AddInfo(str);
	
	for(k=0;k<DeviceCount;k++)
	{
		manufacture[k]="SAMSUNG";
		capacity[k].Format("%dGB",flash_type.cap_in_gb);
	}
	 
	//判断一个片选有几个DIE
	
	diepercs=flash_type.ID[3]&0x01;
	//判断4个channel的id是否相同,并得到DIEMAP
	for(ldie=0;ldie<gldie;ldie++)
	{

		//当每个片选只有一个die的时候，按照现行结构，一个channel只有4个die
		if(diepercs==0 && ldie>=16 ) 
			continue;

		idflag[0]=memcmp(flash_id[ldie],flash_id[32+ldie],4);
		idflag[1]=memcmp(flash_id[ldie],flash_id[64+ldie],4);
		idflag[2]=memcmp(flash_id[ldie],flash_id[96+ldie],4);
		if(idflag[0]||idflag[1]||idflag[2])
		{
			AddInfo("4个channel的FLASH ID不对称，不能初始化！");
			return 0;
		}
		int phpdie;
		//按照phy die num 排布图的顺序读物理die号，从die0 ~ die7 , die16 ~ die23 , die8 ~ die15 ,die24 ~ die31
		if(ldie<8)
			phpdie=ldie;
		else if(ldie<16&&ldie>=8)
			phpdie=ldie+8;
		else if(ldie<24&&ldie>=16)
			phpdie=ldie-8;
		else if(ldie<32&&ldie>=24)
			phpdie=ldie;
		
		if(flash_id[phpdie][1]==0xEC ||flash_id[phpdie][1]==0x89 || flash_id[phpdie][1] == 0x2C || flash_id[phpdie][1] == 0x98)
		{
			DIEMAP[dienum]=phpdie;
			dienum++;
			/*再次读出上次成功读出的FLASHID*/
			regs.bFeaturesReg		= SLW_CMD_VND_SUB_READFLASHID;	// send READFLASHID command
			regs.bSectorCountReg =  ldie;
			regs.bDriveHeadReg = 0;
			if( !PIO_VENDOR_OPS(device, true, buf, 0, &regs))
			{
				AddInfo("*READFLASHID command rejected*");
				return false;
			}
		}			
	}

	blocksize=flash_type.size_block/128;
	page4k=flash_type.size_page/2;

#if _INITD 
	pGuiServerExplorer->m_progress.SetPos(100);
	pGuiServerExplorer->m_progress.SetText("Read ID successfully");
	pGuiServerExplorer->m_progress.SetPos(0);
#endif
	
	struct tm *newtime;
	time_t aclock;
	time( &aclock );                 /* Get time in seconds */
	newtime = localtime( &aclock );  /* Convert time to struct */
                                    /* tm form */

	str.Format("Date : %s\r\nSN : %s\r\nFWFilename: %s\r\nFPGAFilename: %s\r\nLoadFilename: %s\r\n",
		asctime(newtime),BadBlockFilename[device],FWFilename,FPGAFilename,LoadFilename);
	LogInfo(device,str,1);
	str.Format("FLASH ID: %02X %02X %02X %02X", flash_type.ID[1], flash_type.ID[0], flash_type.ID[3], flash_type.ID[2]);
	LogInfo(device,str,1);

	str.Format("Page size: %d  Block size: %d", flash_type.size_page, flash_type.size_block);
	LogInfo(device,str,1);

	str.Format("Pages Per Block: %d ", flash_type.pages_per_block);
	LogInfo(device,str,1);

	str.Format("Sectors Per Page: %d ", flash_type.sectors_per_page);
	LogInfo(device,str,1);

	str.Format("Total Block: %d ", flash_type.total_blocks);
	LogInfo(device,str,1);

	str.Format("TYPE MLC: %d ", flash_type.isMLC );
	LogInfo(device,str,1);

	str.Format("Chip Capactiy: %dG ", flash_type.cap_in_gb );
	LogInfo(device,str,1);

	str.Format("Die per channel: %d ", locgrdie );
	AddInfo(str);
	if(BadBlockFilename[device].GetLength()>0)
		LogInfo(device,str,1);
	
	/*	判断是Intel 还是三星的flash*/
	if(flash_type.ID[1]==0x89)
		manufacture[device]="INTEL";
	else if(flash_type.ID[1]==0xEC)
		manufacture[device]="SAMSUNG";
	else if(flash_type.ID[1]==0x2C)
		manufacture[device]="Micron";
	else if(flash_type.ID[1]==0x98)
		manufacture[device]="Toshiba";

	if(diepercs==0)
		gcs=grdie;
	else
		gcs=grdie/2;
	str.Format("CS per channel: %d ", gcs );
	AddInfo(str);

	return true;
}

int GetDIEnumPerChip()
{
	int i,ldie;
	int dieperchip=0;
	int diepos[8];//记录每个chip中的8个die是否为空
	int tmp;

	memset(diepos,0,8*sizeof(int));
	for(i=0;i<128;i++)
	{
		if(flash_id[i][1]==0xEC ||flash_id[i][1]==0x89 || flash_id[i][1] == 0x2C || flash_id[i][1]==0x98 )
		{
			tmp=i%32;
			if(tmp>=0 && tmp<2)
				diepos[0]=1;
			if(tmp>=4 && tmp<8)
				diepos[1]=1;

			if(tmp>=8 && tmp<12)
				diepos[2]=1;
			if(tmp>=12 && tmp<16)
				diepos[3]=1;

			if(tmp>=16 && tmp<20)
				diepos[4]=1;
			if(tmp>=20 && tmp<24)
				diepos[5]=1;

			if(tmp>=24 && tmp<28)
				diepos[6]=1;
			if(tmp>=28 && tmp<32)
				diepos[7]=1;
		}
	}
			
	for(i=0;i<8;i++)
	{
		if(diepos[i])
			dieperchip++;
	}
	return dieperchip;
}

void ErrorExit(LPTSTR lpszFunction) 
{ 
    TCHAR szBuf[80]; 
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    wsprintf(szBuf, 
        "%s failed with error %d: %s", 
        lpszFunction, dw, lpMsgBuf); 
 
  //  MessageBox(NULL, szBuf, "Error", MB_OK); 
	AddInfo(szBuf);
    LocalFree(lpMsgBuf);

}

BOOL ata_via_scsi_pio(HANDLE hdevice, IDEREGS * regs, unsigned char * data, unsigned long datasize, bool IsWR)
{
	BOOL	bRet;
	DWORD	dwReturned;
	DWORD	length,errorcode;
	int tlength=0;
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptwb;

	::ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));

	if(hdevice == INVALID_HANDLE_VALUE)
	{
		return	FALSE;
	}
	
	
	if (datasize > (64 * 1024)) 
	{
		return FALSE;
	}

	if(datasize > 0)
	{
		tlength=2;
		if( ! IsWR)
		{
			sptwb.Spt.DataIn = SCSI_IOCTL_DATA_IN ;
		}
		else
		{
			sptwb.Spt.DataIn = SCSI_IOCTL_DATA_OUT;
		}
	}
	else
	{
		sptwb.Spt.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
		tlength=0;
	}

	sptwb.Spt.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptwb.Spt.PathId = 0;
	sptwb.Spt.TargetId = 0;
	sptwb.Spt.Lun = 0;
	sptwb.Spt.SenseInfoLength = 32;
	sptwb.Spt.DataTransferLength = datasize;
	sptwb.Spt.TimeOutValue = 2;
	sptwb.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);
	sptwb.Spt.DataBuffer = data;
	
	sptwb.Spt.CdbLength = 12;
	sptwb.Spt.Cdb[0] = 0xA1;
	if(datasize==0)
		sptwb.Spt.Cdb[1] = (0x3 << 1) | 0; //MULTIPLE_COUNT=0,Non-data=3,Reserved
	else
	{
		if(IsWR)
			sptwb.Spt.Cdb[1] = (0x5 << 1) | 0; //MULTIPLE_COUNT=0,PIO OUT=5,Reserved
		else
			sptwb.Spt.Cdb[1] = (0x4 << 1) | 0; //MULTIPLE_COUNT=0,PIO IN=4,Reserved
	}

	if(IsWR)
		sptwb.Spt.Cdb[2] = (1 << 5) | (0 << 3) | (1 << 2) | tlength;//OFF_LINE=0,CK_COND=1,Reserved=0,T_DIR=0(FromDevice),BYTE_BLOCK=1,T_LENGTH=2
	else
		sptwb.Spt.Cdb[2] = (1 << 5) | (1 << 3) | (1 << 2) | tlength;//OFF_LINE=0,CK_COND=1,Reserved=0,T_DIR=1(ToDevice),BYTE_BLOCK=1,T_LENGTH=2
	
	sptwb.Spt.Cdb[3] = regs->bFeaturesReg;//FEATURES (7:0)
	sptwb.Spt.Cdb[4] = regs->bSectorCountReg;//SECTOR_COUNT (7:0)
	sptwb.Spt.Cdb[5] = regs->bSectorNumberReg;//LBA_LOW (7:0)
	sptwb.Spt.Cdb[6] = regs->bCylLowReg;//LBA_MID (7:0)
	sptwb.Spt.Cdb[7] = regs->bCylHighReg;//LBA_HIGH (7:0)
	sptwb.Spt.Cdb[8] =(0x4<<4)  | regs->bDriveHeadReg;//DriveHeadReg (4:0)
	sptwb.Spt.Cdb[9] = regs->bCommandReg;//COMMAND

	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);

	bRet = ::DeviceIoControl(hdevice, IOCTL_SCSI_PASS_THROUGH_DIRECT, 
		&sptwb, length,
		&sptwb, length,	&dwReturned, FALSE);
	
	if(bRet == FALSE)
	{
		errorcode=GetLastError();
		AddInfo("Write error code 121");
//		return	FALSE;
	}

	regs->bCommandReg=sptwb.ucSenseBuf[21];
	regs->bDriveHeadReg=sptwb.ucSenseBuf[20];
    regs->bCylHighReg=sptwb.ucSenseBuf[19];
	regs->bCylLowReg=sptwb.ucSenseBuf[17];
	regs->bSectorNumberReg=sptwb.ucSenseBuf[15];
	regs->bSectorCountReg=sptwb.ucSenseBuf[13];
	regs->bFeaturesReg=sptwb.ucSenseBuf[11];
	return	TRUE;
}

BOOL ata_pass_through_ioctl_pio(int device, IDEREGS * regs, unsigned char * data, unsigned long datasize, bool IsWR)
{ 
	typedef struct 
	{
		ATA_PASS_THROUGH_EX apt;
		UCHAR ucDataBuf[64*1024];
	} ATA_PASS_THROUGH_EX_WITH_BUFFERS;
	
	CString str;

	if(InterfaceType[device]==1)
	{
		int ret;
		ret=ata_via_scsi_pio(Devicehandle[device],regs,data,datasize,IsWR);
		
	
		if (regs->bCommandReg== 0x51) 
		{
			str.Format("ATA Command ERROR: 0x%02X", regs->bCommandReg);
			LogInfo(device,str,3);
			return FALSE;
		}

		return ret;
	}
	else if(InterfaceType[device]==0)
	{
		ATA_PASS_THROUGH_EX_WITH_BUFFERS ab; 
		memset(&ab, 0, sizeof(ab));
		ab.apt.Length = sizeof(ATA_PASS_THROUGH_EX);
		ab.apt.TimeOutValue = 2;

		unsigned size = offsetof(ATA_PASS_THROUGH_EX_WITH_BUFFERS, ucDataBuf);
		ab.apt.DataBufferOffset = size;

		if (datasize > (64 * 1024)) 
		{
			return FALSE;
		}

		ab.apt.DataTransferLength = datasize;
		size += datasize;

		if(datasize > 0)
		{
			if( ! IsWR)
			{
				ab.apt.AtaFlags = ATA_FLAGS_DATA_IN ;
			}
			else
			{
				ab.apt.AtaFlags = ATA_FLAGS_DATA_OUT;
				memcpy(ab.ucDataBuf, data, datasize);
			}
			ab.apt.AtaFlags |= ATA_FLAGS_DRDY_REQUIRED;
		}
		else
			ab.apt.AtaFlags = ATA_FLAGS_DRDY_REQUIRED;


		ASSERT(sizeof(ab.apt.CurrentTaskFile) == sizeof(IDEREGS));
		IDEREGS * ctfregs = (IDEREGS *)ab.apt.CurrentTaskFile;
		*ctfregs = *regs;
		

		DWORD num_out;
		if (!DeviceIoControl(Devicehandle[device], IOCTL_ATA_PASS_THROUGH,
			&ab, size, &ab, size, &num_out, NULL)) 
		{
			if (GetLastError() == ERROR_IO_PENDING) 
			{
				if(num_out != size)
				{
					AddInfo("IO_PENDING finished,but read count is not match expect! ");
					*regs = *ctfregs;
					return FALSE;
				}
			}
			else
			{
				*regs = *ctfregs;
			
				return FALSE;
			}
		}
		
		// Check ATA status
		*regs = *ctfregs;
		if (ctfregs->bCommandReg != 0x50) 
		{
			str.Format("ATA Command ERROR: 0x%02X", ctfregs->bCommandReg);
			return FALSE;
		}
		
		// Check and copy data
		if (!IsWR && datasize) 
		{
			memcpy(data, ab.ucDataBuf, datasize);
		}
		
		return TRUE;
	}
}


BOOL ata_via_scsi_dma(HANDLE hdevice, IDEREGS * regs, unsigned char * data, unsigned long datasize, bool IsWR)
{
	BOOL	bRet;
	DWORD	dwReturned;
	DWORD	length,errorcode;
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptwb;

	::ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));

	if(hdevice == INVALID_HANDLE_VALUE)
	{
		return	FALSE;
	}
		
	if (datasize > (64 * 1024)) 
	{
		return FALSE;
	}

	if(datasize > 0)
	{
		if( ! IsWR)
		{
			sptwb.Spt.DataIn = SCSI_IOCTL_DATA_IN ;
		}
		else
		{
			sptwb.Spt.DataIn = SCSI_IOCTL_DATA_OUT;
		}
		
	}


	sptwb.Spt.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptwb.Spt.PathId = 0;
	sptwb.Spt.TargetId = 0;
	sptwb.Spt.Lun = 0;
	sptwb.Spt.SenseInfoLength = 32;
	sptwb.Spt.DataTransferLength = datasize;
	sptwb.Spt.TimeOutValue = 2;
	sptwb.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);
	sptwb.Spt.DataBuffer = data;
	
	sptwb.Spt.CdbLength = 12;
	sptwb.Spt.Cdb[0] = 0xA1;
	sptwb.Spt.Cdb[1] = (0x6 << 1) | 0; //MULTIPLE_COUNT=0,DMA=6,Reserved
	if(IsWR)
		sptwb.Spt.Cdb[2] =(1 << 5) | (0 << 3) | (1 << 2) | 2;//OFF_LINE=0,CK_COND=1,Reserved=0,T_DIR=0(FromDevice),BYTE_BLOCK=1,T_LENGTH=2
	else
		sptwb.Spt.Cdb[2] =(1 << 5) | (1 << 3) | (1 << 2) | 2;//OFF_LINE=0,CK_COND=1,Reserved=0,T_DIR=1(ToDevice),BYTE_BLOCK=1,T_LENGTH=2
	sptwb.Spt.Cdb[3] = regs->bFeaturesReg;//FEATURES (7:0)
	sptwb.Spt.Cdb[4] = regs->bSectorCountReg;//SECTOR_COUNT (7:0)
	sptwb.Spt.Cdb[5] = regs->bSectorNumberReg;//LBA_LOW (7:0)
	sptwb.Spt.Cdb[6] = regs->bCylLowReg;//LBA_MID (7:0)
	sptwb.Spt.Cdb[7] = regs->bCylHighReg;//LBA_HIGH (7:0)
	sptwb.Spt.Cdb[8] =(0x4<<4)  | regs->bDriveHeadReg;//DriveHeadReg (4:0)
	sptwb.Spt.Cdb[9] = regs->bCommandReg;//COMMAND

	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);

	bRet = ::DeviceIoControl(hdevice, IOCTL_SCSI_PASS_THROUGH_DIRECT, 
		&sptwb, length,
		&sptwb, length,	&dwReturned, FALSE);
	
	if(bRet == FALSE)
	{
		errorcode=GetLastError();
		AddInfo("Write error code 121");
//		return	FALSE;
	}
	regs->bCommandReg=sptwb.ucSenseBuf[21];
	regs->bDriveHeadReg=sptwb.ucSenseBuf[20];
    regs->bCylHighReg=sptwb.ucSenseBuf[19];
	regs->bCylLowReg=sptwb.ucSenseBuf[17];
	regs->bSectorNumberReg=sptwb.ucSenseBuf[15];
	regs->bSectorCountReg=sptwb.ucSenseBuf[13];
	regs->bFeaturesReg=sptwb.ucSenseBuf[11];

	return	TRUE;
}



BOOL ata_pass_through_ioctl_DMA(int device, IDEREGS * regs, unsigned char * data, unsigned long datasize, bool IsWR)
{ 
	int ret;
	if(datasize==0)
	{
		regs->bCommandReg = 0x50;
		return TRUE;
	}
	
	if(InterfaceType[device]==1)
	{
		int ret;
		ret=ata_via_scsi_dma(Devicehandle[device],regs,data,datasize,IsWR);
		return ret;
	}
	else if(InterfaceType[device]==0)
	{
		ATA_PASS_THROUGH_DIRECT ab; 
		memset(&ab, 0, sizeof(ab));
		ab.Length = sizeof(ATA_PASS_THROUGH_DIRECT);
		ab.TimeOutValue = 10;
		ab.DataBuffer = (PVOID)data;	
		ab.DataTransferLength = datasize;

		if ( ! IsWR) 
		{
			ab.AtaFlags = ATA_FLAGS_DATA_IN | ATA_FLAGS_USE_DMA;
		}
		else
		{
			ab.AtaFlags = ATA_FLAGS_DATA_OUT | ATA_FLAGS_USE_DMA;
		}
		ab.AtaFlags |= ATA_FLAGS_DRDY_REQUIRED;
		
		CString str;
		ASSERT(sizeof(ab.CurrentTaskFile) == sizeof(IDEREGS));
		IDEREGS * ctfregs = (IDEREGS *)ab.CurrentTaskFile;
		*ctfregs = *regs;
		
		DWORD num_out;
		ret = DeviceIoControl(Devicehandle[device], IOCTL_ATA_PASS_THROUGH_DIRECT,&ab, sizeof(ATA_PASS_THROUGH_DIRECT), &ab, sizeof(ATA_PASS_THROUGH_DIRECT), &num_out, NULL);
		if(ret<1)
		{
			*regs = *ctfregs;
			return FALSE;
		}

		*regs = *ctfregs;
		if (ctfregs->bCommandReg != 0x50) 
		{
			str.Format("ATA Command 0x%02X failed!",ctfregs->bCommandReg);
			AddInfo(str);
			return FALSE;
		}
		return TRUE;
	}
}

BOOL ata_pass_through_ioctl_DMAEXT(HANDLE hdevice, IDEREGS * regs,IDEREGS * regspre, unsigned char * data, unsigned long datasize, bool IsWR)
{
	int ret;

	ATA_PASS_THROUGH_DIRECT ab; 
	memset(&ab, 0, sizeof(ab));
	ab.Length = sizeof(ATA_PASS_THROUGH_DIRECT);
	ab.TimeOutValue = 10;
	ab.DataBuffer = (PVOID)data;	
	ab.DataTransferLength = datasize;
    
	if ( ! IsWR) 
	{
		ab.AtaFlags = ATA_FLAGS_DATA_IN | ATA_FLAGS_USE_DMA | ATA_FLAGS_48BIT_COMMAND;

	}
	else
	{
		ab.AtaFlags = ATA_FLAGS_DATA_OUT | ATA_FLAGS_USE_DMA | ATA_FLAGS_48BIT_COMMAND;
	}
	ab.AtaFlags |= ATA_FLAGS_DRDY_REQUIRED;
	
	CString str;
	ASSERT(sizeof(ab.CurrentTaskFile) == sizeof(IDEREGS));
	ASSERT(sizeof(ab.PreviousTaskFile) == sizeof(IDEREGS));
	IDEREGS * ctfregs = (IDEREGS *)ab.CurrentTaskFile;
	*ctfregs = *regs;
	IDEREGS * ctfregs1 = (IDEREGS *)ab.PreviousTaskFile;
	*ctfregs1 = *regspre;


	DWORD num_out;
	ret = DeviceIoControl(hdevice, IOCTL_ATA_PASS_THROUGH_DIRECT,&ab, sizeof(ATA_PASS_THROUGH_DIRECT), &ab, sizeof(ATA_PASS_THROUGH_DIRECT), &num_out, NULL);
	if(ret<1)
	{
		*regs = *ctfregs;
		ErrorExit("IOCTL_ATA_PASS_THROUGH_DIRECT");
		return FALSE;

	}
	*regs = *ctfregs;
	if (ctfregs->bCommandReg != 0x50) 
	{
		str.Format("ATA Command 0x%02X failed!",ctfregs->bCommandReg);
	//	AfxMessageBox(str);
		return FALSE;
	}
	
	return TRUE;
}

BOOL SLWIDFY(int device,unsigned char* CSWBuffer)
{
	CString drive;
	if(! IDFY_IDE(drive, CSWBuffer,device) )
	{
		AddInfo("****IDFY FAILED!!!\r\n");
		return 0;
	}

#if _INITD 
	pDataView->m_edhex.SetContent(CSWBuffer, 512);
#endif
	return 1;
}

int SLWGetInterfaceflag(int device)
{
	unsigned char CSWBuffer[512];
	int ret;

	if(SLWIDFY(device,CSWBuffer))
	{
		ret=CSWBuffer[52]&0x10;	
		return ret;
	}
	else
		return -1;
}

BOOL IDFY_IDE(LPCTSTR driveName,BYTE *idfy,int device)
{
	BOOL ret;
	IDEREGS regs;

	if(	InterfaceType[device]>0)
	{
		ret=DoIdentifyDeviceSat(device,0,idfy);
		return ret;
	}
	else
	{	
		regs.bFeaturesReg=0x00;
		regs.bSectorCountReg=0x01;
		regs.bSectorNumberReg=0;
		regs.bCylLowReg=0;
		regs.bCylHighReg=0x0;	
		regs.bDriveHeadReg=0x00;	// 0xE0
		regs.bCommandReg=0xEC;		// IDFY
			
		DWORD  cbBytesReturned = 0;
		
		if( !ata_pass_through_ioctl_pio(device, &regs, idfy, 512, false) )
		{
			CloseHandle(Devicehandle[device]);
			Devicehandle[device]=INVALID_HANDLE_VALUE;
			return FALSE;
		}

		if ( regs.bCommandReg!= 0x50 )//success
		{
	//		return FALSE;
			;
		}
		
		return TRUE;
	}
}

BOOL Write_Disk_IDE(LPCTSTR drivename, BYTE *buf, DWORD *rebyte, int *pLBA)	// write 1GB, buf should be 128KB
{
	CString str;
	DWORD rbytes;
	DWORD re_LBA;
	long filept = (long)pLBA*512;


	HANDLE handle = CreateFile (drivename,
								GENERIC_READ | GENERIC_WRITE, 
								FILE_SHARE_READ , NULL,
								OPEN_EXISTING, 0, NULL);
	
	if (handle == INVALID_HANDLE_VALUE)
	{
	//	str.Format("不能打开设备 %s！",drivename);
	//	AfxMessageBox(str);
		return FALSE;
	}

	SetFilePointer (handle,(long)filept, NULL, FILE_BEGIN );

	DWORD i;
	for( i = 0; i < (1 * 1024 * 1024 / 128); i++)
	{
		rbytes = 0;
		if( !WriteFile(handle, buf, 128 * 1024, &rbytes, NULL))
		{
			re_LBA = i * 128 * 1024 + rbytes;
			*rebyte = re_LBA;
			CloseHandle(handle);
			return FALSE;
		}
	}
	re_LBA = i * 128 * 1024 + rbytes;
	*rebyte = re_LBA;

	CloseHandle(handle);
	return TRUE;
}

BOOL Read_Disk_IDE(LPCTSTR drivename, BYTE *buf, DWORD *rebyte, int *pLBA)	// write 1GB, buf should be 128KB
{
	CString str;
	DWORD rbytes;
	DWORD re_LBA;
	long filept = (long)pLBA*512;


	HANDLE handle = CreateFile (drivename,
								GENERIC_READ | GENERIC_WRITE, 
								FILE_SHARE_READ , NULL,
								OPEN_EXISTING, 0, NULL);
	
	if (handle == INVALID_HANDLE_VALUE)
	{
	//	str.Format("不能打开设备 %s！",drivename);
	//	AfxMessageBox(str);
		return FALSE;
	}

	SetFilePointer (handle,(long)filept, NULL, FILE_BEGIN );

	DWORD i;
	for( i = 0; i < (1 * 1024 * 1024 / 128); i++)
	{
		rbytes = 0;
		if( !ReadFile(handle, buf, 128 * 1024, &rbytes, NULL))
		{
			re_LBA = i * 128 * 1024 + rbytes;
			*rebyte = re_LBA;
			CloseHandle(handle);
			return FALSE;
		}
	}
	re_LBA = i * 128 * 1024 + rbytes;
	*rebyte = re_LBA;

	CloseHandle(handle);
	return TRUE;
}

void reversedstr(BYTE *buffer, int count)  //高低字节交换
{
	BYTE ch;
	for(int i = 0; i < count; i++)
	{
		ch = buffer[i];
		buffer[i] = buffer[i+1];
		buffer[i+1] = ch;
		i++;
	}
}

BOOL CHK_DISK(int device,LPCTSTR driver)
{
	BYTE buf[512];
	CString name,str;
	int ldev=0;
	for(int i = 0; i < 36; i++)
	{
		name.Format("\\\\.\\PhysicalDrive%d", i);

		if( IDFY_IDE(name, buf,device) )
		{
			if( (buf[54] == 'L' || buf[54] == 'o') && buf[55] == 'S')
			{
		//		str.Format("Description is %s %s",&buf[54],&buf[55]);
		//		AddInfo(str);
				if(ldev==device)
				{
					strcpy((char *)driver, name.GetBuffer(1));
					return TRUE;
				}
				ldev++;
			}

		}
	}
	return FALSE;
}
/*
BOOL DMAWriteAll( int device, DWORD total, DWORD dLBA,int sector)
{
	CString drive;
	DWORD i,j;
	DWORD num;
	int k,remainder;
	if(FindSSD()==0)
	{
		AddInfo("Can not find SSD device!!!");
		return 0;
	}
	if(total%sector>0)
		remainder=total%sector;

	num=total/sector;

	int datasize = sector*512;	
	BYTE *buf;
	buf = new BYTE [datasize];


	for(i=0;i<num;i++)
	{
		for(j=0;j<sector;j++)
		{
			for(k=0;k<512;k++)
				buf[j*512 +k]=0;

		}
		if(DMA_OPS(device,buf,sector*512,dLBA+i*sector,true, ata_WRITE_DMA))
		{
#if _INITD 
			pFormMenoa->m_progress.SetPos(100*i/num);		
#endif
		}		
		else 
		{
			delete [datasize] buf;
			CloseSSDHandle();
			return 0;
		}
	}
	for(j=0;j<remainder;j++)
	{
		for(k=0;k<512;k++)
			buf[j*512 +k]=0xFF;

	}
	if(!DMA_OPS(device,buf,remainder*512,dLBA+i*sector,true, ata_WRITE_DMA))
	{		
		delete [datasize] buf;
		CloseSSDHandle();
		return 0;
		
	}
	CloseSSDHandle();
	delete [datasize] buf;
	return 1;
}
*/

BOOL DMAWriteAll( int device, DWORD total, DWORD dLBA,int sector)
{
	CString drive;
	DWORD i,j;
	DWORD num;

	if(FindSSD()==0)
	{
		AddInfo("Can not find SSD device!!!");
		return 0;
	}
	int remainder=0;

	if(total%sector>0)
		remainder=total%sector;

	num=total/sector;

	int datasize = sector*512;	
	BYTE *buf;
	buf = new BYTE [datasize];


	for(i=0;i<num;i++)
	{
		if(FlagStop==1)
			break;
		for(j=0;j<sector;j++)
		{
			buf[j*512 +3]=(unsigned char)((dLBA+j+i*sector)>>24);
			buf[j*512 +2]=(unsigned char)((dLBA+j+i*sector)>>16);
			buf[j*512 +1]=(unsigned char)((dLBA+j+i*sector)>>8);
			buf[j*512]=(unsigned char)((dLBA+j+i*sector)>>0);
		}
		if(DMA_OPS(device,buf,sector*512,dLBA+i*sector,true, ata_WRITE_DMA))
		{
#if _INITD 
			pFormMenoa->m_progress.SetPos(100*i/num);		
#endif
		}		
		else 
		{	
			return 0;
		}
	}
	if(remainder>0)
	{
		for(j=0;j<remainder;j++)
		{
			buf[j*512 +3]=(unsigned char)((dLBA+j+i*sector)>>24);
			buf[j*512 +2]=(unsigned char)((dLBA+j+i*sector)>>16);
			buf[j*512 +1]=(unsigned char)((dLBA+j+i*sector)>>8);
			buf[j*512]=(unsigned char)((dLBA+j+i*sector)>>0);

		}
		if(!DMA_OPS(device,buf,remainder*512,dLBA+i*sector,true, ata_WRITE_DMA))
		{		
			delete [datasize] buf;
			return 0;
			
		}
	}
	delete [datasize] buf;
	return 1;
}


BOOL DMA_HANDLE( int device,BYTE *buf, DWORD sector, DWORD dLBA, bool IsWR, BYTE CMD,HANDLE handle )
{
	IDEREGS regs;
	regs.bFeaturesReg		= 0x00;
	regs.bSectorCountReg	= sector;
	regs.bSectorNumberReg	= BYTE( (dLBA >> 0) & 0xFF);
	regs.bCylLowReg			= BYTE( (dLBA >> 8) & 0xFF);
	regs.bCylHighReg		= BYTE( (dLBA >> 16) & 0xFF);	
	regs.bDriveHeadReg		= 0x40 | BYTE( (dLBA >> 24) & 0x0F);	// LBA mode
	regs.bCommandReg		= CMD;									// WRITE DMA
	if(Devicehandle[device]==INVALID_HANDLE_VALUE)	
		return 0;
	if( !ata_pass_through_ioctl_DMA(device, &regs, buf, sector*512, IsWR) )
	{
		AddInfo("ata_pass_through_ioctl_DMA Failed !");
		CloseHandle(Devicehandle[device]);
		Devicehandle[device]=INVALID_HANDLE_VALUE;
		return FALSE;
	}

	if ( regs.bCommandReg!= 0x50 )//success
	{
		return FALSE;
	}
	return TRUE;
}


BOOL DMA_OPS(int device, BYTE *buf, DWORD bytes, DWORD dLBA, bool IsWR, BYTE CMD)
{
	IDEREGS regs;
	BYTE tmpbuf[512*256];
	DWORD tmplen;
	int sectornum = bytes / 512;
	int i;
	sectornum = sectornum;
	int loopnum = sectornum / 129 + 1 ;

	//根据需要读取的扇区长度，由于在xp系统下，每次硬盘驱动数据传输的长度不能超过128k,分解执行dma 命令，每128个扇区执行一次dma命令
	for (i = 0;i <loopnum; i++)
	{
		memset(tmpbuf,0,512*256);
		memset(&regs,0,sizeof(IDEREGS));

		if (sectornum > 128)
		{
			sectornum = sectornum - 128;
			regs.bSectorCountReg	= 128 ;
			tmplen = 128*512;
			
		}
		else
		{
			regs.bSectorCountReg	= BYTE(sectornum);
			tmplen = sectornum * 512;
		}
		if (IsWR) 
			memcpy(tmpbuf,buf,tmplen);

		regs.bFeaturesReg		= 0x00;
		regs.bSectorNumberReg	= BYTE( (dLBA >> 0) & 0xFF);
		regs.bCylLowReg			= BYTE( (dLBA >> 8) & 0xFF);
		regs.bCylHighReg		= BYTE( (dLBA >> 16) & 0xFF);	
		regs.bDriveHeadReg		= 0x40 | BYTE( (dLBA >> 24) & 0x0F);	// LBA mode						
		regs.bCommandReg		= CMD;									

		if( !ata_pass_through_ioctl_DMA(device, &regs, tmpbuf, tmplen, IsWR) )
		{
			AddInfo("ata_pass_through_ioctl_DMA Failed !");
			CloseHandle(Devicehandle[device]);					
			Devicehandle[device]=INVALID_HANDLE_VALUE;
			return FALSE;
		}

		if ( !IsWR) 
			memcpy(buf,tmpbuf,tmplen);
	
		buf = buf + tmplen;	
		dLBA = dLBA + 128;
	}

	if ( regs.bCommandReg== 0x51 )//success
	{
		return FALSE;
	}
	return TRUE;
}

/*
BOOL DMA_OPS( BYTE *buf, DWORD bytes, DWORDLONG dLBA, bool IsWR, BYTE CMD)
{
	IDEREGS regs,regspre;
    BYTE tmpbuf[512*256];
	DWORD tmplen;
	HANDLE handle ;
	int sectornum = bytes / 512;
	int i;
	sectornum = sectornum;
	int loopnum = sectornum / 251 + 1 ;

	//根据需要读取的扇区长度，由于在xp系统下，每次硬盘驱动数据传输的长度不能超过128k,分解执行dma 命令，每250个扇区执行一次dma命令
	for (i = 0;i <loopnum; i++)
	{
		memset(tmpbuf,0,512*256);
		memset(&regs,0,sizeof(IDEREGS));
		memset(&regspre,0,sizeof(IDEREGS));
		if (sectornum > 250)
		{
			sectornum = sectornum - 250;
			regs.bSectorCountReg	= 0xFA ;
			tmplen = 250*512;
			
		}
		else
		{
			regs.bSectorCountReg	= BYTE(sectornum);
			tmplen = sectornum * 512;
		}
		if (IsWR) 
			memcpy(tmpbuf,buf,tmplen);

		regs.bFeaturesReg		= 0x00;
		regs.bSectorNumberReg	= BYTE( (dLBA >> 0) & 0xFF);
		regs.bCylLowReg			= BYTE( (dLBA >> 8) & 0xFF);
		regs.bCylHighReg		= BYTE( (dLBA >> 16) & 0xFF);	
		regs.bDriveHeadReg		= 0x40;									
		regs.bCommandReg		= CMD;									
		regs.bReserved			= 0x00;

		regspre.bFeaturesReg		= 0x00;
		regspre.bSectorCountReg		= 0x00;
		regspre.bSectorNumberReg	= BYTE( (dLBA >> 24) & 0xFF);
		regspre.bCylLowReg			= BYTE( (dLBA >> 32) & 0xFF);
		regspre.bCylHighReg			= BYTE( (dLBA >> 40) & 0xFF);	
		regspre.bDriveHeadReg		= 0x40;									
		regspre.bCommandReg			= CMD;									
		regspre.bReserved			= 0x00;
			
		//  Windows NT, Windows 2000, must have admin rights
		handle = CreateFile( drive,
							GENERIC_READ | GENERIC_WRITE, 
							0, NULL,
							OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		
		if (handle == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}
		
		if( !ata_pass_through_ioctl_DMAEXT(handle, &regs,&regspre, tmpbuf, tmplen, IsWR) )
		{
	//		AfxMessageBox("ata_pass_through_ioctl_DMA Failed !");
			CloseHandle(handle);
			return FALSE;
		}
		if ( !IsWR) 
			memcpy(buf,tmpbuf,tmplen);
	
		buf = buf + tmplen;
		CloseHandle(handle);
		dLBA = dLBA + 250;
	}

	if ( regs.bCommandReg!= 0x50 )//success
	{
		return FALSE;
	}
	return TRUE;
}
*/

BOOL PIO_OPS(int device, BYTE *buf, DWORD bytes, DWORD dLBA, bool IsWR, BYTE CMD)
{
	IDEREGS regs;
	regs.bFeaturesReg		= 0x00;
	regs.bSectorCountReg	= BYTE(bytes / 512);
	regs.bSectorNumberReg	= BYTE( (dLBA >> 0) & 0xFF);
	regs.bCylLowReg			= BYTE( (dLBA >> 8) & 0xFF);
	regs.bCylHighReg		= BYTE( (dLBA >> 16) & 0xFF);	
	regs.bDriveHeadReg		= BYTE(( (dLBA >> 24) & 0x0F)|0xE0);	// LBA mode
	regs.bCommandReg		= CMD;							// WRITE CMD
	
	if( !ata_pass_through_ioctl_pio(device, &regs, buf, bytes, IsWR) )
	{
		CloseHandle(Devicehandle[device]);
		Devicehandle[device]=INVALID_HANDLE_VALUE;
		return FALSE;
	}

	if ( regs.bCommandReg!= 0x50 )//success
	{
		return FALSE;
	}
	return TRUE;
}

BOOL PIO_VENDOR_OPS(int device, bool IsWR, BYTE *buf, DWORD bytes, IDEREGS *regs)
{
	regs->bCommandReg = SLW_ATA_CMD_VNDNDCMD;
	if( !ata_pass_through_ioctl_pio(device, regs, buf, bytes, IsWR) )
	{
		CloseHandle(Devicehandle[device]);
		Devicehandle[device]=INVALID_HANDLE_VALUE;
		return FALSE;
	}
	return TRUE;
}

BOOL SLW_SSD_DBG(int device,bool enter)
{
	IDEREGS regs;
	if(enter)
	{
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_ENDBG;
		regs.bSectorCountReg	= 0;
		regs.bSectorNumberReg	= 0x53;
		regs.bCylLowReg			= 0;
		regs.bCylHighReg		= 0;	
		regs.bDriveHeadReg		= 0;	
		regs.bCommandReg		= SLW_ATA_CMD_VNDNDCMD;	
	}
	else
	{
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_ENDBG;
		regs.bSectorCountReg	= 0;
		regs.bSectorNumberReg	= 'W';
		regs.bCylLowReg			= 0;
		regs.bCylHighReg		= 0;	
		regs.bDriveHeadReg		= 0;	
		regs.bCommandReg		= SLW_ATA_CMD_VNDNDCMD;	
	}
		
	if( !ata_pass_through_ioctl_pio(device, &regs, NULL, NULL, true) )
	{
		AddInfo("ata_pass_through_ioctl_pio Failed !");
		CloseHandle(Devicehandle[device]);
		Devicehandle[device]=INVALID_HANDLE_VALUE;
		return FALSE;
	}
	return TRUE;
}

BOOL SLWFindBadblock(int device)
{
	CString str;
	unsigned char DATABuffer[512];
	BYTE buf[512];
	int i;
	int j;
		
	int m_datasize=512;	//512
	int datasize=512;	//512	
	int sectorcount=1;	//1 sector
	int irow,ichannel;
	int p1,p2;

	for(i=0;i<datasize;i++)
	{
		DATABuffer[i]=0xff;		
	}

	DWORD page;	
	int num;
	for(i=0;i<channel*gldie;i++)
		xblock[device][i]=0;

/*根据FLASH手册，MLC的查找原始坏块是找每个block的最后一个page的byte 4096(即sector 1023 的offset in sector byte 395),
   SLC是找第一个page的 byte 4096(即sector 7 的 offset in sector byte 395)
*/
	if( flash_type.isMLC==1)		
		VNDSetBlockArg(device,flash_type.sectors_per_block-1,1,0);
	else
		VNDSetBlockArg(device,7,1,0);

	for(i=0;i<channel*gldie;i++)
	{
		ichannel = i/gldie;
		irow = DIEMAP[i%gldie];
#if _MASSINIT
		str.Format("%d",80*i/(4*gldie));
		str+="%";
		pSoliWareMPToolDlg->m_GridCtrl.SetItemText(device+1,2,str);
		pSoliWareMPToolDlg->m_GridCtrl.Refresh();
#endif
		if(irow==65535)
			continue;
		
		p1=SLWFlashIDPos();

		if(flash_id[p1][1] == 0xEC)
		{
			for(j=0; j <4096;j++)
			{	
				if( (irow>7&&irow<16) || (irow>23&&irow<32) )
					num=j+4096;
				else
					num=j;
				
				if(flash_type.isMLC==1)
					page = flash_type.pages_per_block*num+flash_type.pages_per_block -1;
				else
					page = flash_type.pages_per_block*num;

				if(VNDReadSector(device,buf,ichannel,irow,page,4)==0)
				{
				
					str.Format("___FindBadblock fail at:   Channel %d DIE %d block %d", ichannel, irow,num);
					AddInfo(str);
					return 0;
				}
				else
				{
					if(buf[511]!=0xff)			
					{
						badblock[device][i+irow-i%gldie][xblock[device][i+irow-i%gldie]] = num;   //badblock的第二维对应逻辑die,所以要转换为i+m_row-i%16
						xblock[device][i+irow-i%gldie]++;
					}

				}		
			}
		}
		else
		{
			p2=FindFlashtypePos();
			if(p2<FLASHtypeNum)
			{
				for(j=0; j <FlashFeature[p2].blockperdie;j++)
				{	
					if( (irow>7&&irow<16) || (irow>23&&irow<32) )
						num=j+FlashFeature[p2].blockperdie;
					else
						num=j;
					
	
					page = flash_type.pages_per_block*num+FlashFeature[p2].PageMark1;
			
					if(VNDReadSector(device,buf,ichannel,irow,page,7)==0)
					{
					
						str.Format("___FindBadblock fail at:   Channel %d DIE %d block %d", ichannel, irow,num);
						AddInfo(str);
						return 0;
					}
					else
					{
						if(buf[359]!=0xff)			
						{
							badblock[device][i+irow-i%gldie][xblock[device][i+irow-i%gldie]] = num;   //badblock的第二维对应逻辑die,所以要转换为i+m_row-i%16
							xblock[device][i+irow-i%gldie]++;
						}

					}		
				}
			}
		}
		str.Format("channel %d die %d 坏块个数 %d\r\n", ichannel,irow, xblock[device][i+irow-i%gldie]);
		AddInfo(str);		
#if _INITD 
		pGuiServerExplorer->m_progress.SetPos(i*100/(4*gldie));
#endif

	}

	int errorflag=0;	
	for(i=0;i<128;i++)
	{
		for(j=0;j<xblock[device][i];j++)
		{
			//每个DIE的block 0 不能为坏块，用作坏块保留表
			//每个DIE 0 的 block 1 , block 3不能为坏块，用作映射表
			if(badblock[device][i][j]==0 ||  (badblock[device][i][j]==1 && i%gldie==0)  || (badblock[device][i][j]==3 && i%gldie==0))
			{
				errorflag=1;				
				str.Format("Channel: %d die: %d  block: %d 块为坏块\r\n",(i/gldie),i%gldie,badblock[device][i][j]);
				AddInfo(str);		
				LogInfo(device,str,3);
			}
		}
	}
	if(errorflag==1)
	{					
		return 0;
	}
	
	int a=0;
	int b=0;
	int c=0;
	int d=0;
	for(i=0; i < channel*gldie; i++)
	{
		a=0;
		b=0;
		c=0;
		d=0;
		if(step == 1)
		{
			for(j=0;j<xblock[device][i];j++)
			{
				if((badblock[device][i][j]>=0)&&(badblock[device][i][j]<2048))
				{
					a++;
				}
				else if((badblock[device][i][j]>=2048)&&(badblock[device][i][j]<2048*2))
				{
					b++;
				}
				else if((badblock[device][i][j]>=2048*2)&&(badblock[device][i][j]<2048*3))
				{
					c++;
				}
				else if((badblock[device][i][j]>=2048*3)&&(badblock[device][i][j]<2048*4))
				{
					d++;
				}
			}
			if(a>50)
			{
				errorflag=1;
				str.Format("Channel %d row %d layer %d  plane 0 坏块>50\r\n",(i%4),((i/4)&0x03),(i/16));
				AddInfo(str);
					
			}
			if(b>50)
			{
				errorflag=1;
				str.Format("Channel %d row %d layer %d  plane 1 坏块>50\r\n",(i%4),((i/4)&0x03),(i/16));
				AddInfo(str);
					
			}
			if(c>50)
			{
				errorflag=1;
				str.Format("Channel %d row %d layer %d  plane 2 坏块>50\r\n",(i%4),((i/4)&0x03),(i/16));
				AddInfo(str);
					
			}
			if(d>50)
			{
				errorflag=1;
				str.Format("Channel %d row %d layer %d  plane 3 坏块>50\r\n",(i%4),((i/4)&0x03),(i/16));
				AddInfo(str);
					
			}
		}
		else if(step == 2)
		{
			for(j=0;j<xblock[device][i];j++)
			{
				if(((badblock[device][i][j]%2)==0)&&(badblock[device][i][j]<4096))
				{
					a++;
				}
				else if(((badblock[device][i][j]%2)!=0)&&(badblock[device][i][j]<4096))
				{
					b++;
				}
				else if(((badblock[device][i][j]%2)==0)&&(badblock[device][i][j]>=4096))
				{
					c++;
				}
				else if(((badblock[device][i][j]%2)!=0)&&(badblock[device][i][j]>=4096))
				{
					d++;
				}
			}
			if(a>50)
			{
				errorflag=1;
				str.Format("Channel %d die %d plane 0 坏块>50\r\n",(i/gldie),DIEMAP[i%gldie]);
				AddInfo(str);
					
			}
			if(b>50)
			{
				errorflag=1;
				str.Format("Channel %d die %d plane 1 坏块>50\r\n",(i/gldie),DIEMAP[i%gldie]);
				AddInfo(str);
					
			}
			if(c>50)
			{
				errorflag=1;
				str.Format("Channel %d die %d  plane 0 坏块>50\r\n",(i/gldie),DIEMAP[i%gldie]);
				AddInfo(str);
					
			}
			if(d>50)
			{
				errorflag=1;
				str.Format("Channel %d die %d  plane 1 坏块>50\r\n",(i/gldie),DIEMAP[i%gldie]);
				AddInfo(str);
					
			}
		}		
	}
	if(errorflag==1)
	{				
		return 0;
	}
	return 1;
}

BOOL SLWReadResult(int device)
{
	FILE *fp;
	int a;
	int i,j;
	char output[128];
	CString str;
	int loc,ldie,lchannel;

	CString strDldFile="";	
#if _INITD 
	CFileDialog dlgLoad(
		TRUE, 0, 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"TxtFiles (*.bck)|*.bck||");
	dlgLoad.m_ofn.lpstrTitle = "选择原始坏块表";
	dlgLoad.m_ofn.lpstrInitialDir = strDldFile; // OK if you specify a file
	if(dlgLoad.DoModal() != IDOK)
	{
		AddInfo("****ReadResult失败!!!");
						
		return 0;		
	}		
	strDldFile = dlgLoad.m_ofn.lpstrFile;
	BadBlockFilename[device]=strDldFile;
	SelectBadblockfile=strDldFile;
#endif

#if _MASSINIT 
	char filename[128];
	GetModuleFileName(NULL,filename,128); 
	(strrchr(filename,'\\'))[1] = 0; 
//	BadBlockFilename[device]+=".bck";
	strDldFile=filename;
	strDldFile+="BadblockFile\\";
	strDldFile+=BadBlockFilename[device];
	strDldFile+=".bck";
#endif

	fp = fopen(strDldFile, "rb");
	if(fp==NULL)
	{
		AfxMessageBox("Open read file fail!");
		return 0;
	}
	
	for(i=0;i<8;i++)
	{
		fscanf(fp,"%s ",output);
	}

	fscanf(fp,"%d",&a);
/*	if(a!=4)
	{
		AddInfo("\r\n\r\n****channel设置不正确!!!\r\n\r\n");
		fclose(fp);
		return 0;
	}
*/
	fscanf(fp,"%s ",output);
	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&a);
	if(a!=grdie)
	{
		AddInfo("\r\n\r\n****die设置不正确!!!\r\n\r\n");
		fclose(fp);
		return 0;
	}

	fscanf(fp,"%s ",output);
	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&a);
/*	if(a!=plane)
	{
		AddInfo("\r\n\r\n****plane设置不正确!!!\r\n\r\n");
		fclose(fp);
		return 0;
	}
*/
	for(i=0;i<channel*grdie;i++)
	{
		for(j=0;j<6;j++)
		{
			fscanf(fp,"%s ",output);
			str=output;	
			if(j==1)
				ldie=atol(output);
			if(j==3)
				lchannel=atol(output);
			if(str=="End!")
			{
				AddInfo("\r\n\r\n****文件格式不正确!!!\r\n\r\n");	
				fclose(fp);
				return 0;
			}
		}

		loc=lchannel*gldie+ldie;
		fscanf(fp,"%d ",&xblock[device][loc]);

		for(j=0;j<xblock[device][loc];j++)
		{
			fscanf(fp,"%d ",&badblock[device][loc][j]);

		}
	}
	fscanf(fp,"%s ",output);
	str=output;	
	if(str!="End!")
	{
		AddInfo("\r\n\r\n****文件格式不正确!!!\r\n\r\n");
		fclose(fp);
		return 0;
	}

	fclose(fp);
	return 1;
}

BOOL SLWAddressAssign(int device)
{
	int i=0;
	int j=0;	
	int m=0;

	int mapblocknum = 0;	// map 15/16 total  block, need block number?
	mapblocknum= (channel*gldie*2*flash_type.blocknum*15/16)/(1024*flash_type.size_block);//new , 2bytes for a block address, need x blocks ?
	
	if(((channel*gldie*flash_type.blocknum*15/16)%(1024*flash_type.size_block))!=0)
		mapblocknum+=1;
	mapblockcnt[device] = mapblocknum;
	
	globleblock[device]=2;//globle block 0
	for(m=0;m<100;m++)
	{
		if(CompareBadblock(device,globleblock[device],0,0)==0)
			globleblock[device]+=step;
		else
			break;
	}
	globleAddr[device]=(globleblock[device]<<16)|(0<<4)|0;
	
	for(i=0;i<4;i++)
	{
		mapblock[device][0][i]=1;//block 1
		for(m=0;m<100;m++)
		{
			if(CompareBadblock(device,mapblock[device][0][i],i,0)==0)
				mapblock[device][0][i]+=step;
			else
				break;
		}
		mapblock[device][1][i]=mapblock[device][0][i]+step;
		mapAddr[device][0][i]=(mapblock[device][0][i]<<16)|(0<<4)|i ;
	}

	for(i=0;i<4;i++)
	{
		for(m=0;m<100;m++)
		{
			if(CompareBadblock(device,mapblock[device][1][i],i,0)==0)
				mapblock[device][1][i]+=step;
			else
				break;
		}
		mapblock[device][2][i]=mapblock[device][1][i]+step;
		mapAddr[device][1][i]=(mapblock[device][1][i]<<16)|(0<<4)|i ;
	}

	for(i=0;i<4;i++)
	{	
		for(m=0;m<100;m++)
		{
			if(CompareBadblock(device,mapblock[device][2][i],i,0)==0)
				mapblock[device][2][i]+=step;
			else
				break;
		}	
		mapAddr[device][2][i]=(mapblock[device][2][i]<<16)|(0<<4)|i ;
	}

	
	Multiblock[device]=2;
	for(m=0;m<100;m++)
	{
		if(CompareBadblock(device,Multiblock[device],1,0)==0)
			Multiblock[device]+=step;
		else
			break;
	}
	MultiAddr[device]=(Multiblock[device]<<16)|(0<<4)|1 ;

	smartblock[device]=2;
	for(m=0;m<100;m++)
	{
		if(CompareBadblock(device,smartblock[device],2,0)==0)
			smartblock[device]+=step;
		else
			break;
	}
	smartAddr[device]=(smartblock[device]<<16)|(0<<4)|2 ;

	securityblock[device]=2;
	for(m=0;m<100;m++)
	{
		if(CompareBadblock(device,securityblock[device],3,0)==0)
			securityblock[device]+=step;
		else
			break;
	}
	securityAddr[device]=(securityblock[device]<<16)|(0<<4)|3 ;

	for(i=0;i<channel*gldie;i++)
	{
		if( ((i%gldie)>7&&(i%gldie)<16) || ((i%gldie)>23&&(i%gldie)<32))
		{
			
			reserveblock[device][i]= flash_type.blocknum; //  如果位于die8 ~ die15或者die24 ~ die31, 从 block 4096 开始.
			for(m=0;m<100;m++)
			{
				if(CompareBadblock(device,reserveblock[device][i],i/gldie,i%gldie)==0)
					reserveblock[device][i]+=step;
				else
					break;
			}
		}

		else
		{
			reserveblock[device][i]=0;  //坏块保留块表所在的物理block号，如果位于die0 ~ die7, 从 block0 开始.(针对三星的物理block编号，从0到8192)

			for(m=0;m<100;m++)
			{
				if(CompareBadblock(device,reserveblock[device][i],i/gldie,i%gldie)==0)
					reserveblock[device][i]+=step;
				else
					break;
			}
		}
	}
/*
	if(step==1)
	{		
		for(i=0;i<channel*gldie;i++)
		{
			startblock[device][0][i]=reserveblock[device][i]+step;			
			
			startblock[device][1][i]=2048;
			startblock[device][2][i]=4096;
			startblock[device][3][i]=6144;
			for(j=0;j<4;j++)
				for(m=0;m<100;m++)
				{
					if(CompareBadblock(startblock[device][j][i],i/16,i%16)==0)
						startblock[device][j][i]+=step;
					else
						break;
				}
		}
		
		for(i=0;i<channel*gldie;i++)
		{
			datablock[device][0][i]=128;
			datablock[device][1][i]=2176;
			datablock[device][2][i]=4224;
			datablock[device][3][i]=6272;
		}
		
		for(i=0;i<channel*gldie;i++)
		{
			swapblock[device][0][i]=datablock[device][0][i]-step;
			swapblock[device][1][i]=datablock[device][1][i]-step;
			swapblock[device][2][i]=datablock[device][2][i]-step;
			swapblock[device][3][i]=datablock[device][3][i]-step;
			for(j=0;j<4;j++)
				for(m=0;m<100;m++)
				{
					if(CompareBadblock(swapblock[device][j][i],i/16,i%16)==0)
						swapblock[device][j][i]-=step;
					else
						break;
				}
		}
		
		for(i=0;i<channel*gldie;i++)
		{
			endblock[device][0][i]=swapblock[device][0][i]-step;
			endblock[device][1][i]=swapblock[device][1][i]-step;
			endblock[device][2][i]=swapblock[device][2][i]-step;
			endblock[device][3][i]=swapblock[device][3][i]-step;
			for(j=0;j<4;j++)
				for(m=0;m<100;m++)
				{
					if(CompareBadblock(endblock[device][j][i],i/16,i%16)==0)
						endblock[device][j][i]-=step;
					else
						break;
				}
		}
		
				
	}
*/

	if(step==2)
	{
		for(i=0;i<channel*gldie;i++)
		{
			if( ((i%gldie)>7&&(i%gldie)<16) || ((i%gldie)>23&&(i%gldie)<32))
			{
				startblock[device][0][i]=reserveblock[device][i]+step;	//plane 0	
				startblock[device][1][i]=flash_type.blocknum+1;		//plane 1
			}
			else
			{
				startblock[device][0][i]=reserveblock[device][i]+step;	//plane 0	
				if(i==0)
					startblock[device][0][i]=globleblock[device]+step;
				else
					startblock[device][1][i]=1;		//plane 1
				if(i%gldie==0)
					startblock[device][1][i]=mapblock[device][1][i/gldie]+step;
			}
			for(j=0;j<2;j++)
				for(m=0;m<100;m++)
				{
					if(CompareBadblock(device,startblock[device][j][i],i/gldie,i%gldie)==0)
						startblock[device][j][i]+=step;
					else
						break;
				}
		}
		for(i=0;i<channel*gldie;i++)
		{
			if( ((i%gldie)>7&&(i%gldie)<16) || ((i%gldie)>23&&(i%gldie)<32))
			{
				datablock[device][0][i]=flash_type.blocknum+flash_type.blocknum*1/16;	// 4096 + 256 = 4352
				datablock[device][1][i]=flash_type.blocknum+flash_type.blocknum*1/16+1;	// 4097 + 256 = 4353
			}
			else
			{
				datablock[device][0][i]=flash_type.blocknum*1/16;
				datablock[device][1][i]=flash_type.blocknum*1/16+1;
			}

		}
		for(i=0;i<channel*gldie;i++)
		{
			if( ((i%gldie)>7&&(i%gldie)<16) || ((i%gldie)>23&&(i%gldie)<32))
			{
				swapblock[device][0][i]=datablock[device][0][i]-step;
				swapblock[device][1][i]=datablock[device][1][i]-step;
			}
			else
			{
				swapblock[device][0][i]=datablock[device][0][i]-step;
				swapblock[device][1][i]=datablock[device][1][i]-step;
			}

			for(j=0;j<2;j++)
				for(m=0;m<100;m++)
				{
					if(CompareBadblock(device,swapblock[device][j][i],i/gldie,i%gldie)==0)
						swapblock[device][j][i]-=step;
					else
						break;
				}
		}
		for(i=0;i<channel*gldie;i++)
		{
			if( ((i%gldie)>7&&(i%gldie)<16) || ((i%gldie)>23&&(i%gldie)<32))
			{
				endblock[device][0][i]=swapblock[device][0][i]-step;
				endblock[device][1][i]=swapblock[device][1][i]-step;
			}
			else
			{
				endblock[device][0][i]=swapblock[device][0][i]-step;
				endblock[device][1][i]=swapblock[device][1][i]-step;
			}
			for(j=0;j<2;j++)
				for(m=0;m<100;m++)
				{
					if(CompareBadblock(device,endblock[device][j][i],i/gldie,i%gldie)==0)
						endblock[device][j][i]-=step;
					else
						break;
				}
		}

	}

	return 1;
}

BOOL CompareBadblock(int device,int blocknum,int channelnum,int dienum)
{
	int i;

	for(i=0;i<xblock[device][channelnum*gldie+dienum];i++)
	{
		if(blocknum==badblock[device][channelnum*gldie+dienum][i])
		{			
			return 0;
		}
			
	}	
	return 1;
}

int SLWEraseBlock(int device,int channelnum,int rownum,int blocknum)
{
	return VNDEraseblk(device,channelnum, rownum,blocknum);
}

BOOL WriteResult(int device,BOOL CreatFlag)
{
	FILE *fp;
	CString str;

	char output[128];
	int i;
	int j;
	int k,ret;	
	
	CString filename;
	if(CreatFlag)
	{
		CFileDialog FileDlg(FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
							"TextFiles (*.bck)|*.bck||");
		if (FileDlg.DoModal() == IDOK)			//打开文件对话框
			filename = FileDlg.GetPathName();	//得到文件路经
		else 
			return 0;
		
		if( filename.Right(4) != ".bck")
			filename += ".bck";	
	}
	else
		filename = BadBlockFilename[device]+".bck";
	if(filename.GetLength()==0)
	{
		return 0;
	}

	char file[128];
    GetModuleFileName(NULL,file,128); 
    //Scan a string for the last occurrence of a character.
    (strrchr(file,'\\'))[1] = 0; 
	strcat(file,"BadblockFile\\");
    strcat(file,filename);

	//判断同目录下是否存在同名的坏块文件
	ret=GetFileAttributes(file);
	if(ret>0)
	{
		AfxMessageBox("当前目录下存在同名的原始坏块文件，请重新选择文件名保存原始坏块文件!");
		CFileDialog FileDlg(FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
							"TextFiles (*.bck)|*.bck||");
		GetModuleFileName(NULL,file,128); 
		(strrchr(file,'\\'))[1] = 0; 
		FileDlg.m_ofn.lpstrInitialDir=file;   
		if (FileDlg.DoModal() == IDOK)			//打开文件对话框
			filename = FileDlg.GetPathName();	//得到文件路经
		else 
			return 0;
		
		if( filename.Right(4) != ".bck")
			filename += ".bck";	

		if(filename.GetLength()==0)
		{
			return 0;
		}
		strcpy(file,filename);
	}
	


	fp = fopen(file, "w");
	if(fp==NULL)
	{
		AfxMessageBox("Open write file fail!");
		return 0;
	}

	sprintf(output,"%s",filename.GetBuffer(0));	
	filename.Empty();
	int pos=0;
	for(i=0;output[i]!='.';i++)
	{
		if(output[i]==('\\'))
		{
			pos=i;
		}
	}
	if(pos==0)
		pos=-1;
	for(i=pos+1;output[i]!='.';i++)
	{
		filename+=output[i];
	}

	str.Format("PRODUCTION SERIALNUMBER: %s\r\n", filename.GetBuffer(0)); //得到文件名
	fprintf(fp,"%s",str);
	
	str=manufacture[device]+" ";
	str+=capacity[device]+" ";
	if(flash_type.isMLC==0)
	{
		str+="SLC\r\n";
	}
	else
	{
		str+="MLC\r\n";
	}
	fprintf(fp,"%s",str);

	str="CHANNEL = 4\r\n";//channel number
	fprintf(fp,"%s",str);

	sprintf(output,"DIE = %d\r\n",grdie);//die number
	str=output;
	fprintf(fp,"%s",str);

	sprintf(output,"PLANE = %d\r\n",2);//row number
	str=output;
	fprintf(fp,"%s",str);		
		
	for(i=0;i<gldie*channel;i++)
	{
		k=DIEMAP[i%gldie];
		if(k==65535)
			continue;

		sprintf(output,"DIE %d CHANNEL %d BADBLOCK NUMBER: %d\r\n",k,i/gldie,xblock[device][i+k-i%gldie]);
		str=output;
		fprintf(fp,"%s",str);
		for(j=0;j<xblock[device][i+k-i%gldie];j++)
		{
			fprintf(fp,"%04d ",badblock[device][i+k-i%gldie][j]);
	//		fprintf(fp,"%s","\r\n");			
		}
		fprintf(fp,"%s","\r\n");
	}

	str="End!";
	fprintf(fp,"%s",str);
	fclose(fp);
	return  1;
}

BOOL SLWReadBadblock(int device)
{
	unsigned int i,j,block,ret,k,addr,badblockaddr,temp;
	unsigned char buf[512*18];

	for(i=0;i<5120;i++)
		buf[i]=0xFF;

	for(i=0;i<channel;i++)
		for(j=0;j<grdie;j++)
		{
			if(DIEMAP[j]==0xFFFF)
				continue;
			if( (DIEMAP[j]>7&&DIEMAP[j]<16) || (DIEMAP[j]>23&&DIEMAP[j]<32) )
				block=flash_type.blocknum;
			else
				block=0;
	
			VNDSetBlockArg(device,0,18,0);
			ret = VNDReadblock(device,buf,i,DIEMAP[j],block);
			if (ret == 0)
			{
				AddInfo("****Read BadblockTable info failed!!!");	
				return 0;
			}
			addr=4096;
			for(k=0;k<128;k++)
			{
				temp=buf[addr+8*k+1]&0x80;
				if(temp==0x80)
				{
					badblockaddr=(buf[addr+8*k]|(buf[addr+8*k+1]<<8))&0x3FFF;
					SLWUpdateBadblock(device,i,DIEMAP[j],badblockaddr);
				}
			}
			addr=8192;
			for(k=0;k<128;k++)
			{
				temp=buf[addr+8*k+1]&0x80;
				if(temp==0x80)
				{
					badblockaddr=(buf[addr+8*k]|(buf[addr+8*k+1]<<8))&0x3FFF;
					SLWUpdateBadblock(device,i,DIEMAP[j],badblockaddr);
				}
			}
		}
	return 1;
}

BOOL SLWGetDeviceInfo(int device,int Initnum)
{
	int i=0;
	int j=0;
	int ret;
	CString str;
	int ichannel,irow;
	DWORD dblock;

	if(Initnum==0)
	{
		if(IniMode==1)
		{
			if(SLWFindBadblock(device)==0)
			{
				AddInfo("****GetDeviceInfo FindBadblock failed!!!");		
				return 0;
			}
		
			if(WriteResult(device,0)==0)
			{
				AddInfo("****Save bad block table failed!!!");		
				return 0;
			}	
					
		}
		else if(IniMode==2)	
		{
			if(SLWReadBadblock(device)==0)
			{
				AddInfo("****GetDeviceInfo Read BadBlockTables failed!!!");		
				return 0;
			}	
			if(WriteResult(device,0)==0)
			{
				AddInfo("****Save bad block table failed!!!");		
				return 0;
			}		
		}
		else if(IniMode==3)	
		{
			/*擦除所有block，找出坏块*/
			SLWEraseAll(device);
			if(WriteResult(device,0)==0)
			{
				AddInfo("****Save bad block table failed!!!");		
				return 0;
			}	
		}
		else if(IniMode==0)	//从文件初始化
		{
			if(SLWReadResult(device)==0)
			{
				AddInfo("****GetDeviceInfo ReadResult failed!!!");		
				return 0;
			}				
		}
		else
		{
				AddInfo("****Please select the mode for bad block !!!");							
				return 0;
		}
	}


	AddInfo("Address Assign...");
	
	
	if(SLWAddressAssign(device)==0)
	{
		AddInfo("AddressAssign fail!");		
		return 0;
	}	
	
#if _INITD 
//	if(pGuiSolExplorer->m_class1.GetCheck()!=TRUE)//erase some blocks for initial
#endif
	{
		AddInfo("Erasing block...");
		dblock=globleblock[device];
		ichannel=0;
		irow=DIEMAP[0];
		ret=SLWEraseBlock(device,ichannel,irow,dblock);
		if(ret==-1)
		{
			AddInfo("****Erase global Block failed!!!");
			SLWUpdateBadblock(device,ichannel,irow,dblock);
			Initnum++;
			SLWInitial(device,Initnum);
			return 1;
		}
		else if(ret==-2)
		{
			return 0;
		}

		for(i=0;i<4;i++)
			for(j=0;j<3;j++)
		{
			irow=DIEMAP[0];
			ret = SLWEraseBlock(device,i,irow,mapblock[device][j][i]);
			if(ret==-1)//erase map block 1
			{
				AddInfo("Write Map fail!\r\n");
				SLWUpdateBadblock(device,i,irow,mapblock[device][j][i]);
				Initnum++;
				SLWInitial(device,Initnum);
				return 1;
			}
			else if(ret==-2)
			{
				return 0;
			}
		}

		dblock=Multiblock[device];
		ichannel=1;
		irow=DIEMAP[0];

		ret=SLWEraseBlock(device,ichannel,irow,dblock);
		if(ret==-1)
		{
			AddInfo("****Erase Multi-boot Block failed!!!");
			SLWUpdateBadblock(device,ichannel,irow,dblock);
			Initnum++;
			SLWInitial(device,Initnum);
			return 1;
		}
		else if(ret==-2)
		{
			return 0;
		}

		dblock=smartblock[device];
		ichannel=2;
		irow=DIEMAP[0];
	
		ret=SLWEraseBlock(device,ichannel,irow,dblock);
		if(ret==-1)
		{
			AddInfo("****Erase smart Block failed!!!");	
			SLWUpdateBadblock(device,ichannel,irow,dblock);
			Initnum++;
			SLWInitial(device,Initnum);	
			return 1;
		}
		else if(ret==-2)
		{
			return 0;
		}

		dblock=securityblock[device];
		ichannel=3;
		irow=DIEMAP[0];
	
		ret=SLWEraseBlock(device,ichannel,irow,dblock);
		if(ret==-1)
		{
			AddInfo("****Erase security Block failed!!!");		
			SLWUpdateBadblock(device,ichannel,irow,dblock);
			Initnum++;
			SLWInitial(device,Initnum);
			return 1;
		}
		else if(ret==-2)
		{
			return 0;
		}
	
		for(i=0;i<channel*gldie;i++)//擦除坏块保留区
		{
			irow=DIEMAP[i%gldie];  //擦除block需要物理die号，所以用DIEMAP		
			dblock=reserveblock[device][i+irow-i%gldie];
			ichannel=i/gldie;
			if (irow ==65535)  //当前物理die不存在，回到循环开始
				continue;
			ret=SLWEraseBlock(device,ichannel,irow,dblock);
			if(ret==-1)
			{
				str.Format("Erase Reserve Block failed: CH:%d ROW:%d block:%d",
							ichannel, irow, dblock);
				AddInfo(str);
				SLWUpdateBadblock(device,ichannel,irow,dblock);
				Initnum++;
				SLWInitial(device,Initnum);
				return 1;
			}
			else if(ret==-2)
			{
				return 0;
			}
		}

		for(j=0;j<plane;j++)//擦除交换块
		{
			for(i=0;i<channel*gldie;i++)
			{
				irow=DIEMAP[i%gldie];
				dblock=swapblock[device][j][i+irow-i%gldie];
				ichannel=i/gldie;				
				if (irow ==65535)  //当前物理die不存在，回到循环开始
					continue;
				ret=SLWEraseBlock(device,ichannel,irow,dblock);
				if(ret==-1)
				{
					str.Format("Erase SWAP Block failed: CH:%d ROW:%d block:%d",
							ichannel, irow, dblock);
					AddInfo(str);
					SLWUpdateBadblock(device,ichannel,irow,dblock);
					Initnum++;
					SLWInitial(device,Initnum);
					return 1;
				}
				else if(ret==-2)
				{
					return 0;
				}
			}
		}
	
	}

	return 1;
}

BOOL SLWGetSn(int device)
{
	char output[128];
	int i,j;
	char DATABuffer[20];
	CString sn;
	memset(DATABuffer,0,20);
	memset(output,0,32);
	int ret =BadBlockFilename[device].Find(".");
	if(IniMode==0&&ret>0)
	{
		sprintf(output,"%s",BadBlockFilename[device].GetBuffer(0));	
		int pos=0;
		for(i=0;output[i]!='.';i++)
		{
			if(output[i]==('\\'))
			{
				pos=i;
			}
		}
		if(pos==0)
			pos=-1;
		for(i=pos+1,j=0;output[i]!='.';i++,j++)
		{
			DATABuffer[j]=output[i];
		}
		BadBlockFilename[device]=DATABuffer;
	}

	return 1;
}


void SLWSetCapacity(int device)
{
	int i;
	int addptr=1024;
	ULONGLONG lcapa;
	DWORD tmp;

	tmp=grdie*channel*flash_type.blocknum*15/16;	//  保留/16	的block内部用		
	lcapa=tmp*flash_type.pages_per_block*flash_type.sectors_per_page;

	if(lcapa<0xF00000)
	{
		IDFY32G[1]=lcapa/(16*63);
		IDFY32G[54]=IDFY32G[1];
		tmp=IDFY32G[1]*16*63;
		IDFY32G[57]=tmp;
		IDFY32G[58]=(tmp)>>16;

		IDFY32G[60]=lcapa;
		IDFY32G[61]=lcapa>>16;
		IDFY32G[83]=0x5000;
		IDFY32G[86]=0x1000;
		IDFY32G[100]=IDFY32G[60];
		IDFY32G[101]=IDFY32G[61];
		for(i=0;i<256;i++)
		{
			GlobelBuffer[device][addptr+2*i]=(unsigned char)IDFY32G[i];//WORD的低位在前
			GlobelBuffer[device][addptr+2*i+1]=(unsigned char)(IDFY32G[i]>>8);
		}
	}
	else
	{
		IDFY32G[1]=0x3FFF;
		IDFY32G[54]=IDFY32G[1];
		IDFY32G[57]=0xFC10;
		IDFY32G[58]=0x00FB;
		IDFY32G[83]=0x5400;
		IDFY32G[86]=0x1400;
	}

	if((channel*grdie*plane*blocksize*flash_type.blocknum/(4*4096))==8)//new 8GB
	{	
		IDFY32G[31]=0x3030;
		IDFY32G[32]=0x3847;
		IDFY32G[60]=0;
		IDFY32G[61]=0xF0;

		IDFY32G[100]=IDFY32G[60];
		IDFY32G[101]=IDFY32G[61];
		for(i=0;i<256;i++)
		{
			GlobelBuffer[device][addptr+2*i]=(unsigned char)IDFY32G[i];//WORD的低位在前
			GlobelBuffer[device][addptr+2*i+1]=(unsigned char)(IDFY32G[i]>>8);
		}
	}
	else if((channel*grdie*plane*blocksize*flash_type.blocknum/(4*4096))==16)//new 16GB
	{	

		IDFY32G[31]=0x3031;
		IDFY32G[32]=0x3647;
		lcapa=0x01E00000;
		IDFY32G[60]=lcapa;	
		IDFY32G[61]=lcapa>>16;	

		IDFY32G[100]=IDFY32G[60];
		IDFY32G[101]=IDFY32G[61];
		for(i=0;i<256;i++)
		{
			GlobelBuffer[device][addptr+2*i]=(unsigned char)IDFY32G[i];
			GlobelBuffer[device][addptr+2*i+1]=(unsigned char)(IDFY32G[i]>>8);
		}
	}
	else if((channel*grdie*plane*blocksize*flash_type.blocknum/(4*4096))==32)//new 32GB
	{
		IDFY32G[31]=0x3033;
		IDFY32G[32]=0x3247;
		lcapa=0x03c00000;
		IDFY32G[60]=lcapa;	
		IDFY32G[61]=lcapa>>16;	
		IDFY32G[100]=IDFY32G[60];
		IDFY32G[101]=IDFY32G[61];
		for(i=0;i<256;i++)
		{
			GlobelBuffer[device][addptr+2*i]=(unsigned char)IDFY32G[i];
			GlobelBuffer[device][addptr+2*i+1]=(unsigned char)(IDFY32G[i]>>8);
		}
	}
	else if((channel*grdie*plane*blocksize*flash_type.blocknum/(4*4096))==64)////4k//new 64GB
	{	
		IDFY32G[31]=0x3036;//64G//4k
		IDFY32G[32]=0x3447;//64G//4k
		lcapa=0x07800000;
		IDFY32G[60]=lcapa;
		IDFY32G[61]=lcapa>>16;//64G//4k	
		IDFY32G[100]=IDFY32G[60];
		IDFY32G[101]=IDFY32G[61];
		for(i=0;i<256;i++)
		{
			GlobelBuffer[device][addptr+2*i]=(unsigned char)IDFY32G[i];
			GlobelBuffer[device][addptr+2*i+1]=(unsigned char)(IDFY32G[i]>>8);
		}
	}
	else if((channel*grdie*plane*blocksize*flash_type.blocknum/(4*4096))==128)//128G//4k//new
	{
	
		IDFY32G[31]=0x3132;//128G//4k
		IDFY32G[32]=0x3847;//128G//4k
		lcapa=0x0f000000;

		IDFY32G[60]=lcapa;
		IDFY32G[61]=lcapa>>16;//128G	
		IDFY32G[100]=IDFY32G[60];
		IDFY32G[101]=IDFY32G[61];
		for(i=0;i<256;i++)
		{
			GlobelBuffer[device][addptr+2*i]=(unsigned char)IDFY32G[i];
			GlobelBuffer[device][addptr+2*i+1]=(unsigned char)(IDFY32G[i]>>8);
		}
	}
	else if((channel*grdie*plane*blocksize*flash_type.blocknum/(4*4096))==256)//256G//4k//new
	{
		IDFY32G[31]=0x3235;//256G//4k
		IDFY32G[32]=0x3647;//256G//4k
		IDFY32G[60]=0xffff;
		IDFY32G[61]=0x0fff;//256G
		IDFY32G[100]=0;
		IDFY32G[101]=0x1dff+1;
		for(i=0;i<256;i++)
		{
			GlobelBuffer[device][addptr+2*i]=(unsigned char)IDFY32G[i];
			GlobelBuffer[device][addptr+2*i+1]=(unsigned char)(IDFY32G[i]>>8);
		}
	}
	
	MaxLBA = (IDFY32G[61]<<16|IDFY32G[60])-1;

}

BOOL SLWWriteGlobleInfoBlock(int device,int Initnum)
{
	int i,j;
	char output[256];
	int datasize=flash_type.size_block * 1024;
	GlobelBuffer[device]=(PUCHAR) malloc(datasize);

#if _INITD 
	pGuiSolExplorer->GetArgs();
#endif

	for(i=0;i<datasize;i++)
	{
		GlobelBuffer[device][i]=0xff;		
	}

	int addptr=0;
	GlobelBuffer[device][addptr++]=0x55;//globleinfoblock flag
	GlobelBuffer[device][addptr++]=0xAA;
	GlobelBuffer[device][addptr++]=0x5A;
	GlobelBuffer[device][addptr++]=0xA5;

	GlobelBuffer[device][addptr++]=0x20;//FW 起始位置（扇区为单位）
	GlobelBuffer[device][addptr++]=0x00;
	GlobelBuffer[device][addptr++]=0x00;
	GlobelBuffer[device][addptr++]=0x00;
	
	GlobelBuffer[device][addptr++]=0x40;//FW 长度（扇区为单位）
	GlobelBuffer[device][addptr++]=0x00;
	GlobelBuffer[device][addptr++]=0x00;
	GlobelBuffer[device][addptr++]=0x00;
/*
	GlobelBuffer[device][addptr++]=0x88;//Rebuild 代码 起始位置（扇区为单位）
	GlobelBuffer[device][addptr++]=0x00;
	GlobelBuffer[device][addptr++]=0x00;
	GlobelBuffer[device][addptr++]=0x00;
	
	GlobelBuffer[device][addptr++]=0x80;//Rebuild 代码 长度（扇区为单位）
	GlobelBuffer[device][addptr++]=0x00;
	GlobelBuffer[device][addptr++]=0x00;
	GlobelBuffer[device][addptr++]=0x00;
*/	
	addptr=0x20;
	for(i=0;i<3;i++)
		for(j=0;j<4;j++)
		{
			GlobelBuffer[device][addptr++]=mapAddr[device][i][j];
			GlobelBuffer[device][addptr++]=mapAddr[device][i][j]>>8;
			GlobelBuffer[device][addptr++]=mapAddr[device][i][j]>>16;
			GlobelBuffer[device][addptr++]=mapAddr[device][i][j]>>24;
		}


	addptr=1024;		//IDFY DATA
	SLWSetCapacity(device);
#if SLW_MCR_SMART>0
	GlobelBuffer[device][addptr+256]=0x01;
#else
	GlobelBuffer[device][addptr+256]=0x00;
#endif


	/////////////////////////// sn ///////////////////
	for(i=20;i<40;i++)
	{
		GlobelBuffer[device][addptr+i]=0x20;
	}
	SLWGetSn(device);

	BadBlockFilename[device].TrimRight();
	if(BadBlockFilename[device].GetLength()>20)
	{
		AfxMessageBox("Serial Number的最大长度是20个字节,当前值超出范围，请重新输入!");
		return 0;
	}
	sprintf(output,"%s",BadBlockFilename[device].GetBuffer(0));	


	int pos=0;

	//写入WWN
	GlobelBuffer[device][addptr+216]=0x01;
	GlobelBuffer[device][addptr+217]=0x50;
	GlobelBuffer[device][addptr+218]=0xfa;

	GlobelBuffer[device][addptr+219]=slwatoh(output[0])|0xE0;
	GlobelBuffer[device][addptr+220]=slwatoh(output[1])<<4 | slwatoh(output[2]);
	GlobelBuffer[device][addptr+221]=slwatoh(output[3])<<4 | slwatoh(output[4]);
	GlobelBuffer[device][addptr+222]=slwatoh(output[5])<<4 | slwatoh(output[6]);
	GlobelBuffer[device][addptr+223]=slwatoh(output[7])<<4 | slwatoh(output[8]);

	for(i=0;output[i]!=NULL;i++)
	{
		GlobelBuffer[device][addptr+20+i]=output[i];
	}
	for(i=10;i<20;i++)
	{
		output[0]=GlobelBuffer[device][addptr+2*i];
		GlobelBuffer[device][addptr+2*i]=GlobelBuffer[device][addptr+2*i+1];
		GlobelBuffer[device][addptr+2*i+1]=output[0];
	}


	///////////////////////////fw version//////////////////
	for(i=46;i<54;i++)
	{
		GlobelBuffer[device][addptr+i]=0x20;
	}
	sprintf(output,"%s",m_fw[device]);
	for(i=0;i<8;i++)
	{
		GlobelBuffer[device][addptr+46+i]=output[i];
	}
	for(i=23;i<27;i++)
	{
		output[0]=GlobelBuffer[device][addptr+2*i];
		GlobelBuffer[device][addptr+2*i]=GlobelBuffer[device][addptr+2*i+1];
		GlobelBuffer[device][addptr+2*i+1]=output[0];
	}

	///////////////// descriptor /////////////
	CString descr="";

	if(m_descriptor[device]=="")
	{
		descr="SoliWare";
	}
	else
		descr=m_descriptor[device];

	descr.TrimRight();
	if(descr.GetLength()>40)
	{
		AfxMessageBox("Model Number 的最大长度是40个字节,当前值超出范围，请重新输入!");
		return 0;
	}

	sprintf(output,"%s",descr);
	for(i=0;i<descr.GetLength();i++)
	{
		GlobelBuffer[device][addptr+54+i]=output[i];
	}
	for(i=descr.GetLength();i<40;i++)
	{
		GlobelBuffer[device][addptr+54+i]=0x20;
	}

	for(i=27;i<47;i++)
	{
		output[0]=GlobelBuffer[device][addptr+2*i];
		GlobelBuffer[device][addptr+2*i]=GlobelBuffer[device][addptr+2*i+1];
		GlobelBuffer[device][addptr+2*i+1]=output[0];
	}

	GlobelBuffer[device][addptr+418]=0x1;
	GlobelBuffer[device][addptr+419]=0x40;
	GlobelBuffer[device][addptr+434]=0x1;
	GlobelBuffer[device][addptr+435]=0x0;

	int ret;
	/*读RAM-DISK的identify 数据，判断字偏移26位处(即byte 52)的bit 4，是1就是pata,是0就是sata*/
	ret = SLWGetInterfaceflag(device);
	if(ret==-1)
	{
		AddInfo("Get Identify data fail!\r\n");
		return 0;
	}
	/*Supports SATA Gen2 Signaling Speed (3.0Gb/s)  （填入0x6,支持sata2必须向下支持sata1） and SATA Gen1  （填入0x2） PATA （填入0x0） */
	else if(ret>0)
	{
		GlobelBuffer[device][1024+152]=0x0;	
	}
	else if(ret==0)
	{
		GlobelBuffer[device][1024+152]=0x2;
	}

	GlobelBuffer[device][addptr+510]=0xa5;
	int checksum=0;
	for(i=0;i<511;i++)
	{
		checksum+=GlobelBuffer[device][addptr+i];
	}
	GlobelBuffer[device][addptr+511]=~checksum+1;//Bytes 511 为所有值的CheckSum

    addptr=2048;
	/*写DIEMAP信息*/
	for(i=0;i<grdie;i++)
	{
		GlobelBuffer[device][addptr++] = DIEMAP[i];
		GlobelBuffer[device][addptr++] = DIEMAP[i] >> 8;
		GlobelBuffer[device][addptr++] = 0;
		GlobelBuffer[device][addptr++] = 0;
	}
	
	addptr = 2048+128;
	
	if(flash_type.isMLC==0)
	{
		for(i=0;i<ConfNum;i++)
		{
			GlobelBuffer[device][addptr+2*i]=(unsigned char)SLCFLASH[i];//WORD的低位在前
			GlobelBuffer[device][addptr+2*i+1]=(unsigned char)(SLCFLASH[i]>>8);
		}
		mapsize[device]=SLCFLASH[50];
	}
	else
	{
		for(i=0;i<ConfNum;i++)
		{
			GlobelBuffer[device][addptr+2*i]=(unsigned char)MLCFLASH[i];//WORD的低位在前
			GlobelBuffer[device][addptr+2*i+1]=(unsigned char)(MLCFLASH[i]>>8);
		}
		mapsize[device]=MLCFLASH[50];	
	}


	addptr = 2048+0x84;
	/*读RAM-DISK的identify 数据，判断字偏移26位处(即byte 52)的最高位，最高位是1就是pata,是0就是sata*/
	ret = SLWGetInterfaceflag(device);
	if(ret==-1)
	{
		AddInfo("Get Identify data fail!\r\n");
		return 0;
	}
	/*Supports SATA Gen2 Signaling Speed (3.0Gb/s)  （填入0x6,支持sata2必须向下支持sata1） and SATA Gen1  （填入0x2） PATA （填入0x0） */
	else if(ret>0)
	{
		GlobelBuffer[device][addptr]=0;
	}
	else if(ret==0)
	{
		GlobelBuffer[device][addptr]=1;
	}
	
	/*初始化时，为写全盘需要，LBA alignment的值和MultiBoot的值都为0*/
	addptr=2048+0xC8;
	GlobelBuffer[device][addptr++]=0;
	GlobelBuffer[device][addptr++]=0;
	GlobelBuffer[device][addptr++]=0;
	GlobelBuffer[device][addptr++]=0;
	addptr=2048+0xE8;
	GlobelBuffer[device][addptr++]=0;
	GlobelBuffer[device][addptr++]=0;
	GlobelBuffer[device][addptr++]=0;
	GlobelBuffer[device][addptr++]=0;

	addptr=2048+0xCC;
	GlobelBuffer[device][addptr++]=MultiAddr[device];
	GlobelBuffer[device][addptr++]=MultiAddr[device]>>8;
	GlobelBuffer[device][addptr++]=MultiAddr[device]>>16;
	GlobelBuffer[device][addptr++]=MultiAddr[device]>>24;

	addptr=2048+0xEC;
	GlobelBuffer[device][addptr++]=globleAddr[device];
	GlobelBuffer[device][addptr++]=globleAddr[device]>>8;
	GlobelBuffer[device][addptr++]=globleAddr[device]>>16;
	GlobelBuffer[device][addptr++]=globleAddr[device]>>24;

	GlobelBuffer[device][addptr++]=smartAddr[device];
	GlobelBuffer[device][addptr++]=smartAddr[device]>>8;
	GlobelBuffer[device][addptr++]=smartAddr[device]>>16;
	GlobelBuffer[device][addptr++]=smartAddr[device]>>24;

	GlobelBuffer[device][addptr++]=securityAddr[device];
	GlobelBuffer[device][addptr++]=securityAddr[device]>>8;
	GlobelBuffer[device][addptr++]=securityAddr[device]>>16;
	GlobelBuffer[device][addptr++]=securityAddr[device]>>24;
	
	SLWWritesmartInfo(device,GlobelBuffer[device],512*5);

	addptr = 512*32;
	ret = SLWGlobalinfo(device,GlobelBuffer[device],Initnum,addptr,0);
	if(ret ==0 )
	{				
		AddInfo("Get FW code fail!\r\n");
		free(GlobelBuffer[device]);
		return 0;
	}

	return 1;
}

BOOL SLWWriteBadReserveBlock(int device,int Initnum)
{
	int i,j,k,a,addr,addptr=0;
	char output[128];
	PUCHAR DATABuffer,buffer=NULL;
	int datasize=flash_type.size_block * 1024;
	DATABuffer=(PUCHAR) malloc(datasize);
	unsigned char buf[2][512];
	int pos,planeaddr[2];
	DWORD reblock,num;
	int ret;
	int ichannel,irow;
	DWORD dblock;
	MinReserveblock[device]=128;
	if(step==1)
	{
		for(i=0;i<channel*gldie;i++)
		{
			if( ((i%gldie)>7&&(i%gldie)<16) || ((i%gldie)>23&&(i%gldie)<32))
			{
				datablock[device][0][i]=flash_type.blocknum+flash_type.blocknum*1/32;     // 4224=4096+128
				datablock[device][1][i]=flash_type.blocknum+flash_type.blocknum*1/2+flash_type.blocknum*1/32; // 6272=4096+2048+128	
			}
			else
			{
				datablock[device][0][i]=flash_type.blocknum*1/32; //128
				datablock[device][1][i]=flash_type.blocknum*1/2+flash_type.blocknum*1/32;  //2176=2048+128
			}
		}
	}
	else
	{
		for(i=0;i<channel*gldie;i++)
		{
			if( ((i%gldie)>7&&(i%gldie)<16) || ((i%gldie)>23&&(i%gldie)<32))
			{
				datablock[device][0][i]=flash_type.blocknum*1/16+flash_type.blocknum;	// 4096 + 256 = 4352
				datablock[device][1][i]=flash_type.blocknum*1/16+flash_type.blocknum+1;	// 4097 + 256 = 4353
			}
			else
			{
				datablock[device][0][i]=flash_type.blocknum*1/16;
				datablock[device][1][i]=flash_type.blocknum*1/16+1;
			}
		}
	}

	for(i=0;i<channel*gldie;i++)
	{		
		ichannel=i/gldie;
		irow=DIEMAP[i%gldie];
		if (irow == 65535)
			continue;
		int loc;
		loc=i+irow-i%gldie;
		dblock=reserveblock[device][loc];//坏块保留块的位置

		//////初始化DATA//////
		for(j=0;j<datasize;j++)
		{
			DATABuffer[j]=0xff;		
		}

		addptr=0;
		//加上该芯片的序列号，由随机数和时间组成
		SYSTEMTIME   st;	 
		GetLocalTime(&st);
		
		DATABuffer[addptr++]=(unsigned char)(st.wYear-2000);//生产日期
		DATABuffer[addptr++]=(unsigned char)(st.wMonth);
		DATABuffer[addptr++]=(unsigned char)(st.wDay);
		DATABuffer[addptr++]=(unsigned char)(st.wHour);
		DATABuffer[addptr++]=(unsigned char)(st.wMinute);
		DATABuffer[addptr++]=(unsigned char)(st.wSecond);
		DATABuffer[addptr++]=(unsigned char)(st.wMilliseconds);
		DATABuffer[addptr++]=(unsigned char)(st.wMilliseconds >> 8);
		
		srand( (unsigned)st.wMilliseconds);
		DATABuffer[addptr++]=(unsigned char)(rand());
		DATABuffer[addptr++]=(unsigned char)(rand() >> 8);
		DATABuffer[addptr++]=(unsigned char)(rand());
		DATABuffer[addptr++]=(unsigned char)(rand() >> 8);

		//加上channel号，die号
		DATABuffer[addptr++]=(unsigned char)ichannel;
		DATABuffer[addptr++]=0;
		DATABuffer[addptr++]=(unsigned char)irow;
		DATABuffer[addptr++]=0;

		for(j=0;j<plane;j++)
		{
			for(k=flash_type.blocknum*1/32;k>0;k--)
			{
				a=datablock[device][j][loc]-k*step;
				addr=512-4*k;
				// 可用的保留块00；
				// 保留区已经被写过的块,映射块，交换块，坏块保留块01；
				// 保留区本身是坏块10；
				// 保留区本身是好块，但被数据区的坏块替换，10。
				if(a<startblock[device][j][loc])
				{

					if(CompareBadblock(device,a,i/gldie,irow)==0)
					{
						buf[j][addr++]=(unsigned char)(a);
						buf[j][addr++]=(unsigned char)(a>>8)|0x80;
						buf[j][addr++]=0x00;
						buf[j][addr++]=0x00;
					}
					else
					{
						buf[j][addr++]=(unsigned char)(a);
						buf[j][addr++]=(unsigned char)(a>>8)|0x40;
						buf[j][addr++]=0x00;
						buf[j][addr++]=0x00;
					}
				}
				else if(a>endblock[device][j][loc])
				{
					if(CompareBadblock(device,a,i/gldie,irow)==0)
					{
						buf[j][addr++]=(unsigned char)(a);
						buf[j][addr++]=(unsigned char)(a>>8)|0x80;
						buf[j][addr++]=0x00;
						buf[j][addr++]=0x00;
					}
					else
					{
						if(a==swapblock[device][j][loc])
						{
							buf[j][addr++]=(unsigned char)(a);
							buf[j][addr++]=(unsigned char)(a>>8)|0x40;
							buf[j][addr++]=0x00;
							buf[j][addr++]=0x00;
						}
						else
						{
							replaceblocknum[device][j][loc]--;
							buf[j][addr++]=(unsigned char)(replaceblock[device][j][loc][replaceblocknum[device][j][loc]]);
							buf[j][addr++]=(unsigned char)(replaceblock[device][j][loc][replaceblocknum[device][j][loc]]>>8)|0x80;
							buf[j][addr++]=0x00;
							buf[j][addr++]=0x00;
						}
					}
				}
				else
				{	if(CompareBadblock(device,a,i/gldie,irow)==0)
					{
						buf[j][addr++]=(unsigned char)(a);
						buf[j][addr++]=(unsigned char)(a>>8)|0x80;
						buf[j][addr++]=0x00;
						buf[j][addr++]=0x00;
					}
					else
					{
						buf[j][addr++]=(unsigned char)(a);
						buf[j][addr++]=(unsigned char)(a>>8);
						buf[j][addr++]=0x00;
						buf[j][addr++]=0x00;
					}
				}
			
			}
			if(replaceblocknum[device][j][loc]!=0)
			{
				sprintf(output,"replaceblocknum[device][%d][%d]!=0\r\n",j,i);
			}
		}

		/*对保留块项排序，顺序是保留块（00）,擦出错（10），其他类型块（01）*/
		for(j=0;j<plane;j++)
		{
			pos=0;
			if(j==0)
				addptr=flash_type.sectors_per_page*512;
			else
				addptr=flash_type.sectors_per_page*512*2;
			for(k=flash_type.blocknum*1/32;k>0;k--)
			{
				addr=512-4*k;
				if((buf[j][addr+1]&0xc0)==0)
				{
					reblock=buf[j][addr]; 
					DATABuffer[addptr++]=buf[j][addr++];
					reblock=buf[j][addr]<<8|reblock; 
					DATABuffer[addptr++]=buf[j][addr++];
					reblock=buf[j][addr]<<8|reblock; 
					DATABuffer[addptr++]=buf[j][addr++];
					reblock=buf[j][addr]<<8|reblock; 
					DATABuffer[addptr++]=buf[j][addr++];
					DATABuffer[addptr++]=0x00;
					DATABuffer[addptr++]=0x00;
					DATABuffer[addptr++]=0x00;
					DATABuffer[addptr++]=0x00;
					pos++;
				//	if(pos<110)		/*实验用代码，测试的时候只擦除后面16个保留块，正式版本应该擦除所有的保留块*/
				//		continue;

					ret = SLWEraseBlock(device,ichannel,irow,reblock);
					if(ret==-1)//erase reserver block
					{
						free(DATABuffer);
						AddInfo("EraseBadReserveBlock fail!\r\n");
						LogInfo(device,"EraseBadReserveBlock!!!\r\n",3);
						SLWUpdateBadblock(device,ichannel,irow,reblock);
						Initnum++;
						SLWInitial(device,Initnum);
						return 1;
					}
					else if(ret==-2)
					{
						free(DATABuffer);
						return 0;
					}
				}				
			}
			planeaddr[j]=pos;
			for(k=flash_type.blocknum*1/32;k>0;k--)
			{
				addr=512-4*k;
				if((buf[j][addr+1]&0xc0)==0x80)
				{
					DATABuffer[addptr++]=buf[j][addr++];
					DATABuffer[addptr++]=buf[j][addr++];
					DATABuffer[addptr++]=buf[j][addr++];
					DATABuffer[addptr++]=buf[j][addr++];
					DATABuffer[addptr++]=0x00;
					DATABuffer[addptr++]=0x00;
					DATABuffer[addptr++]=0x00;
					DATABuffer[addptr++]=0x00;
				}				
			}
			for(k=flash_type.blocknum*1/32;k>0;k--)
			{
				addr=512-4*k;
				if((buf[j][addr+1]&0xc0)==0x40)
				{
					DATABuffer[addptr++]=buf[j][addr++];
					DATABuffer[addptr++]=buf[j][addr++];
					DATABuffer[addptr++]=buf[j][addr++];
					DATABuffer[addptr++]=buf[j][addr++];
					DATABuffer[addptr++]=0x00;
					DATABuffer[addptr++]=0x00;
					DATABuffer[addptr++]=0x00;
					DATABuffer[addptr++]=0x00;
					CString str;
					str.Format("%d %d %d %d",buf[j][addr-4],buf[j][addr-3],buf[j][addr-2],buf[j][addr-1]);
				}				
			}
		}
		
		addptr=0x10;
		if(planeaddr[0]<MinReserveblock[device])
			MinReserveblock[device]=planeaddr[0];
		DATABuffer[addptr++]=planeaddr[0];
		DATABuffer[addptr++]=planeaddr[0] >> 8;
		addptr=0x20;
		if(planeaddr[1]<MinReserveblock[device])
			MinReserveblock[device]=planeaddr[1];
		DATABuffer[addptr++]=planeaddr[1];
		DATABuffer[addptr++]=planeaddr[1] >> 8;
	
		DWORD block_add;
		block_add = 8188<< 18 | dblock << 5| irow;	// LBN + strip size
		VNDSetBlockArg(device,0,sectorperblock,7);
		ret = VNDWriteblock(device,DATABuffer,ichannel,block_add,sectorperblock);
		if(ret==-1)//write map to flash block
		{
			AddInfo("Write BadReserveBlock fail!\r\n");
			LogInfo(device,"Write BadReserveBlock!!!\r\n",3);
			free(DATABuffer);
			SLWUpdateBadblock(device,ichannel,irow,dblock);
			Initnum++;
			SLWInitial(device,Initnum);
			return 1;
		}
		else if(ret==-2)
		{
			free(DATABuffer);
			return 0;
		}
		
		buffer=(PUCHAR)malloc(sectorperblock*512);
		memset(buffer, 255, sectorperblock*512);

		VNDSetBlockArg(device,0,sectorperblock,0);
		ret= VNDReadblock(device,buffer,ichannel,irow,dblock);
		if(ret ==0)
		{
			free(buffer);
			free(DATABuffer);
			AddInfo("Read BadReserveBlock info fail!\r\n");		
			LogInfo(device,"Read BadReserveBlock fail!!!\r\n",3);
			return 0;
		}
/*
		if( !DMA_OPS(device,buffer, sectorperblock*512, 0, false, ata_READ_DMA))
		{
			free(buffer);
			free(DATABuffer);
			return 0;
		}
*/
		for(num=0;num<sectorperblock*512;num++)
		{
			if(DATABuffer[num]!=buffer[num])
			{
				AddInfo("****Compare BadReserveBlock info failed!!!");	
				LogInfo(device,"Compare BadReserveBlock info failed!!!!\r\n",3);

				free(buffer);
				free(DATABuffer);
				return 0;
			}
		}
#if _INITD 
					pGuiServerExplorer->m_progress.SetPos((30+55*i/(channel*gldie)));
#endif
#if _MASSINIT
		CString str;
		str.Format("%d",80+20*i/(channel*gldie));
		str+="%";
		pSoliWareMPToolDlg->m_GridCtrl.SetItemText(device+1,2,str);
		pSoliWareMPToolDlg->m_GridCtrl.Refresh();
#endif
	}

	free(DATABuffer);
	if(buffer!=NULL)
		free(buffer);
	return 1;
}

BOOL SLWWriteCodeBlock(int device)
{	
	PUCHAR DATABuffer;	
	int i=0;
	int j=0;
	int k=0;	
	int datasize=0;//128k	//4k//new
	int sectorcount=256;//64 page//4k	

	datasize=flash_type.size_block * 1024;//new
	DATABuffer=(PUCHAR) malloc(datasize);
	
	for(i=0;i<datasize;i++)
	{
		DATABuffer[i]=0x00;		
	}

	FILE *fp;
	CString strDldFile="";	
	int readnum;
	int line;

	CFileDialog dlgLoad(
		TRUE, 0, 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"Intel Hex(*.b)|*.b||");
	dlgLoad.m_ofn.lpstrTitle = "Anchor Download";
	dlgLoad.m_ofn.lpstrInitialDir = strDldFile; // OK if you specify a file
	if(dlgLoad.DoModal() != IDOK)
	{
	
		AddInfo("****User Canceled download Fw!!!\r\n");
		free(DATABuffer);
		return 0;		
	}		
	strDldFile = dlgLoad.m_ofn.lpstrFile;
//	if(strDldFile.Find(".b") != -1) //intel hex format
//	{
		fp = fopen(strDldFile, "rb");
		if(fp)
		{
			line=0;
			while(1)
			{
				readnum=fread(DATABuffer+32*line, 1, 32, fp);
				if(readnum==0)
				{
					break;
				}				
				line++;
			}
		
		}
		else
		{
			m_result+="打开.b文件失败!";			
			free(DATABuffer);
			return 0;
		}
		fclose(fp);

	unsigned char data[8];	
	for(i=0;i<datasize/2;i++)
	{
		data[0]=DATABuffer[2*i];
		DATABuffer[2*i]=DATABuffer[2*i+1];
		DATABuffer[2*i+1]=data[0];
	}

	if(SLWWritePhyBlock(device,DATABuffer,0,0,0,Multiblock[device],0)==0)
	{
		m_result+="WriteCodeBlock WritePhyBlock fail!!!\r\n";
		free(DATABuffer);
		return 0;
	}

	free(DATABuffer);
	return 1;
}

BOOL SLWWritesmartInfo(int device,PUCHAR DATABuffer,long offset)		// 没有注释
{
	int i;
	long addr;
	addr=offset;
	DATABuffer[addr++]=0x10;
	DATABuffer[addr++]=0x00;
	DATABuffer[addr++]=0x01;
	DATABuffer[addr++]=0x03;
	DATABuffer[addr++]=0x00;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x00;
	for(i=8;i<14;i++)
	{
		DATABuffer[addr++]=0x00;
	}

	DATABuffer[addr++]=0x02;
	DATABuffer[addr++]=0x03;
	DATABuffer[addr++]=0x00;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x00;
	for(i=20;i<26;i++)
	{
		DATABuffer[addr++]=0x00;
	}

	DATABuffer[addr++]=0xbf;
	DATABuffer[addr++]=0x02;
	DATABuffer[addr++]=0x00;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x00;
	for(i=32;i<38;i++)
	{
		DATABuffer[addr++]=0x00;
	}

	DATABuffer[addr++]=0xc0;
	DATABuffer[addr++]=0x02;
	DATABuffer[addr++]=0x00;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x00;
	for(i=44;i<50;i++)
	{
		DATABuffer[addr++]=0x00;
	}

	DATABuffer[addr++]=0xc2;
	DATABuffer[addr++]=0x02;
	DATABuffer[addr++]=0x00;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x28;
	for(i=56;i<62;i++)
	{
		DATABuffer[addr++]=0x00;
	}

	DATABuffer[addr++]=0xc5;
	DATABuffer[addr++]=0x02;
	DATABuffer[addr++]=0x00;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x00;
	for(i=68;i<74;i++)
	{
		DATABuffer[addr++]=0x00;
	}

	DATABuffer[addr++]=0xc6;
	DATABuffer[addr++]=0x02;
	DATABuffer[addr++]=0x00;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x00;
	for(i=80;i<86;i++)
	{
		DATABuffer[addr++]=0x00;
	}

	DATABuffer[addr++]=0xc7;
	DATABuffer[addr++]=0x02;
	DATABuffer[addr++]=0x00;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x64;
	DATABuffer[addr++]=0x00;
	for(i=92;i<98;i++)
	{
		DATABuffer[addr++]=0x00;
	}

	for(i=98;i<362;i++)
	{
		DATABuffer[addr++]=0x00;
	}
	DATABuffer[addr++]=0x00;
	DATABuffer[addr++]=0x00;
	DATABuffer[addr++]=0xb8;
	DATABuffer[addr++]=0x01;
	DATABuffer[addr++]=0x00;
	DATABuffer[addr++]=0x7b;
	DATABuffer[addr++]=0x02;
	DATABuffer[addr++]=0x00;
	DATABuffer[addr++]=0x01;
	DATABuffer[addr++]=0x00;
	DATABuffer[addr++]=0x02;
	DATABuffer[addr++]=0x3c;
	DATABuffer[addr++]=0x02;
	for(i=375;i<386;i++)
	{
		DATABuffer[addr++]=0x00;
	}
	for(i=386;i<511;i++)
	{
		DATABuffer[addr++]=0x00;
	}
	int checksum=0;
	for(i=0;i<511;i++)
	{
		checksum+=DATABuffer[addr-i];
	}
	DATABuffer[addr++]=~checksum+1;

	
	for(i=0;i<512;i++)
	{
		DATABuffer[addr+i]=0x00;
	}
	DATABuffer[addr+0]=0x10;
	DATABuffer[addr+1]=0x00;
	DATABuffer[addr+2]=0x01;
	DATABuffer[addr+3]=0x00;

	DATABuffer[addr+14]=0x02;
	DATABuffer[addr+15]=0x00;

	DATABuffer[addr+26]=0xbf;
	DATABuffer[addr+27]=0x0a;

	DATABuffer[addr+38]=0xc0;
	DATABuffer[addr+39]=0x0a;

	DATABuffer[addr+50]=0xc2;
	DATABuffer[addr+51]=0x46;

	DATABuffer[addr+62]=0xc5;
	DATABuffer[addr+63]=0x00;

	DATABuffer[addr+74]=0xc6;
	DATABuffer[addr+75]=0x0a;

	DATABuffer[addr+86]=0xc7;
	DATABuffer[addr+87]=0x00;

	checksum=0;
	for(i=0;i<511;i++)
	{
		checksum+=DATABuffer[addr+i];
	}
	DATABuffer[addr+511]=~checksum+1;


	return 1;
}

/*在初始化的最后写入全局信息表和映射表*/
BOOL SLWWriteGMBlock(int device,int Initnum)
{
	int ret,i;
	DWORD num;
	MapBuffer[device][80]=MinReserveblock[device];
	int ichannel,irow;
	DWORD dblock;
	for(i=0;i<channel;i++)
	{
		ichannel = i;
		irow = DIEMAP[0];
		DWORD block_add;
		dblock=mapblock[device][0][i];
		block_add = 8187<< 18 | mapblock[device][0][i] << 5| irow;	// LBN + strip size
		VNDSetBlockArg(device,0,mapsize[device]/channel,0);
		ret = VNDWriteblock(device,MapBuffer[device],ichannel,block_add,mapsize[device]);
		if(ret ==-1)//write map to flash block
		{
			AddInfo("Write Map fail!\r\n");
			LogInfo(device,"Write Map fail!\r\n",3);
			free(MapBuffer[device]);
			SLWUpdateBadblock(device,ichannel,irow,dblock);
			Initnum++;
			SLWInitial(device,Initnum);
			return 1;
		}
		else if(ret==-2)
		{
			free(MapBuffer[device]);
			return 0;
		}
	}
	PUCHAR buffer;
	buffer=(PUCHAR)malloc(mapsize[device]*512);
	for(i=0;i<channel;i++)
	{
		ichannel = i;
		irow = DIEMAP[0];
		DWORD block_add;
		memset(buffer, 255, mapsize[device]*512);
		block_add = 8187<< 18 |  mapblock[device][0][i] << 5| irow;	// LBN + strip size
		VNDSetBlockArg(device,0,mapsize[device]/channel,0);
		ret= VNDReadblock(device,buffer,ichannel,irow,mapblock[device][0][i]);
		if(ret ==0)//write map to flash block
		{
			free(buffer);
			free(MapBuffer[device]);
			AddInfo("Read Map fail!\r\n");
			LogInfo(device,"Read Map fail!\r\n",3);
			return 0;
		}
	}


	if( !DMA_OPS(device,buffer, mapsize[device]*512, 0x1000, false, ata_READ_DMA))
	{	
		free(buffer);
		free(MapBuffer[device]);
		LogInfo(device,"DMA  fail!\r\n",3);
		return 0;
	}

	for(num=0;num<mapsize[device]*512;num++)
	{
		if(MapBuffer[device][num]!=buffer[num])
		{
			AddInfo("****Compare Map info failed!!!");	
			LogInfo(device,"Compare Map info failed!\r\n",3);
			free(buffer);
			free(MapBuffer[device]);
			return 0;
		}
	}

	free(buffer);
	free(MapBuffer[device]);
	AddInfo("Write Map info block successfully!\r\n");

	dblock=globleblock[device];
	ichannel=0;
	irow = DIEMAP[0];
	DWORD block_add;
	block_add = 8182<< 18 | dblock << 5| irow;	// LBN + strip size
	VNDSetBlockArg(device,0,sectorperblock,7);
	ret=VNDWriteblock(device,GlobelBuffer[device],ichannel,block_add,sectorperblock);
	if(ret==-1)
	{
		AddInfo("Write global info block fail!\r\n");
		LogInfo(device,"Write global info block failed!\r\n",3);
		SLWUpdateBadblock(device,ichannel,irow,dblock);
		Initnum++;
		SLWInitial(device,Initnum);
		free(GlobelBuffer[device]);
		return 1;
	}
	else if(ret==-2)
	{
		free(GlobelBuffer[device]);
		return 0;
	}

	buffer=(PUCHAR)malloc(sectorperblock*512);
	memset(buffer, 255, sectorperblock*512);
	ichannel = 0;
	irow = DIEMAP[0];

	block_add = 8182<< 18 |  dblock << 5| irow;	// LBN + strip size
	VNDSetBlockArg(device,0,sectorperblock,0);
	ret= VNDReadblock(device,buffer,ichannel,irow,dblock);
	if(ret ==0)//write map to flash block
	{
		free(buffer);
		free(GlobelBuffer[device]);
		AddInfo("Read global info fail!\r\n");	
		LogInfo(device,"Read global info block failed!\r\n",3);
		return 0;
	}

/*
	if( !DMA_OPS(device,buffer, sectorperblock*512, 0, false, ata_READ_DMA))
	{
		free(buffer);
		free(GlobelBuffer[device]);
		LogInfo(device,"DMA global info block failed!\r\n",3);
		return 0;
	}
*/

	for(num=0;num<sectorperblock*512;num++)
	{
		if(GlobelBuffer[device][num]!=buffer[num])
		{
			AddInfo("****Compare global info failed!!!");	
			LogInfo(device,"Compare global info block failed!\r\n",3);
			free(buffer);
			free(GlobelBuffer[device]);
			return 0;
		}
	}

	free(buffer);
	free(GlobelBuffer[device]);
	AddInfo("Write global info block successfully!\r\n");
	return 1;
}

BOOL SLWWriteMultiboot(int device,int Initnum)
{
	int ret,i,findkey=0;
	PUCHAR buf,buffer;
	char readchar[256];
	int ichannel,irow;
	DWORD dblock;	
	int datasize=flash_type.size_block * 1024;
	buf=(PUCHAR) malloc(datasize);
	for(i=0;i<datasize;i++)
		buf[i]=0xff;
	
	CString strLine="",str,filename;
	char file[128];
    GetModuleFileName(NULL,file,128); 
    //Scan a string for the last occurrence of a character.
    (strrchr(file,'\\'))[1] = 0; 
	
	strcat(file,"BadblockFile\\");
    strcat(file,BadBlockFilename[device]);
	filename=file;
	if( filename.Right(4) != ".bck")
		filename += ".bck";	
	strcpy(file,filename);
	FILE *fp;
#if _INITD
	if(IniMode==0)
		strcpy(file,SelectBadblockfile);
#endif
	fp = fopen(file, "rb");
	if(fp==NULL)
		return 0;
    while(fgets(readchar,256,fp)!=NULL)
	{
		if(strstr(readchar,"key:"))
		{
			findkey=1;
			break;
		}
	}
	if(findkey==1)                     
	{
		for(i=0;i<512;i++)
		{
			fscanf(fp,"%X ",&buf[i]);
		}
		fclose(fp);
	}
	else
	{		
		fclose(fp);

		time_t ltime;
		unsigned int tmp;
		time( &ltime );
		srand( (unsigned)ltime);

		for(i=0;i<512;)
		{		
			tmp=rand();
			buf[i++]=(unsigned char)(tmp);
			buf[i++]=(unsigned char)(tmp >> 8);
		}
		SYSTEMTIME   st;	 
		GetLocalTime(&st);
		srand( (unsigned)st.wMilliseconds);
		for(i=0;i<512;)
		{		
			tmp=rand();
			buf[i++]+=(unsigned char)(tmp);
			buf[i++]+=(unsigned char)(tmp >> 8);
		}

		CStdioFile fileNew(file,CFile::modeReadWrite);
		fileNew.SeekToEnd();
		fileNew.WriteString("\r\n\r\nkey:\r\n");
		for(i = 0;i< 512;i++)
		{		
			//line info
			if ( ((i+1)%16) == 0)
				str.Format("%X\r\n",buf[i]);
			else
				str.Format("%X ",buf[i]);
			strLine+=str;		
			fileNew.WriteString(strLine);
			strLine="";
		}
		fileNew.Close();
	}

	dblock=Multiblock[device];
	ichannel=1;
	irow = DIEMAP[0];
	DWORD block_add,num;
	block_add = 8189<< 18 | dblock << 5| irow;	// LBN + strip size
	VNDSetBlockArg(device,0,sectorperblock,7);
	ret=VNDWriteblock(device,buf,ichannel,block_add,sectorperblock);
	if(ret==-1)
	{
		AddInfo("Write Multiboot block fail!\r\n");
		SLWUpdateBadblock(device,ichannel,irow,dblock);
		Initnum++;
		SLWInitial(device,Initnum);
		free(buf);
		return 1;
	}
	else if(ret==-2)
	{
		free(buf);
		return 0;
	}

	buffer=(PUCHAR)malloc(sectorperblock*512);
	memset(buffer, 255, sectorperblock*512);

	VNDSetBlockArg(device,0,sectorperblock,0);
	ret= VNDReadblock(device,buffer,ichannel,irow,dblock);
	if(ret ==0)//write map to flash block
	{
		free(buffer);
		free(buf);
		AddInfo("Read Mutiboot info fail!\r\n");		
		return 0;
	}

/*
	if( !DMA_OPS(device,buffer, sectorperblock*512, 0, false, ata_READ_DMA))
	{
		free(buffer);
		free(buf);
		return 0;
	}
*/
	for(num=0;num<sectorperblock*512;num++)
	{
		if(buf[num]!=buffer[num])
		{
			AddInfo("****Compare Multiboot info failed!!!");	
			free(buffer);
			free(buf);
			return 0;
		}
	}

	free(buffer);
	free(buf);
	AddInfo("Write Multiboot block successfully!\r\n");
	return 1;

}

BOOL SLWWriteMapTableToFLASH(int device,int Initnum)
{
	int i=0;
	int j=0;
	int k=0;
	int l=0;
	int m=0;
	int n=0;
	int a=0;
	int datasize=0;
	CString str;
	int mapblocknum=0;	
	mapblocknum= (channel*gldie*2*flash_type.blocknum*15/16)/(1024*flash_type.size_block);//new , 2bytes for a block address, need x blocks ?
	if(((channel*gldie*flash_type.blocknum*15/16)%(1024*flash_type.size_block))!=0)
		mapblocknum+=1;

	datasize=mapblocknum*flash_type.size_block * 1024;
	MapBuffer[device]=(PUCHAR) malloc(datasize);
	for(i=0;i<datasize;i++)
	{
		MapBuffer[device][i]=0xff;
	}

	for(i=0;i<plane;i++)
		for(j=0;j<channel*gldie;j++)
			replaceblocknum[device][i][j]=0;
	
	int addr = 0x20;

	for(i=0;i<76;i++)
		MapBuffer[device][addr++]=0;
	
	if(PurgeFlag)
	{
		MapBuffer[device][88]=0x2;	//自毁或者清空进行中的状态,当初始化时写入0x8002，SSD在loadfw运行后，进清空数据，可以用在写全盘
		MapBuffer[device][89]=0x80;	//自毁或者清空进行中的状态,当初始化时写入0x8002，SSD在loadfw运行后，进清空数据，可以用在写全盘
	}

	//交互块表
	addr = 512; 
	for(k=0;k<plane;k++)
	{
		for(m=0;m<grdie;m++)
		{
			for(n=0;n<channel;n++)
			{
				MapBuffer[device][addr+(2*channel*grdie)*k+2*channel*m+2*n]=(unsigned char)(swapblock[device][k][n*gldie+DIEMAP[m]]);
				MapBuffer[device][addr+(2*channel*grdie)*k+2*channel*m+2*n+1]=(unsigned char)(swapblock[device][k][n*gldie+DIEMAP[m]]>>8);
			}
		}
	}
	
	addr = 16384;
	for(j=0;j<(flash_type.blocknum/plane-flash_type.blocknum/32);j++)//2048-128组映射表  (以plane为单位映射，现在每个plane的block 数为 2048 或 1024)
	{
		for(k=0;k<plane;k++)
		{
			for(m=0;m<grdie;m++)
			{
				for(n=0;n<channel;n++)
				{
					//////分配一个好的endblock	

					for(a=0;a<flash_type.blocknum/32;a++)
					{
						if(CompareBadblock(device,endblock[device][k][n*gldie+DIEMAP[m]],n,DIEMAP[m])==0)//以物理die,block比较
							endblock[device][k][n*gldie+DIEMAP[m]]-=step;
						else
							break;
					}

					if(CompareBadblock(device,datablock[device][k][n*gldie+DIEMAP[m]],n,DIEMAP[m])==0)
					{
						MapBuffer[device][addr+(2*channel*grdie*plane)*j+(2*channel*grdie)*k+2*channel*m+2*n]=(unsigned char)(endblock[device][k][n*gldie+DIEMAP[m]]);
						MapBuffer[device][addr+(2*channel*grdie*plane)*j+(2*channel*grdie)*k+2*channel*m+2*n+1]=(unsigned char)(endblock[device][k][n*gldie+DIEMAP[m]]>>8);
						replaceblock[device][k][n*gldie+DIEMAP[m]][replaceblocknum[device][k][n*gldie+DIEMAP[m]]]=datablock[device][k][n*gldie+DIEMAP[m]];
						replaceblocknum[device][k][n*gldie+DIEMAP[m]]++;
						datablock[device][k][n*gldie+DIEMAP[m]]+=step;
						endblock[device][k][n*gldie+DIEMAP[m]]-=step;							
						
					}
					else
					{
						MapBuffer[device][addr+(2*channel*grdie*plane)*j+(2*channel*grdie)*k+2*channel*m+2*n]=(unsigned char)(datablock[device][k][n*gldie+DIEMAP[m]]);
						MapBuffer[device][addr+(2*channel*grdie*plane)*j+(2*channel*grdie)*k+2*channel*m+2*n+1]=(unsigned char)(datablock[device][k][n*gldie+DIEMAP[m]]>>8);
						datablock[device][k][n*gldie+DIEMAP[m]]+=step;		
			/*			if(n==0 && m==0&&k==0)
						{
							int block;
							block = DataBuffer[(2*channel*grdie*plane)*j+(2*channel*grdie)*k+2*channel*m+2*n]|(DataBuffer[(2*channel*grdie*plane)*j+(2*channel*grdie)*k+2*channel*m+2*n+1] << 8);
							str.Format("block = %d,loginblock = %d %d %d %d ",block,(2*channel*grdie*plane)*j+(2*channel*grdie)*k+2*channel*m+2*n,n,m,k);
							AddInfo(str);
						}*/
					}
				}
			}
		}
	}
/*
	CString strLine="";
	CStdioFile fileNew("maptable.txt",CFile::modeCreate|CFile::modeReadWrite);
		
	for(i = 0;i< 512*1024;i++)
	{
		
		//line info
		if ( ((i+1)%16) == 0)
			str.Format("%X\r\n",MapBuffer[device][i]);
		else
			str.Format("%X ",MapBuffer[device][i]);
		strLine+=str;

	
		fileNew.WriteString(strLine);
		strLine="";
	}

	fileNew.Close();
*/
	return 1;
}

BOOL SLWWritesecurityBlock(int device)
{
	int i;
	//char output[256];
	PUCHAR DATABuffer;
	int datasize=flash_type.size_block * 1024;
	DATABuffer=(PUCHAR) malloc(datasize);

	for(i=0;i<datasize;i++)
	{
		DATABuffer[i]=0xff;		
	}

	for(i=0;i<104;i++)
	{
		DATABuffer[i]=0x00;		
	}
	DATABuffer[0]=0x01;
	DATABuffer[6]=0x05;

	if(SLWWritePhyBlock(device,DATABuffer,3,0,0,securityblock[device],1)==0)
	{
		m_result+="WritePhyBlock fail!\r\n";
		free(DATABuffer);
		return 0;
	}	
	
	free(DATABuffer);	
	return 1;
}

BOOL SLWWritePhyBlock(int device,PUCHAR DATABuffer,int channelnum,int rownum,int layernum,int blocknum,int elbn)
{
	int ichannel,irow;
	DWORD dblock;

	ichannel=channelnum;
	irow=rownum;
	dblock=blocknum;

	int CBWBufferSize = 31;
	int CSWBufferSize = 13;
	unsigned char CBWBuffer[31];
	unsigned char CSWBuffer[13];
	
	PUCHAR ReadBuffer;
	int i=0;
	int j=0;
	int k=0;

	int m_datasize = 64*2048;	//128k//4k
	int datasize   = 0;			//128k	//4k//new
	int sectorcount= 256;		//64 page//4k

	

	for(i=0;i<CBWBufferSize;i++)
	{
		CBWBuffer[i]=0x00;		
	}
	for(i=0;i<CSWBufferSize;i++)
	{
		CSWBuffer[i]=0x00;		
	}	

	datasize=flash_type.size_block * 1024;//new
	
	ReadBuffer=(PUCHAR) malloc(datasize);
	
	for(i=0;i<datasize;i++)
	{
		ReadBuffer[i]=0xff;		
	}

	DWORD LBAADD;
	((BYTE *) &LBAADD)[0]=(unsigned char)dblock;
	((BYTE *) &LBAADD)[1]=(unsigned char)((dblock>>8)|(elbn<<6));
	((BYTE *) &LBAADD)[2]=(unsigned char)((irow<<4)|(ichannel<<2));
	((BYTE *) &LBAADD)[3]=(unsigned char)(0x02);

	if( !DMA_OPS(device,DATABuffer, 256 * 512, LBAADD, true, ata_WRITE_DMA))
	{
		AddInfo("\r\n\r\n****Write_DMA first 256 SECTORS 失败!!!\r\n\r\n");			
		free(ReadBuffer);	
		return 0;	
	}

	if(blocksize>1)//new
	{
		((BYTE *) &LBAADD)[0]=(unsigned char)dblock;
		((BYTE *) &LBAADD)[1]=(unsigned char)((dblock>>8)|(elbn<<6));
		((BYTE *) &LBAADD)[2]=(unsigned char)((irow<<4)|(ichannel<<2)|(1));
		((BYTE *) &LBAADD)[3]=(unsigned char)(0x02);

		if( !DMA_OPS(device,DATABuffer+32*4096, 256 * 512, LBAADD, true, ata_WRITE_DMA))
		{
			AddInfo("\r\n\r\n****Write_DMA second 256 SECTORS 失败!!!\r\n\r\n");			
			free(ReadBuffer);	
			return 0;	
		}
	}

	if(blocksize>2)//new
	{
		((BYTE *) &LBAADD)[0]=(unsigned char)dblock;
		((BYTE *) &LBAADD)[1]=(unsigned char)((dblock>>8)|(elbn<<6));
		((BYTE *) &LBAADD)[2]=(unsigned char)((irow<<4)|(ichannel<<2)|(2));
		((BYTE *) &LBAADD)[3]=(unsigned char)(0x02);


		if( !DMA_OPS(device,DATABuffer+64*4096, 256 * 512, LBAADD, true, ata_WRITE_DMA))
		{
			AddInfo("\r\n\r\n****Write_DMA third 256 SECTORS 失败!!!\r\n\r\n");			
			free(ReadBuffer);	
			return 0;	
		}

		((BYTE *) &LBAADD)[0]=(unsigned char)dblock;
		((BYTE *) &LBAADD)[1]=(unsigned char)((dblock>>8)|(elbn<<6));
		((BYTE *) &LBAADD)[2]=(unsigned char)((irow<<4)|(ichannel<<2)|(3));
		((BYTE *) &LBAADD)[3]=(unsigned char)(0x02);

		if( !DMA_OPS(device,DATABuffer+96*4096, 256 * 512, LBAADD, true, ata_WRITE_DMA))
		{
			AddInfo("\r\n\r\n****Write_DMA fourth 256 SECTORS 失败!!!\r\n\r\n");			
			free(ReadBuffer);	
			return 0;	
		}
		
	}

/*
	if(SLWIDERead(ReadBuffer)==0)
	{
		AddInfo("\r\n\r\n****Read data back 失败!!!\r\n\r\n");		
		free(ReadBuffer);
				
		return 0;
	}
	for(i=0;i<datasize;i++)
	{
		if(ReadBuffer[i]!=DATABuffer[i])
		{
			AddInfo("\r\n\r\n****IDERead数据比较失败!!!\r\n\r\n");		
			free(ReadBuffer);
					
			return 0;
		}
	}

*/	
	free(ReadBuffer);	
	return 1;
}

BOOL SLWRWSPI(int device,int sector,PUCHAR DATABuffer,int rw)
{
	int CBWBufferSize = 31;
	int CSWBufferSize = 13;
	unsigned char CBWBuffer[31];
	unsigned char CSWBuffer[13];	
	int i;

	int m_datasize=512*128;//512
	int sectorcount=128;//1 page	

	for(i=0;i<CBWBufferSize;i++)
	{
		CBWBuffer[i]=0x00;		
	}
	for(i=0;i<CSWBufferSize;i++)
	{
		CSWBuffer[i]=0x00;		
	}	

	DWORD LBAADD;
	((BYTE *) &LBAADD)[0]=(unsigned char)(sector);
	((BYTE *) &LBAADD)[1]=(unsigned char)0;
	((BYTE *) &LBAADD)[2]=(unsigned char)(rw);
	((BYTE *) &LBAADD)[3]=(unsigned char)(0x04);

	if(rw)
	{
		if( !DMA_OPS(device,DATABuffer, 128 *512, LBAADD, true, ata_WRITE_DMA_EXT))
		{
			AddInfo("SLWRWSPI Write data fail!");
			return 0;
		}
	}
	else
	{
		if( !DMA_OPS(device,DATABuffer, 128 *512, LBAADD, false, ata_READ_DMA_EXT))
		{
			AddInfo("SLWRWSPI Read data fail!");
			return 0;
		}
	}
	return 1;
}

long SLWGlobalinfo(int device,PUCHAR buf,int Initnum,long offset,BOOL CodeFlag)
{
	PUCHAR DATABuffer;	
	long datasize=64*2048*(blocksize);//new
	if(blocksize==0)
		datasize=64*2048*4;
	DATABuffer=(PUCHAR) malloc(datasize);
	int i;
	FILE *fp;
	CString strDldFile="";	
	int readnum;
	long line;
	
	for(i=0;i<datasize;i++)
	{
		DATABuffer[i]=0x00;		
	}

#if _INITD 
	if(Initnum==0)
	{
		CFileDialog dlgLoad(
			TRUE, 0, 0,
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
			"(*.b)|*.b|AllFiles (*.*)|*.*||");
		if(CodeFlag==0)
			dlgLoad.m_ofn.lpstrTitle = "Select FW code";
		else
			dlgLoad.m_ofn.lpstrTitle = "Select Rebuild code";
		dlgLoad.m_ofn.lpstrInitialDir = strDldFile; // OK if you specify a file
		if(dlgLoad.DoModal() != IDOK)
		{
			free(DATABuffer);
			return 0;		
		}
		
		strDldFile = dlgLoad.m_ofn.lpstrFile;	
		if(CodeFlag==0)
			FWFilename = strDldFile;
		else
			RebuildFilename = strDldFile;
	}
	else
		strDldFile=FWFilename;

#endif

#if _UPDATE
	if(CodeFlag==0)
		strDldFile = FWFilename ;
	else
		strDldFile = RebuildFilename ;
#endif

#if _MASSINIT 
	char filename[128];
	GetModuleFileName(NULL,filename,128); 
	(strrchr(filename,'\\'))[1] = 0;
	
	if(CodeFlag==0)
		strDldFile = filename+FWFilename ;
	else
		strDldFile = filename+RebuildFilename ;
#endif

	if(strDldFile.Find(".b") != -1) //intel hex format
	{
		fp = fopen(strDldFile, "rb");
		if(fp)
		{
			line=0;
			while(1)
			{
				readnum=fread(DATABuffer+32*line, 1, 32, fp);
				if(readnum==0)
				{
					break;
				}				
				line++;
			}
		
		}
		else
		{
			AddInfo("打开.b文件失败!");
			free(DATABuffer);
			return 0;
		}
		fclose(fp);
	}
	else
	{
		AddInfo("找不到.b文件!");
		free(DATABuffer);
		return 0 ;
	}
	unsigned char data[8];	
	for(i=0;i<datasize/2;i++)
	{
		data[0]=DATABuffer[2*i];
		DATABuffer[2*i]=DATABuffer[2*i+1];
		DATABuffer[2*i+1]=data[0];
	}

	line = line*32;
	long addptr=offset;			//code block
	for(i=0;i<line;i++)
	{
		buf[addptr++] = DATABuffer[i];
	}

	return line;
}


BOOL SLWReadfpga(int device)
{
	int lba = 0;
	int lbalen = 128;	
	int datasize=1024*1024;
	int i;

	PUCHAR DataBuffer;
	DataBuffer=(PUCHAR)malloc(datasize);
	CStdioFile   file;
	
	for(i=0;i<datasize;i++)
	{
		DataBuffer[i]=0;
	}

	for(lba=0;lba<16;lba++)
	{
		if(VNDReadSPI(device,(BYTE*)&DataBuffer[lba*65536],lba)==0)
		{
			AddInfo("SLWVNDCMD fail!");
			free(DataBuffer);
			return 0;
		}
#if _INITD 
		pGuiServerExplorer->m_progress.SetPos(100*lba/16);
#endif
	}
	SLWSaveFile(DataBuffer,datasize,0);
	free(DataBuffer);
	return 1;
}

BOOL SLWWritefpga(int device)
{
	int lba = 0;
	int lbalen = 128;	
	CString result;
	int datasize=16*128*512;
	int readsize=128*512;
	unsigned char data;
	int i,j;

	PUCHAR DataBuffer;
	PUCHAR ReadBuffer;
	
	DataBuffer=(PUCHAR)malloc(datasize);
	ReadBuffer=(PUCHAR)malloc(readsize);
	
	for(i=0;i<datasize;i++)
	{
		DataBuffer[i]=0xff;
	}
	for(i=0;i<readsize;i++)
	{
		ReadBuffer[i]=0x00;
	}

	char str[256];
	int recType;	
	int CNTFIELD=1;
	int ADDRFIELD=3;
	int RECFIELD=7;
	int DATAFIELD=9;	
	int crt_len=0;
	DWORD crt_add=0;
	int	len=0;	
	DWORD baseadd=0;
	CString strDldFile="",str1;	

#if _INITD 
	CFileDialog dlgLoad(
		TRUE, 0, 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"PROM (*.mcs)|*.mcs||");
	dlgLoad.m_ofn.lpstrTitle = "Select FPGA";
	dlgLoad.m_ofn.lpstrInitialDir = strDldFile; // OK if you specify a file
	if(dlgLoad.DoModal() != IDOK)
	{
		AddInfo("__User Cancled__");
		free(DataBuffer);
		free(ReadBuffer);
		return 0;		
	}		
	strDldFile = dlgLoad.m_ofn.lpstrFile;
#endif

#if _MASSINIT 
	char filename[128];
	GetModuleFileName(NULL,filename,128); 
	(strrchr(filename,'\\'))[1] = 0; 
	strDldFile = filename+FPGAFilename;
#endif
	
	FILE *fp = fopen(strDldFile, "rb");		
	if(fp)
	{			
		while(fgets(str,256,fp))
		{
			if(str[0]!=':')
			{
				fclose(fp);
				AddInfo("hex文件格式不正确!\r\n");
				free(DataBuffer);
				free(ReadBuffer);
				return 0;
			}
			sscanf(str+RECFIELD,"%2x",&recType);
			if((recType==4)||(recType==2))
			{
				sscanf(str+DATAFIELD,"%4x",&baseadd);
				baseadd<<=16;
				len=0;
				
			}
			else if(recType==1)
			{									
				break;
			}
			else if(recType==0)
			{
				sscanf(str+CNTFIELD,"%2x",&crt_len);
				sscanf(str+ADDRFIELD,"%4x",&crt_add);					
				
				for(j=0;j<crt_len;j++)
				{						
					sscanf(str+DATAFIELD+j*2,"%2x",&data);
					DataBuffer[baseadd+len]=data;
					len++;						
				}
			}
			else
			{
				fclose(fp);
				AddInfo("mcs文件格式不正确!\r\n");
				free(DataBuffer);
				free(ReadBuffer);
				return 0;
			}
		}
		fclose(fp);
	}
	else
	{
		AddInfo("打开.mcs文件失败!\r\n");		
		free(DataBuffer);
		free(ReadBuffer);
		return 0;
	}

	for(lba=0;lba<12;lba++)
	{
		result.Format("Write sector %d...\r\n",lba);
		AddInfo(result);
		if(VNDWriteSPI(device,DataBuffer+lba*128*512,0,lba)==0)
		{
			AddInfo("**Write FPGA Failed**");
			free(DataBuffer);
			free(ReadBuffer);
			return 0;
		}	
		Sleep(200);
#if _INITD 
		pGuiServerExplorer->m_progress.SetPos(50*lba/12);
#endif
#if _MASSINIT
		str1.Format("%d",50*lba/12);
		str1+="%";
		pSoliWareMPToolDlg->m_GridCtrl.SetItemText(device+1,4,str1);
		pSoliWareMPToolDlg->m_GridCtrl.Refresh();
#endif
	}

#if _INITD 
		pGuiServerExplorer->m_progress.SetPos(0);
#endif

	for(lba=0;lba<12;lba++)
	{
		result.Format("Read sector %d...\r\n",lba);
		AddInfo(result);
		if(VNDReadSPI(device,ReadBuffer,lba)==0)
		{
			AddInfo("**Read FPGA BACK Failed**");
			free(ReadBuffer);
			free(DataBuffer);
			return 0;
		}
/*		for(i=0;i<64*512;i++)
		{
			data=ReadBuffer[2*i];
			ReadBuffer[2*i]=ReadBuffer[2*i+1];
			ReadBuffer[2*i+1]=data;
		}*/
		for(i=8;i<128*512;i++)
		{
			if(ReadBuffer[i]!=DataBuffer[lba*128*512+i])
			{
				AddInfo("SLWRWSPI data compare fail!");
				result.Format("ReadBuffer[%d]=%02x,DataBuffer[%d]=%02x",i,ReadBuffer[i],lba*128*512+i,DataBuffer[lba*128*512+i]);
				AddInfo(result);
				free(ReadBuffer);
				free(DataBuffer);
				return 0;
			}
		}
#if _INITD 
		pGuiServerExplorer->m_progress.SetPos(50+50*lba/12);
#endif
#if _MASSINIT
		str1.Format("%d",50+50*lba/12);
		str1+="%";
		pSoliWareMPToolDlg->m_GridCtrl.SetItemText(device+1,4,str1);
		pSoliWareMPToolDlg->m_GridCtrl.Refresh();
#endif
	}

	free(DataBuffer);
	free(ReadBuffer);
	return 1;

}

BOOL SLWWritefpgaDr(int device)
{
	int lba = 0;
	int lbalen = 128;	
	CString result;
	int datasize=1024*1024;
	int readsize=1024*1024;
	unsigned char data[128];
	int i,j,ret;

	PUCHAR DataBuffer;
	PUCHAR ReadBuffer;
	
	DataBuffer=(PUCHAR)malloc(datasize);
	ReadBuffer=(PUCHAR)malloc(readsize);
	memset(data,0,128);
	
	for(i=0;i<datasize;i++)
	{
		DataBuffer[i]=0xff;
	}
	for(i=0;i<readsize;i++)
	{
		ReadBuffer[i]=0xff;
	}

	char str[256];
	int recType;	
	int CNTFIELD=1;
	int ADDRFIELD=3;
	int RECFIELD=7;
	int DATAFIELD=9;	
	int crt_len=0;
	DWORD crt_add=0;
	int	len=0;	
	DWORD baseadd=0;
	CString strDldFile="";	
	CFileDialog dlgLoad(
		TRUE, 0, 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"PROM (*.mcs)|*.mcs||");
	dlgLoad.m_ofn.lpstrTitle = "Select FPGA";
	dlgLoad.m_ofn.lpstrInitialDir = strDldFile; // OK if you specify a file
	if(dlgLoad.DoModal() != IDOK)
	{
		AddInfo("__User Cancled__");
		free(DataBuffer);
		free(ReadBuffer);
		return 0;		
	}		
	strDldFile = dlgLoad.m_ofn.lpstrFile;
	
	FILE *fp = fopen(strDldFile, "rb");		
	if(fp)
	{			
		while(fgets(str,256,fp))
		{
			if(str[0]!=':')
			{
				fclose(fp);
				AddInfo("hex文件格式不正确!\r\n");
				free(DataBuffer);
				free(ReadBuffer);
				return 0;
			}
			sscanf(str+RECFIELD,"%2x",&recType);
			if((recType==4)||(recType==2))
			{
				sscanf(str+DATAFIELD,"%4x",&baseadd);
				baseadd<<=16;
				len=0;
				
			}
			else if(recType==1)
			{									
				break;
			}
			else if(recType==0)
			{
				sscanf(str+CNTFIELD,"%2x",&crt_len);
				sscanf(str+ADDRFIELD,"%4x",&crt_add);					
				
				for(j=0;j<crt_len;j++)
				{						
					sscanf(str+DATAFIELD+j*2,"%2x",&data[0]);
					DataBuffer[baseadd+len]=data[0];
					len++;						
				}
			}
			else
			{
				fclose(fp);
				AddInfo("mcs文件格式不正确!\r\n");
				free(DataBuffer);
				free(ReadBuffer);
				return 0;
			}
		}
		fclose(fp);
	}
	else
	{
		AddInfo("打开.mcs文件失败!\r\n");		
		free(DataBuffer);
		free(ReadBuffer);
		return 0;
	}


	if(VNDDriveWSPI(device,0,DataBuffer,208)==0)
	{
		AddInfo("**Write FPGA Failed**");
		free(DataBuffer);
		free(ReadBuffer);
		return 0;
	}	
	Sleep(200);

	if(InterfaceType[device]==1)
		ret=VNDDriveRSPI48b(device,0,ReadBuffer,208);
	else
		ret=VNDDriveRSPI(device,0,ReadBuffer,208);
	if(ret==0)
	{
		AddInfo("**Read FPGA Failed**");
		free(ReadBuffer);
		free(DataBuffer);
		return 0;
	}
	for(i=0;i<datasize/2;i++)
	{
		data[0]=ReadBuffer[2*i];
		ReadBuffer[2*i]=ReadBuffer[2*i+1];
		ReadBuffer[2*i+1]=data[0];
	}
	

	for(i=0;i<datasize;i++)
	{
		if(ReadBuffer[i]!=DataBuffer[i])
		{
			AddInfo("SLWRWSPI data compare fail!");
			result.Format("ReadBuffer[%d]=%02x,DataBuffer[%d]=%02x",i,ReadBuffer[i],i,DataBuffer[i]);
			AddInfo(result);
			SLWSaveFile(ReadBuffer,datasize,0);
			SLWSaveFile(DataBuffer,datasize,0);
			free(ReadBuffer);
			free(DataBuffer);
			return 0;
		}
	}

	free(DataBuffer);
	free(ReadBuffer);
	return 1;

}

void SLWUpdateIdentify(int device)
{
	PUCHAR buf;
	PUCHAR ReadBuffer;
	int i;
	int datasize=1024*512,readsize=1024*512;
	CString str;
	long ret =0;
	
	buf=(PUCHAR) malloc(datasize);
	ReadBuffer=(PUCHAR)malloc(readsize);

	for(i=0;i<datasize;i++)
	{
		buf[i]=0xff;
	}
	for(i=0;i<readsize;i++)
	{
		ReadBuffer[i]=0x00;
	}

	VNDSetparam(device);	

	ret =  VNDFW2DRAM(device);
	if (ret == 0)
	{
		AddInfo("****Read Global info failed!!!");	
		free(buf);
		free(ReadBuffer);
		return ;
	}
	else
	{
		if(InterfaceType[device]==1)
			ret=VNDFW2ATA48b(device, buf);
		else
			ret=VNDFW2ATA(device, buf);

		if(!ret)
		 {
			AddInfo("****Read Global info failed!!!");	
			free(buf);
			free(ReadBuffer);
			return ;
		 }
	}

	str.Format("ReadSector: %02X %02X %02X %02X %02X %02X %02X %02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	int addptr=1024;
	char output[256];
	CString descr="";
	CString file; 
	file=SLWCONFINI;
	ReadConfigureIni(file);

	if(m_descriptor[device]=="")
	{
		descr="SoliWare";
	}
	else
		descr=m_descriptor[device];

/*
	if( (MaxLBA<0x1E00000) && (MaxLBA>0) )
	{
		descr+=" 016G";
	}
	else if((MaxLBA<0x3C00000 ) && (MaxLBA>0x1E00000))
	{
		descr+=" 032G";
	}
	else if((MaxLBA<0x7800000  ) && (MaxLBA>0x3C00000))
	{
		descr+=" 064G";
	}
	else if((MaxLBA<0xF000000   ) && (MaxLBA>0x7800000 ))
	{
		descr+=" 128G";
	}
*/
	sprintf(output,"%s",descr);
	for(i=0;i<descr.GetLength();i++)
	{
		buf[addptr+54+i]=output[i];
	}
	for(i=descr.GetLength();i<38;i++)
	{
		buf[addptr+54+i]=0x20;
	}
	
	for(i=27;i<46;i++)
	{
		output[0]=buf[addptr+2*i];
		buf[addptr+2*i]=buf[addptr+2*i+1];
		buf[addptr+2*i+1]=output[0];
	}

	if(!VNDWFW2DRAM(device,buf))
	{
		AddInfo("Write Global info block fail!\r\n");
		free(buf);
		free(ReadBuffer);
		return ;
	}	
	else
	{	
		ret=VNDWFW2FLASH(device);
		if(!ret)
		 {
			AddInfo("Write Global info block fail!\r\n");	
			free(buf);
			free(ReadBuffer);
			return ;
		 }
	}

	AddInfo("Write Global info block successfully!\r\n");

	ret =  VNDFW2DRAM(device);
	if (ret == 0)
	{
		AddInfo("****Read Global info failed!!!");	
		free(buf);
		free(ReadBuffer);
		return ;
	}
	else
	{
		if(InterfaceType[device]==1)
			ret=VNDFW2ATA48b(device, ReadBuffer);
		else
			ret=VNDFW2ATA(device, ReadBuffer);
		
		 if(!ret)
		 {
			AddInfo("****Read Global info failed!!!");	
			free(buf);
			free(ReadBuffer);
			return ;
		 }
	}


	for(i=0;i<datasize;i++)
	{
		if(ReadBuffer[i]!=buf[i])
		{
			AddInfo("****Compare Global info failed!!!");	
			free(buf);
			free(ReadBuffer);
			return ;
		}
	}
	
	AddInfo("****Compare Global info successful!!!");
	free(buf);
	free(ReadBuffer);
}

BOOL SLWWritefwDr(int device)
{
	PUCHAR buf;
	PUCHAR ReadBuffer;
	int i;
	int datasize=1024*256,readsize=1024*256;
	CString str;
	long ret =0;

	buf=(PUCHAR) malloc(datasize);
	ReadBuffer=(PUCHAR)malloc(readsize);

	for(i=0;i<datasize;i++)
	{
		buf[i]=0xff;
	}
	for(i=0;i<readsize;i++)
	{
		ReadBuffer[i]=0x00;
	}

	VNDSetparam(device);	

	ret =  VNDFW2DRAM(device);
	if (ret == 0)
	{
		AddInfo("****Read FW info failed!!!");	
		free(buf);
		free(ReadBuffer);
		return 0;
	}
	else
	{
		if(InterfaceType[device]==1)
			ret=VNDFW2ATA48b(device, buf);
		else
			ret=VNDFW2ATA(device, buf);
		
		if(!ret)
		 {
			AddInfo("****Read FW info failed!!!");	
			free(buf);
			free(ReadBuffer);
			return 0;
		 }
	}

	str.Format("ReadSector: %02X %02X %02X %02X %02X %02X %02X %02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	buf[4]=0x20;//FW 起始位置（扇区为单位）	
	buf[8]=0x40;//FW 长度（扇区为单位）


	//清空位于sector 8, 长度为32KB的FW CODE
	for(i=0;i<32*1024;i++)
		buf[i+512*8]=0x0;

	SLWWritesmartInfo(device,buf,512*5);

	int addptr = 512*32;
	ret = SLWGlobalinfo(device,buf,0,addptr,0);
	if(ret ==0 )
	{				
		AddInfo("Get FW code fail!\r\n");
		free(buf);
		free(ReadBuffer);
		return 0;
	}
	
	if(!VNDWFW2DRAM(device,buf))
	{
		AddInfo("Write FW info block fail!\r\n");
		free(buf);
		free(ReadBuffer);
		return 0;
	}	
	else
	{	
		ret=VNDWFW2FLASH(device);
		if(!ret)
		 {
			AddInfo("Write FW info block fail!\r\n");	
			free(buf);
			free(ReadBuffer);
			return 0;
		 }
	}

	AddInfo("Write FW info block successfully!\r\n");

	ret =  VNDFW2DRAM(device);
	if (ret == 0)
	{
		AddInfo("****Read FW info failed!!!");	
		free(buf);
		free(ReadBuffer);
		return 0;
	}
	else
	{
		if(InterfaceType[device]==1)
			ret=VNDFW2ATA48b(device, ReadBuffer);
		else
			ret=VNDFW2ATA(device, ReadBuffer);

		 if(!ret)
		 {
			AddInfo("****Read FW info failed!!!");	
			free(buf);
			free(ReadBuffer);
			return 0;
		 }
	}

/*	for(i=0;i<readsize/2;i++)
	{		
		data=ReadBuffer[2*i];
		ReadBuffer[2*i]=ReadBuffer[2*i+1];
		ReadBuffer[2*i+1]=data;
	}
*/	
	for(i=0;i<datasize;i++)
	{
		if(ReadBuffer[i]!=buf[i])
		{
			AddInfo("****Compare FW info failed!!!");	
			free(buf);
			free(ReadBuffer);
			return 0;
		}
	}
	
	AddInfo("****Compare FW info successful!!!");
	free(buf);
	free(ReadBuffer);
	return 1;
}

void SLWWritefw(int device)
{
	PUCHAR buf,ReadBuffer;
	long ret =0;
	int i,datasize=1024*128*blocksize,readsize=1024*128*blocksize;
	CString str;
	int ichannel,irow;
	DWORD dblock;	
	buf=(PUCHAR) malloc(datasize);
	ReadBuffer=(PUCHAR)malloc(readsize);

	for(i=0;i<datasize;i++)
	{
		buf[i]=0xff;
	}
	for(i=0;i<readsize;i++)
	{
		ReadBuffer[i]=0x00;
	}

	VNDSetparam(device);	
	ret = VNDReadflashid(device);
	if(ret == 0)
	{
		AddInfo("****Read flash id failed!!!");
		free(buf);
		return ;
	}
	
	dblock=2;
	ichannel=0;
	irow=DIEMAP[0];
	VNDSetBlockArg(device,0,sectorperblock,0);
	ret = VNDReadblock(device,buf,ichannel,irow,dblock);
	if (ret == 0)
	{
		AddInfo("****Read global info failed!!!");	
		free(buf);
		free(ReadBuffer);
		return ;
	}

	str.Format("ReadSector: %02X %02X %02X %02X %02X %02X %02X %02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	int addptr = 512*32;
	ret = SLWGlobalinfo(device,buf,0,addptr,0);
	if(ret ==0 )
	{				
		AddInfo("Get FW code fail!\r\n");
		free(buf);
		free(ReadBuffer);
		return ;
	}

	if( VNDEraseblk(device,ichannel,irow,dblock))
	{		
		
		DWORD block_add;
		block_add = 8182<< 18 |dblock << 5| irow;	// LBN + strip size
		VNDSetBlockArg(device,0,sectorperblock,7);

		if(!VNDWriteblock(device,buf,ichannel,block_add,sectorperblock))
		{
			AddInfo("Write FW info block fail!\r\n");
			free(buf);
			free(ReadBuffer);
			return ;
		}	
		else
			AddInfo("Write FW info block successfully!\r\n");
	}
	VNDSetBlockArg(device,0,sectorperblock,0);
	if( VNDReadblock(device,ReadBuffer,ichannel,irow,dblock))
	{
	/*	unsigned char data;
		for(i=0;i<datasize/2;i++)
		{		
			data=ReadBuffer[2*i];
			ReadBuffer[2*i]=ReadBuffer[2*i+1];
			ReadBuffer[2*i+1]=data;
		}
	*/
		for(i=0;i<datasize;i++)
		{
			if(ReadBuffer[i]!=buf[i])
			{
				AddInfo("****Compare Fw info failed!!!");	
				free(buf);
				free(ReadBuffer);
				return ;
			}
		}
	}

	free(buf);
	free(ReadBuffer);
	AddInfo("****Compare Fw info successfully!!!");	
}

BOOL SLWReadfwDr(int device)
{
	int datasize=1024*512,i;
	PUCHAR DataBuffer;
	DataBuffer=(PUCHAR)malloc(datasize);
	
	for(i=0;i<datasize;i++)
	{
		DataBuffer[i]=0;
	}

	VNDSetparam(device);	

	int ret =  VNDFW2DRAM(device);
	if (ret == 0)
	{
		AddInfo("****Read FW info failed!!!");	
		free(DataBuffer);
		return 0;
	}
	else
	{
		if(InterfaceType[device]==1)
			ret=VNDFW2ATA48b(device, DataBuffer);
		else
			ret=VNDFW2ATA(device, DataBuffer);

		 if(!ret)
		 {
			AddInfo("****Read FW info failed!!!");	
			free(DataBuffer);
			return 0;
		 }
	}

	SLWSaveFile(DataBuffer,datasize,1);
	free(DataBuffer);
	return 1;
}

BOOL SLWReadfpgaDr(int device)
{
	CString str;
	int datasize=1024*1024;
	int i;
	PUCHAR DataBuffer;
	DataBuffer=(PUCHAR)malloc(datasize);
	CStdioFile   file;
	int ret;
	for(i=0;i<datasize;i++)
	{
		DataBuffer[i]=0;
	}

	if(InterfaceType[device]==1)
		ret=VNDDriveRSPI48b(device,0,DataBuffer,208);
	else
		ret=VNDDriveRSPI(device,0,DataBuffer,208);
	if(ret==0)
	{
		AddInfo("**Read FPGA Failed**");
		free(DataBuffer);
		return 0;
	}
	SLWSaveFile(DataBuffer,datasize,0);
	free(DataBuffer);
	return 1;
}


BOOL SLWReadfw(int device)
{
	int datasize=1024*512;
	int i;
	PUCHAR DataBuffer;
	DataBuffer=(PUCHAR)malloc(datasize);
	int ichannel,irow;
	DWORD dblock;
	for(i=0;i<datasize;i++)
	{
		DataBuffer[i]=0;
	}

	dblock=2;
	ichannel=0;
	irow=0;
	VNDSetBlockArg(device,0,sectorperblock,0);
	if (VNDReadblock(device,DataBuffer,ichannel,irow,dblock) == 0)
	{
		AddInfo("****Read global info failed!!!");	
		free(DataBuffer);
		return 0;
	}

	SLWSaveFile(DataBuffer,datasize,1);
	free(DataBuffer);
	return 1;
}

BOOL ReadConfigureIni(CString FilePath)
{
	char str[128];
	char filename[128];
	COPini ini;
	int mchannelconf;
	int mplaneconf;
	int mblocksizeconf;
	int mpagesizeconf;
	CString mfwconf;
	CString mdesconf;
	CString sn;
	int i;

    GetModuleFileName(NULL,filename,128); 
    //Scan a string for the last occurrence of a character.
    (strrchr(filename,'\\'))[1] = 0; 
    strcat(filename,FilePath);
		
	ini.ReadString("FLASH Setting","CHANNEL",str,filename);
	mchannelconf=atoi(str);
	channel=mchannelconf;
	ini.ReadString("FLASH Setting","PLANE",str,filename);
	mplaneconf=atoi(str);
	plane=mplaneconf;
	ini.ReadString("FLASH Setting","BLOCK SIZE",str,filename);
	mblocksizeconf=atoi(str);
	ini.ReadString("FLASH Setting","PAGE SIZE",str,filename);
	mpagesizeconf=atoi(str);
	page4k=mpagesizeconf/2;
	
	ini.ReadString("Device Setting","FW VERSION",str,filename);
	mfwconf=str;
	for(i=0;i<DeviceCount;i++)
		m_fw[i] = str;
	ini.ReadString("Device Setting","Model Number",str,filename);
	mdesconf=str;
	for(i=0;i<DeviceCount;i++)
		m_descriptor[i] = str;
	ini.ReadString("Device Setting","Serial Number",str,filename);
	sn=str;
	if(sn.IsEmpty()==0)
	{
		for(i=0;i<DeviceCount;i++)
			BadBlockFilename[i] = sn;
	}
#if _MASSINIT
	ini.ReadString("Device Setting","Initial Mode",str,filename);
	IniMode=atoi(str);	
	ini.ReadString("Update Setting","FW FILE",str,filename);
	FWFilename=str;
	ini.ReadString("Update Setting","REBUILD FILE",str,filename);
	RebuildFilename=str;
	ini.ReadString("Update Setting","FPGA FILE",str,filename);
	FPGAFilename=str;
	ini.ReadString("Update Setting","LOAD FILE",str,filename);
	LoadFilename=str;
#endif
	ini.ReadString("FW Setting","SSD Capability",str,filename);
	SSDCapacity=atol(str);
	/*清空的触发标志位*/
	ini.ReadString("FW Setting","SAN_Status",str,filename);
	PurgeFlag=atol(str);

	ini.ReadString("FW Setting","SSD Interface",str,filename);
	SSDInterface=atol(str);	

	ini.ReadString("FW Setting","Enable Hardware Reset",str,filename);
	Hardwarereset=atol(str);	

	/*测试充电速度时,用的充电时间*/
	ini.ReadString("FW Setting","Charge Time",str,filename);
	chargetime=atol(str);	

	ini.ReadString("Option","Power TEST",str,filename);
	checkpower=atoi(str);
	ini.ReadString("Option","PCB TEST",str,filename);
	checkpcb=atoi(str);
	ini.ReadString("Option","SSD INITIONALIZE",str,filename);
	checkinit=atoi(str);
	ini.ReadString("Option","UPDATE FW",str,filename);
	checkupdatefw=atoi(str);
	ini.ReadString("Option","UPDATE FPGA",str,filename);
	checkupdatefpga=atoi(str);
	return 1;
}

BOOL SLWReadGlobalinfoblockTxt(CString File)
{
	FILE *fp;
	int i,num;
	char output[128];
	CString str;
	unsigned long tmp;
    double n;
	CHAR FilePath[255]; 

    GetModuleFileName(NULL,FilePath,255); 
    (strrchr(FilePath,'\\'))[1] = 0; 
    strcat(FilePath,File);

	fp = fopen(FilePath, "rb");
	if(fp==NULL)
	{
		AfxMessageBox("Open read file fail!");
		return 0;
	}
	
	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&num);
	for(i=0;i<num;i++)
	{
		fscanf(fp,"%X,",&IDFY32G[i]);	
	}

	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&num);
	for(i=0;i<num;i++)
	{
		fscanf(fp,"%x,",&MLCFLASH[i]);	
		BCKMLCFLASH[i]=MLCFLASH[i];
	}

	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&num);
	ConfNum = num;
	for(i=0;i<num;i++)
	{
		fscanf(fp,"%x,",&SLCFLASH[i]);	
		BCKSLCFLASH[i]=SLCFLASH[i];
	}

	/*从全局信息文件中读FLASH参数 */
	FLASHtypeNum=0;
	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&FLASHtypeNum);
	if(FLASHtypeNum==0)
		return 0;
	for(i=0;i<num;i++)
	{
		fscanf(fp,"%X,",&FlashFeature[i].ID[0]);	
		fscanf(fp,"%X,",&FlashFeature[i].ID[1]);	
		fscanf(fp,"%X,",&FlashFeature[i].ID[2]);	
		fscanf(fp,"%X,",&FlashFeature[i].ID[3]);	
		fscanf(fp,"%X,",&FlashFeature[i].Endurance);
		fscanf(fp,"%X,",&FlashFeature[i].pagesize);
		fscanf(fp,"%X,",&FlashFeature[i].blocksize);
		fscanf(fp,"%X,",&FlashFeature[i].blockperdie);
		fscanf(fp,"%X,",&FlashFeature[i].PageMark0);
		fscanf(fp,"%X,",&FlashFeature[i].PageMark1);
		fscanf(fp,"%X,",&FlashFeature[i].ByteMark0);
		fscanf(fp,"%X,",&FlashFeature[i].ByteMark1);
		fscanf(fp,"%X,",&FlashFeature[i].Flashtype);
		fscanf(fp,"%X,",&FlashFeature[i].Planeperdie);
	}


	fclose(fp);

	return 1;
}

int FindFlashtypePos()
{
	int i,j;

	for(i=0;i<64;i++)
	{
		for(j=0;j<FLASHtypeNum;j++)
		{
			if(flash_id[i][0]==FlashFeature[j].ID[0] && flash_id[i][1]==FlashFeature[j].ID[1] && flash_id[i][2]==FlashFeature[j].ID[2] && flash_id[i][3]==FlashFeature[j].ID[3]) 
				return j;		
		}
	}

	return 255;
}


BOOL SLWSetFlash() 
{	
	ULONGLONG tmp=0;
    double n;
	int j;
	int i,dieplaneoffset;


	//SLC为0，MLC为1(改为page_per_block为64置0 page_per_block为128 置1)
	if(flash_type.pages_per_block==64)
		SLCFLASH[0]=0;
	else if(flash_type.pages_per_block==128)
		SLCFLASH[0]=1;

	if(flash_type.pages_per_block==64)
		MLCFLASH[0]=0;
	else if(flash_type.pages_per_block==128)
		MLCFLASH[0]=1;

	j=FindFlashtypePos();
	if(j<FLASHtypeNum)
	{
		if(flash_type.isMLC)
		{
			//LBA总数量，为最大LBA地址+1 (保留1/16)
			tmp=channel*grdie*FlashFeature[j].blockperdie*FlashFeature[j].blocksize*2;
			tmp=tmp*15/16;
			MLCFLASH[16]=tmp;
			MLCFLASH[17]=tmp>>16;
			//物理block的大小（扇区）
			tmp=FlashFeature[j].blocksize*2;
			MLCFLASH[18]=tmp;
			MLCFLASH[19]=tmp>>16;
			//LBA中 logic die num的掩码
			tmp=grdie-1;
			n=log((double)(2*FlashFeature[j].blocksize*channel))/log(2.0);
			dieplaneoffset=n;
			if(n-dieplaneoffset>0)
				dieplaneoffset=dieplaneoffset+1;
			MLCFLASH[20]= tmp<<dieplaneoffset;
			MLCFLASH[21]= (tmp<<dieplaneoffset)>>16;
			//LBA中 die , plane 的位移
			MLCFLASH[22]= dieplaneoffset;
			MLCFLASH[23]= (dieplaneoffset)>>16;
			//LBA中 LBN 的掩码
			tmp=grdie*FlashFeature[j].blockperdie; 
			tmp=tmp-1;
			MLCFLASH[28]= tmp<<dieplaneoffset;
			MLCFLASH[29]= (tmp<<dieplaneoffset)>>16;
			//LBA中 LBN 的掩码取反
			MLCFLASH[30]= ~MLCFLASH[28];
			MLCFLASH[31]= 0;
			//LBA中 逻辑条带 的掩码
			tmp=FlashFeature[j].blocksize/FlashFeature[j].pagesize;
			tmp=tmp-1;
			MLCFLASH[32]=tmp<<5 ;
			MLCFLASH[33]=(tmp<<5) >>16;
			//LBA中 plane num的掩码
			tmp=FlashFeature[j].Planeperdie*grdie-1;
			MLCFLASH[40]= tmp<<dieplaneoffset;
			MLCFLASH[41]= (tmp<<dieplaneoffset)>>16;
			//LBA中 in-plane-block-num 的位移
			n=log((double)(FlashFeature[j].Planeperdie*grdie))/log(2.0);
			tmp=n+dieplaneoffset;
			MLCFLASH[48]=tmp;
			MLCFLASH[49]=tmp>>16;
			//映射表大小，依盘容量而定，以sector为单位,计算公式每个BLOCK占用2字节，所以是盘的可用block数乘2，除以512
			tmp=channel*grdie*FlashFeature[j].blockperdie*2*15/(512*16)+32;
			if(tmp%32>0)
			{
				tmp=(tmp/32+1)*32;
			}
			MLCFLASH[50]=tmp;
			MLCFLASH[51]=tmp>>16;
		}
		else
		{
			//LBA总数量，为最大LBA地址+1 (保留1/16)
			tmp=channel*grdie*FlashFeature[j].blockperdie*FlashFeature[j].blocksize*2;
			tmp=tmp*15/16;
			SLCFLASH[16]=tmp;
			SLCFLASH[17]=tmp>>16;
			//物理block的大小（扇区）
			tmp=FlashFeature[j].blocksize*2;
			SLCFLASH[18]=tmp;
			SLCFLASH[19]=tmp>>16;
			//LBA中 logic die num的掩码
			tmp=grdie-1;
			n=log((double)(2*FlashFeature[j].blocksize*channel))/log(2.0);
			dieplaneoffset=n;
			if(n-dieplaneoffset>0)
				dieplaneoffset=dieplaneoffset+1;
			SLCFLASH[20]= tmp<<dieplaneoffset;
			SLCFLASH[21]= (tmp<<dieplaneoffset)>>16;
			//LBA中 die , plane 的位移
			SLCFLASH[22]= dieplaneoffset;
			SLCFLASH[23]= (dieplaneoffset)>>16;
			//LBA中 LBN 的掩码
			tmp=grdie*FlashFeature[j].blockperdie; 
			tmp=tmp-1;
			SLCFLASH[28]= tmp<<dieplaneoffset;
			SLCFLASH[29]= (tmp<<dieplaneoffset)>>16;
			//LBA中 LBN 的掩码取反
			SLCFLASH[30]= ~SLCFLASH[28];
			SLCFLASH[31]= 0;
			//LBA中 逻辑条带 的掩码
			tmp=FlashFeature[j].blocksize/FlashFeature[j].pagesize;
			tmp=tmp-1;
			SLCFLASH[32]=tmp<<5 ;
			SLCFLASH[33]=(tmp<<5) >>16;
			//LBA中 plane num的掩码
			tmp=FlashFeature[j].Planeperdie*grdie-1;
			SLCFLASH[40]= tmp<<dieplaneoffset;
			SLCFLASH[41]= (tmp<<dieplaneoffset)>>16;
			//LBA中 in-plane-block-num 的位移
			n=log((double)(FlashFeature[j].Planeperdie*grdie))/log(2.0);
			tmp=n+dieplaneoffset;
			SLCFLASH[48]=tmp;
			SLCFLASH[49]=tmp>>16;
			//映射表大小，依盘容量而定，以sector为单位,计算公式每个BLOCK占用2字节，所以是盘的可用block数乘2，除以512
			tmp=channel*grdie*FlashFeature[j].blockperdie*2*15/(512*16)+32;
			if(tmp%32>0)
			{
				tmp=(tmp/32+1)*32;
			}
			SLCFLASH[50]=tmp;
			SLCFLASH[51]=tmp>>16;
			SLCFLASH[14]=dieplaneoffset;//根据逻辑block大小决定，log2(Logic_block_size）
		}

	}
	else
	{
		if(flash_type.isMLC)
		{
			//LBA总数量，为最大LBA地址+1
			tmp=grdie*1024*1024*15;
			MLCFLASH[16]=tmp;
			MLCFLASH[17]=tmp>>16;
			//LBA中 logic die num的掩码
			tmp=grdie-1;
			MLCFLASH[20]= tmp<<12;
			MLCFLASH[21]= (tmp<<12)>>16;
			//LBA中 LBN 的掩码
			tmp=2*grdie-1;
			tmp=(tmp<<11)|0x7ff;
			MLCFLASH[28]= tmp<<12;
			MLCFLASH[29]= (tmp<<12)>>16;
			//LBA中 plane num的掩码
			tmp=2*grdie-1;
			MLCFLASH[40]= tmp<<12;
			MLCFLASH[41]= (tmp<<12)>>16;
			//LBA中 in-plane-block-num 的位移
			n=log((double)(2*grdie))/log(2.0);

			tmp=n+12;
			MLCFLASH[48]=tmp;
			MLCFLASH[49]=tmp>>16;
			//映射表大小，依盘容量而定，以sector为单位
			tmp=4*2*grdie*15*8/16+32;
			if(tmp%32>0)
			{
				tmp=(tmp/32+1)*32;
			}
			MLCFLASH[50]=tmp;
			MLCFLASH[51]=tmp>>16;
		}
		else
		{
			//LBA总数量，为最大LBA地址+1
			tmp=grdie*1024*512*15;
			SLCFLASH[16]=tmp;
			SLCFLASH[17]=tmp>>16;
			//LBA中 logic die num的掩码
			tmp=grdie-1;
			SLCFLASH[20]= tmp<<11;
			SLCFLASH[21]= (tmp<<11)>>16;
			//LBA中 LBN 的掩码
			tmp=2*grdie-1;
			tmp=(tmp<<11)|0x7ff;
			SLCFLASH[28]= tmp<<11;
			SLCFLASH[29]= (tmp<<11)>>16;
			//LBA中 plane num的掩码
			tmp=2*grdie-1;
			SLCFLASH[40]= tmp<<11;
			SLCFLASH[41]= (tmp<<11)>>16;
			//LBA中 in-plane-block-num 的位移
			n=log((double)(2*grdie))/log(2.0);
			tmp=n+11;
			SLCFLASH[48]=tmp;
			SLCFLASH[49]=tmp>>16;
			//映射表大小，依盘容量而定，以sector为单位
			tmp=4*2*grdie*15*8/16+32;
			if(tmp%32>0)
			{
				tmp=(tmp/32+1)*32;
			}
			SLCFLASH[50]=tmp;
			SLCFLASH[51]=tmp>>16;

		}
	}
	return 1;
}

BOOL SLWWriteGlobalinfoblockTxt(CString File)
{
	FILE *fp;
	int i;
	CString str;

    CHAR FilePath[255]; 
    GetModuleFileName(NULL,FilePath,255); 
    (strrchr(FilePath,'\\'))[1] = 0; 
    strcat(FilePath,File);

	fp = fopen(FilePath, "w");
	if(fp==NULL)
	{
		AfxMessageBox("Open read file fail!");
		return 0;
	}
	
	fprintf(fp,"%s ","IDFY32G:");
	fprintf(fp,"%d\r\n",128);
	for(i=0;i<128;i++)
	{
		fprintf(fp,"%X,",IDFY32G[i]);	
		if((i+1)%8==0)
			fprintf(fp,"\r\n");
	}

	fprintf(fp,"\r\n%s ","MLCFLASH:");
	fprintf(fp,"%d\r\n",ConfNum);
	for(i=0;i<ConfNum;i++)
	{
		fprintf(fp,"%X,",BCKMLCFLASH[i]);
		if((i+1)%8==0)
			fprintf(fp,"\r\n");
	}

//保持FLASH参数到文件时，不保留修改后的值，保存原值，即参数是一个原表，每次读出后修改后，配置到全局参数区，但是保存到文件原值不变
	fprintf(fp,"\r\n%s ","SLCFLASH:");
	fprintf(fp,"%d\r\n",ConfNum);
	for(i=0;i<ConfNum;i++)
	{
		fprintf(fp,"%X,",BCKSLCFLASH[i]);	
		if((i+1)%8==0)
			fprintf(fp,"\r\n");
	}

	fprintf(fp,"\r\n\r\n%s ","FLASHTYPENUM:");
	fprintf(fp,"%d\r\n",FLASHtypeNum);
	for(i=0;i<FLASHtypeNum;i++)
	{

		fprintf(fp,"%02X,",FlashFeature[i].ID[0]);	
		fprintf(fp,"%02X,",FlashFeature[i].ID[1]);	
		fprintf(fp,"%02X,",FlashFeature[i].ID[2]);	
		fprintf(fp,"%02X,",FlashFeature[i].ID[3]);	
		fprintf(fp,"%08X,",FlashFeature[i].Endurance);
		fprintf(fp,"%X,",FlashFeature[i].pagesize);
		fprintf(fp,"%04X,",FlashFeature[i].blocksize);
		fprintf(fp,"%04X,",FlashFeature[i].blockperdie);
		fprintf(fp,"%04X,",FlashFeature[i].PageMark0);
		fprintf(fp,"%04X,",FlashFeature[i].PageMark1);
		fprintf(fp,"%04X,",FlashFeature[i].ByteMark0);
		fprintf(fp,"%04X,",FlashFeature[i].ByteMark1);
		fprintf(fp,"%X,",FlashFeature[i].Flashtype);
		fprintf(fp,"%X,",FlashFeature[i].Planeperdie);
		fprintf(fp,"\r\n");
	}

	fclose(fp);
	return 1;
}

/*清空全局变量，为初始化做准备*/
void ResetGlobal(int device,int Initnum)
{
	int i,j,k;
	/*当第一次初始化时清空原始坏块表，其他时候更新原始坏块表*/
	if(Initnum==0)
	{
		for(i=0;i<gldie*4;i++)
			for(j=0;j<512;j++)
			{
				badblock[device][i][j]=0;	
			}

		for(i=0;i<gldie*4;i++)
			xblock[device][i]=0;
	}

	for(i=0;i<2;i++)
		for(j=0;j<gldie*4;j++)
		{
			datablock[device][i][j]=0;	
		}
 	
	for(i=0;i<2;i++)
		for(j=0;j<gldie*4;j++)
		{
			startblock[device][i][j]=0;	
		}

	for(i=0;i<2;i++)
		for(j=0;j<gldie*4;j++)
		{
			endblock[device][i][j]=0;	
		}

	for(i=0;i<2;i++)
		for(j=0;j<gldie*4;j++)
		{
			swapblock[device][i][j]=0;	
		}

	for(i=0;i<2;i++)
		for(j=0;j<gldie*4;j++)
			for(k=0;k<128;k++)
			{
				replaceblock[device][i][j][k]=0;	
			}		

	for(i=0;i<2;i++)
		for(j=0;j<gldie*4;j++)
		{
			replaceblocknum[device][i][j]=0;	
		}
	
	for(i=0;i<gldie*4;i++)
		reserveblock[device][i]=0;			
}

/*初始化函数，在检测到新增坏块时，更新坏块表，并重新初始化，初始化次数不能大于InitialCount*/
int SLWInitial(int Device,int Initnum )
{
	if(Initnum >= InitialCount)
	{
		AddInfo("****Find too many bad block,advice Initial from chip !\r\n");
		return -1;
	}

	ResetGlobal(Device,Initnum);
	VNDSetparam(Device);
	if(Initnum==0)
	{
		if(VNDReadflashid(Device)==0)
		{
			AddInfo("****ReadID fail!\r\n");
			LogInfo(Device,"ReadID fail!\r\n",3);
			return -1;
		}
	}

	SLWSetFlash();
	VNDSetFLASHArg(Device);
#if _INITD 			
	pGuiServerExplorer->m_progress.SetText("");	
	pGuiServerExplorer->m_progress.SetPos(0);
#endif
	CString file; 
	file=SLWCONFINI;
	if(Initnum==0)
		ReadConfigureIni(file);

	AddInfo("Building BadblockTable...");
	if( !SLWGetDeviceInfo(Device,Initnum))		// 建立坏块表
	{
		AddInfo("****failed to build bad table!!!\r\n");
		LogInfo(Device,"****failed to build bad table!!!\r\n",3);
		return -1;
	}
#if _INITD 
	pGuiServerExplorer->m_progress.SetPos(15);
#endif
	AddInfo("Building the GlobalInfoBlock...");
	LogInfo(Device,"Building the GlobalInfoBlock!!!\r\n",3);
	if(SLWWriteGlobleInfoBlock(Device,Initnum)==0)
	{
		AddInfo("Building GlobleInfoBlock fail!");
		LogInfo(Device,"****failed to build GlobleInfoBlock!!!\r\n",3);
		return -1;
	}
#if _INITD 
	pGuiServerExplorer->m_progress.SetPos(20);
#endif
	AddInfo("Building MapTable...");
	LogInfo(Device,"Building MapTable!!!\r\n",3);
	if(SLWWriteMapTableToFLASH(Device,Initnum)==0)
	{
		AddInfo("Building MapTableToRAM fail!");
		LogInfo(Device,"****failed to build MapTable!!!\r\n",3);
		return -1;
	}
#if _INITD 
	pGuiServerExplorer->m_progress.SetPos(30);
#endif
	AddInfo("Writing BadReserveBlock...");//写坏块保留区之前要写映射表，因为要先确定替换块
	LogInfo(Device,"Writing BadReserveBlock!!!\r\n",3);
	if(SLWWriteBadReserveBlock(Device,Initnum)==0)
	{
		AddInfo("Write BadReserveBlock fail!");
		LogInfo(Device,"****failed to Write BadReserveBlock!!!\r\n",3);
		return -1;
	}
/*	if(SLWWriteGMBlock(Device,0)==0)
	{
		AddInfo("WriteMapTableToRAM failed!");
		return -1;
	}*/
#if _INITD 
	pGuiServerExplorer->m_progress.SetPos(85);
#endif
	return 1;

/*
	AddInfo("Writing SmartBlock...");

	if(SLWWritesmartBlock()==0)
	{
		AddInfo("SLWWritesmartBlock fail!");
		return;
	}

	AddInfo("Writing SecurityBlock...");

	if(SLWWritesecurityBlock()==0)
	{
		AddInfo("SLWWritesecurityBlock fail!");
		return;
	}

	AddInfo("Writing CodeBlock...");
	
	if(SLWWriteCodeBlock()==0)
	{
		AddInfo("WriteCodeBlock fail!");
		return;
	}
*/
}

BOOL SLWUpdateBadblock(int device,int channelnum,int dienume,int blocknum)
{
	int loc,j;
	BOOL flag=0;
	loc=channelnum*gldie+dienume;
	for(j=0;j<xblock[device][loc];j++)
	{
		if(blocknum==badblock[device][loc][j])
		{
			flag=1;
			break;
		}
	}
	if(flag==0)
	{
		badblock[device][loc][xblock[device][loc]] = blocknum;   //badblock的第二维对应逻辑die,所以要转换为i+m_row-i%gldie
		xblock[device][loc]++;
	}
	return 1;
}

BOOL VNDReadNV(int device,BYTE* buf)
{
	IDEREGS regs;
	CString str;
	regs.bFeaturesReg		= SLW_VU_R_NV2DRAM;	// send  command
	regs.bSectorCountReg	= 0;			
	regs.bSectorNumberReg	= 0 ;					
	regs.bCylLowReg	= 0;
	regs.bCylHighReg = 0;
	regs.bDriveHeadReg = 0;		

	if( !PIO_VENDOR_OPS( device,true, buf, 0, &regs))
	{
		AddInfo("读出NV到DRAM command rejected*");
		return 0;
	}	

	regs.bFeaturesReg		= SLW_VU_R_FW2ATA;	// send  command
	regs.bSectorCountReg	= 8;	
	regs.bSectorNumberReg	= 0 ;					
	regs.bCylLowReg	= 8;
	regs.bCylHighReg = 0x80;
	regs.bDriveHeadReg = 0x00;		
	regs.bCommandReg = 0xFE;

	if( !ata_pass_through_ioctl_pio(device, &regs, buf, 4096, false) )
	{
		AddInfo("读出NV到ATA command rejected !");
		CloseHandle(Devicehandle[device]);
		Devicehandle[device]=INVALID_HANDLE_VALUE;
		return 0;
	}
	return 1;
}

BOOL VNDReadGC(int device,BYTE* buf)
{
	IDEREGS regs;
	CString str;
	regs.bFeaturesReg		= SLW_VU_R_GC2DRAM;	// send  command
	regs.bSectorCountReg	= 0;			
	regs.bSectorNumberReg	= 0 ;					
	regs.bCylLowReg	= 0;
	regs.bCylHighReg = 0;
	regs.bDriveHeadReg = 0;		

	if( !PIO_VENDOR_OPS( device,true, buf, 0, &regs))
	{
		AddInfo("读出GC到DRAM command rejected*");
		return 0;
	}	

	regs.bFeaturesReg		= SLW_VU_R_FW2ATA;	// send  command
	regs.bSectorCountReg	= 8;	
	regs.bSectorNumberReg	= 0 ;					
	regs.bCylLowReg	= 8;
	regs.bCylHighReg = 0x80;
	regs.bDriveHeadReg = 0x00;		
	regs.bCommandReg = 0xFE;

	if( !ata_pass_through_ioctl_pio(device, &regs, buf, 4096, false) )
	{
		AddInfo("读出GC到ATA command rejected !");
		CloseHandle(Devicehandle[device]);
		Devicehandle[device]=INVALID_HANDLE_VALUE;
		return 0;
	}
	return 1;
}

BOOL VNDReadSWAP(int device,BYTE* buf)
{
	IDEREGS regs;
	CString str;
	regs.bFeaturesReg		= SLW_VU_R_SWAP2DRAM;	// send  command
	regs.bSectorCountReg	= 0;			
	regs.bSectorNumberReg	= 0 ;					
	regs.bCylLowReg	= 0;
	regs.bCylHighReg = 0;
	regs.bDriveHeadReg = 0;		

	if( !PIO_VENDOR_OPS( device,true, buf, 0, &regs))
	{
		AddInfo("读出SWAP到DRAM command rejected*");
		return 0;
	}	

	regs.bFeaturesReg		= SLW_VU_R_FW2ATA;	// send  command
	regs.bSectorCountReg	= 8;	
	regs.bSectorNumberReg	= 0 ;					
	regs.bCylLowReg	= 8;
	regs.bCylHighReg = 0x80;
	regs.bDriveHeadReg = 0x00;		
	regs.bCommandReg = 0xFE;

	if( !ata_pass_through_ioctl_pio(device, &regs, buf, 4096, false) )
	{
		AddInfo("读出SWAP到ATA command rejected !");
		CloseHandle(Devicehandle[device]);
		Devicehandle[device]=INVALID_HANDLE_VALUE;
		return 0;
	}
	return 1;
}


BOOL VNDFW2DRAM(int device)
{
	BYTE buf[512];
	IDEREGS regs;
	CString str;

	regs.bFeaturesReg		= SLW_VU_R_FW2DRAM;	// send  command
	regs.bSectorCountReg	= 0;			
	regs.bSectorNumberReg	= 0 ;					
	regs.bCylLowReg	= 0;
	regs.bCylHighReg = 0;
	regs.bDriveHeadReg = 0;		

	if( !PIO_VENDOR_OPS( device,true, buf, 0, &regs))
	{
		AddInfo("读出FW到DRAM command rejected*");
		return 0;
	}	

	AddInfo("*读出FW到DRAM成功");
	return 1;

}

BOOL VNDFW2ATA(int device,BYTE* buf)
{
	IDEREGS regs;
	CString str;

	int i;
	BYTE Data[4096];
	memset(Data,0,4096);

	for(i=0;i<64;i++)
	{
		regs.bFeaturesReg		= SLW_VU_R_FW2ATA;	// send  command
		regs.bSectorCountReg	= 8;	
		regs.bSectorNumberReg	= i ;					
		regs.bCylLowReg	= 8;
		regs.bCylHighReg = 0x80;
		regs.bDriveHeadReg = 0xE0;		
		regs.bCommandReg = 0xFE;

		if( !ata_pass_through_ioctl_pio(device, &regs, (BYTE *)&buf[i * 4096], 4096, false) )
		{
			AddInfo("读出FW到ATA command rejected !");
			CloseHandle(Devicehandle[device]);
			Devicehandle[device]=INVALID_HANDLE_VALUE;
			return FALSE;
		}
#if _INITD 
		pGuiServerExplorer->m_progress.SetPos(100*i/128);
#endif
	}
	AddInfo("*读出FW到ATA成功*");
	return 1;
}



BOOL VNDWFW2DRAM(int device,BYTE* buf)
{
	IDEREGS regs;
	CString str;

	int i;

	for(i=0;i<64;i++)
	{
		regs.bFeaturesReg		= SLW_VU_W_FW2DRAM;	// send  command
		regs.bSectorCountReg	= 8;			
		regs.bSectorNumberReg	= i ;					
		regs.bCylLowReg	= 8;
		regs.bCylHighReg = 0;
		regs.bDriveHeadReg = 0xE0;		
		regs.bCommandReg = SLW_ATA_CMD_VND;
	
		if( !ata_pass_through_ioctl_pio(device, &regs, (BYTE *)&buf[i * 4096], 4096, true) )
		{
			AddInfo("* 写入FW到DRAM command rejected*");
			CloseHandle(Devicehandle[device]);
			Devicehandle[device]=INVALID_HANDLE_VALUE;
			return 0;
		}
#if _INITD 
		pGuiServerExplorer->m_progress.SetPos(100*i/128);
#endif
	}
	AddInfo("*写入FW到DRAM成功*");
	return 1;
}


BOOL VNDWFW2FLASH(int device)
{
	BYTE buf[512];
	IDEREGS regs;
	CString str;

	regs.bFeaturesReg		= SLW_VU_W_FW2FLASH;	// send  command
	regs.bSectorCountReg	= 0;			
	regs.bSectorNumberReg	= 0 ;					
	regs.bCylLowReg	= 0;
	regs.bCylHighReg = 0;
	regs.bDriveHeadReg = 0;		

	if( !PIO_VENDOR_OPS( device,true, buf, 0, &regs))
	{
		AddInfo("写入FW到FLASH command rejected*");
		return 0;
	}	

	AddInfo("*写入FW到FLASH成功");
	return 1;
}

BOOL VNDDriveRSPI(int device,int pageoff,BYTE* buf,int pagenum)
{
	IDEREGS regs;
	CString str;

	int i;
	BYTE Data[4096];
	memset(Data,0,4096);

	for(i=0;i<pagenum;i++)
	{
		regs.bFeaturesReg		= SLW_VU_R_SPI;	// send  command
		regs.bSectorCountReg	= 8;	
		regs.bSectorNumberReg	= i+pageoff ;					
		regs.bCylLowReg	= 8;
		regs.bCylHighReg = 0x80;
		regs.bDriveHeadReg = 0xE0;		
		regs.bCommandReg = 0xFE;

		if( !ata_pass_through_ioctl_pio(device, &regs, (BYTE *)&buf[i * 4096], 4096, false) )
		{
			AddInfo("从SPI 读出数据到主机 command rejected !");
			CloseHandle(Devicehandle[device]);
			Devicehandle[device]=INVALID_HANDLE_VALUE;
			return FALSE;
		}
#if _INITD 
		pGuiServerExplorer->m_progress.SetPos(100*i/208);
#endif
	}
	AddInfo("*从SPI 读出数据到主机 成功*");
	return 1;
}

BOOL VNDDriveWSPI(int device,int pageoff,BYTE* buf,int pagenum)
{
	IDEREGS regs;

	int i;

	for(i=0;i<pagenum;i++)
	{
//		Sleep(2);
		regs.bFeaturesReg		= SLW_VU_W_SPI;	// send  command
		regs.bSectorCountReg	= 8;			
		regs.bSectorNumberReg	= i+pageoff ;					
		regs.bCylLowReg	= 0;
		regs.bCylHighReg = 0;
		regs.bDriveHeadReg = 0xE0;		
		regs.bCommandReg = SLW_ATA_CMD_VND;
		
		if( !ata_pass_through_ioctl_pio(device, &regs, (BYTE *)&buf[i * 4096], 4096, true) )
		{
			AddInfo("* 从主机写入数据到SPI FLASH command rejected*");
			CloseHandle(Devicehandle[device]);
			Devicehandle[device]=INVALID_HANDLE_VALUE;
			return 0;
		}
		if((i%16)==0)
			Sleep(1200);		/*每次命令传输4k,每64k等待2秒*/
#if _INITD 
		pGuiServerExplorer->m_progress.SetPos(100*i/256);
#endif
	}
	AddInfo("*从主机写入数据到SPI FLASH成功*");
	return 1;
}


void SLWEraseAll(int device)
{
	int idie,ichannel,iplane = 2;
	DWORD dblock = 4096,num;
	DWORDLONG LBAADD = 0;
	CString str;
	int loc=0;
	for(ichannel = 0; ichannel <4 ;ichannel ++)
	{
		for (idie = 0; idie <gldie ;idie ++)
		{

#if _MASSINIT
			str.Format("%d",80*(ichannel*gldie+idie)/(4*gldie));
			str+="%";
			pSoliWareMPToolDlg->m_GridCtrl.SetItemText(device+1,2,str);
			pSoliWareMPToolDlg->m_GridCtrl.Refresh();
#endif

			for(dblock = 0;dblock <flash_type.blocknum ; dblock++)
			{
				if(DIEMAP[idie]==65535)
					continue;
				if( (DIEMAP[idie]>7&&DIEMAP[idie]<16) || (DIEMAP[idie]>23&&DIEMAP[idie]<32) )
					num=dblock+flash_type.blocknum;
				else
					num=dblock;
				if( VNDEraseblk(device,ichannel,DIEMAP[idie],num)==-1)
				{
					loc=ichannel*gldie+DIEMAP[idie];
					badblock[device][loc][xblock[device][loc]] = num;   //badblock的第二维对应逻辑die,所以要转换为i+m_row-i%gldie
					xblock[device][loc]++;
				}
			}
		}
	}
	AddInfo("Erase all block and find new bad block successfully !\r\n");
}

/*SLWSaveFile保存读出的fw和fpga， flag 标准为0 则高低字节不需要交换，为1则高低字节需要交换*/
BOOL SLWSaveFile(PUCHAR ReadBuffer,int datasize,BOOL flag)
{
	char filename[128];
	CString strDldFile;
	unsigned char data;
	CStdioFile   file;
	CString Str;
	char output[64];
	int i,j;

	CFileDialog dlgLoad(
		TRUE, 0, 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"(*.txt)|*.txt|AllFiles (*.*)|*.*||");
	dlgLoad.m_ofn.lpstrTitle = "Save File";
	dlgLoad.m_ofn.lpstrInitialDir = strDldFile; // OK if you specify a file
	if(dlgLoad.DoModal() != IDOK)
	{
		return 0;		
	}	

	strDldFile = dlgLoad.m_ofn.lpstrFile;
	if( strDldFile.Right(4) != ".txt")
		strDldFile += ".txt";	
	sprintf(filename,"%s",strDldFile);

	if(!file.Open(filename,CFile::modeCreate|CFile::modeReadWrite|CFile::shareDenyNone))
	{
		return 0;
	}
	else
	{
		if(flag)
		{
			for(i=0;i<datasize/2;i++)
			{
				data=ReadBuffer[2*i];
				ReadBuffer[2*i]=ReadBuffer[2*i+1];
				ReadBuffer[2*i+1]=data;		
			}
		}
		file.SeekToEnd();
		for(i=0;i<datasize/16;i++)
		{
			Str.Format("%08x: ",i*16);	
			for(j=0;j<16;j++)
			{
				sprintf(output,"%02x ",(ReadBuffer[16*i+j]));
				Str+=output;
			}
			Str+="\r\n";
			file.WriteString(Str);
		}
		file.Close();
	}
	return 1;

}

BOOL FindSSD()
{
	HDEVINFO        hIntDevInfo;
    DWORD           index;
    BOOL            status;
    //
    // Open the device using device interface registered by the driver
    //

    //
    // Get the interface device information set that contains all devices of event class.
    //
	int i;
	for(i=0;i<6;i++)
		Devicehandle[i]=INVALID_HANDLE_VALUE;

    hIntDevInfo = SetupDiGetClassDevs (
                 (LPGUID)&DiskClassGuid,
                 NULL,                                   // Enumerator
                 NULL,                                   // Parent Window
                 (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE  // Only Devices present & Interface class
                 ));

    if( hIntDevInfo == INVALID_HANDLE_VALUE ) {
        return 0;
    }

    //
    //  Enumerate all the disk devices
    //
    index = 0;
	SSDID = 0;
    while (TRUE) 
    {
        status = GetDeviceProperty( hIntDevInfo, index );
	//	status = SW80GetDeviceProperty( hIntDevInfo, index );
        if ( status == FALSE ) {
            break;
        }
        index++;
    }
    SetupDiDestroyDeviceInfoList(hIntDevInfo);
	if(Devicehandle[0]!=INVALID_HANDLE_VALUE)
		return 1;
	else 
		return 0;
}

int  GetNewSSDHandle(int sataport)
{
	HDEVINFO        hIntDevInfo;
    DWORD           index;
    BOOL            status;
    //
    // Open the device using device interface registered by the driver
    //

    //
    // Get the interface device information set that contains all devices of event class.
    //
//	WaitForSingleObject(hEventEnumSSD, INFINITE);
//	ResetEvent(hEventEnumSSD);
	
    hIntDevInfo = SetupDiGetClassDevs (
                 (LPGUID)&DiskClassGuid,
                 NULL,                                   // Enumerator
                 NULL,                                   // Parent Window
                 (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE  // Only Devices present & Interface class
                 ));

    if( hIntDevInfo == INVALID_HANDLE_VALUE ) {
        return 0;
    }

    //
    //  Enumerate all the disk devices
    //
    index = 0;

    while (TRUE) 
    {
        status = GetNewDeviceProperty( hIntDevInfo, index,sataport );
        if ( status == FALSE || Devicehandle[sataport]!=INVALID_HANDLE_VALUE) {
            break;
        }
        index++;
    }

    SetupDiDestroyDeviceInfoList(hIntDevInfo);
//	SetEvent(hEventEnumSSD);
	if(Devicehandle[sataport]==INVALID_HANDLE_VALUE)
		return 0;
	return 1;
}

BOOL FindRamDisk()
{
	HDEVINFO        hIntDevInfo;
    DWORD           index;
    BOOL            status;
    //
    // Open the device using device interface registered by the driver
    //

    //
    // Get the interface device information set that contains all devices of event class.
    //
	int i;
	for(i=0;i<6;i++)
		Devicehandle[i]=INVALID_HANDLE_VALUE;

    hIntDevInfo = SetupDiGetClassDevs (
                 (LPGUID)&DiskClassGuid,
                 NULL,                                   // Enumerator
                 NULL,                                   // Parent Window
                 (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE  // Only Devices present & Interface class
                 ));

    if( hIntDevInfo == INVALID_HANDLE_VALUE ) {
        return 0;
    }

    //
    //  Enumerate all the disk devices
    //
    index = 0;
	SSDID = 0;
    while (TRUE) 
    {
        status = GetRamDiskProperty( hIntDevInfo, index );
        if ( status == FALSE ) {
            break;
        }
        index++;
    }
    SetupDiDestroyDeviceInfoList(hIntDevInfo);
	if(Devicehandle[0]!=INVALID_HANDLE_VALUE)
		return 1;
	else 
		return 0;
}

BOOL CloseSSDHandle()
{
	int i;
	for(i=0;i<DeviceCount;i++)
	{
		if(Devicehandle[i]!=INVALID_HANDLE_VALUE)
		{
			CloseHandle(Devicehandle[i]);
			Devicehandle[i]=INVALID_HANDLE_VALUE;
		}
	}
	return 1;
}

/*在DevSnMap中查找SN,
如果存在，说明相应的device handle已经存在，不需要重新创建,返回0
如果不存在，说明相应的device handle不存在，需要重新创建,返回1
*/
BOOL SearchDevSnMap(char* devicepath,char* devsn)
{
	char *pdest,*pdest1,*pdest2;
	int  result,i;

	pdest = strstr(devicepath,"ide#");
	if(pdest!=NULL)
	{
		pdest = pdest+4;
		pdest1 = strstr(pdest,"#");
	}
	else
		return 0;
	if(pdest1!=NULL)
		pdest2 = strstr(pdest1,"#{");
	else
		return 0;
	if(pdest2!=NULL)
		result=pdest2-pdest1-1;
	else
		return 0;

	memcpy(devsn,pdest1+1,result);
	for(i=0;i<6;i++)
	{
		if(Devicemap[i].DevSn==devsn)
			return 0;
	}
	return 1;
}

BOOL GetDeviceProperty(HDEVINFO IntDevInfo, DWORD Index )
/*++

Routine Description:

    This routine enumerates the disk devices using the Device interface
    GUID DiskClassGuid. Gets the Adapter & Device property from the port
    driver. Then sends IOCTL through SPTI to get the device Inquiry data.

Arguments:

    IntDevInfo - Handles to the interface device information list

    Index      - Device member 

Return Value:

  TRUE / FALSE. This decides whether to continue or not

--*/
{
    SP_DEVICE_INTERFACE_DATA            interfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    interfaceDetailData = NULL;
    BOOL                                status;
    DWORD                               errorCode,reqSize,interfaceDetailDataSize;
    STORAGE_PROPERTY_QUERY              query;
    PSTORAGE_ADAPTER_DESCRIPTOR         adpDesc;
    UCHAR                               outBuf[512];
    ULONG                               returnedLength;

    interfaceData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);
	status = SetupDiEnumDeviceInterfaces ( 
                IntDevInfo,             // Interface Device Info handle
                0,                      // Device Info data
                (LPGUID)&DiskClassGuid, // Interface registered by driver
                Index,                  // Member
                &interfaceData          // Device Interface Data
                );

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode == ERROR_NO_MORE_ITEMS ) {
        }
        else {
        }
        return FALSE;
    }        

    // Find out required buffer size, so pass NULL 
    status = SetupDiGetDeviceInterfaceDetail (
                IntDevInfo,         // Interface Device info handle
                &interfaceData,     // Interface data for the event class
                NULL,               // Checking for buffer size
                0,                  // Checking for buffer size
                &reqSize,           // Buffer size required to get the detail data
                NULL                // Checking for buffer size
                );

    //
    // This call returns ERROR_INSUFFICIENT_BUFFER with reqSize 
    // set to the required buffer size. Ignore the above error and
    // pass a bigger buffer to get the detail data
    //

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode != ERROR_INSUFFICIENT_BUFFER ) {
            return FALSE;
        }
    }

    //
    // Allocate memory to get the interface detail data
    // This contains the devicepath we need to open the device
    //

    interfaceDetailDataSize = reqSize;
    interfaceDetailData =(PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc (interfaceDetailDataSize);
    if ( interfaceDetailData == NULL ) {
        return FALSE;
    }
    interfaceDetailData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);

    status = SetupDiGetDeviceInterfaceDetail (
                  IntDevInfo,               // Interface Device info handle
                  &interfaceData,           // Interface data for the event class
                  interfaceDetailData,      // Interface detail data
                  interfaceDetailDataSize,  // Interface detail data size
                  &reqSize,                 // Buffer size required to get the detail data
                  NULL);                    // Interface device info

    if ( status == FALSE ) {
        return 0;
    }
   
   Devicehandle[SSDID] = CreateFile(
				interfaceDetailData->DevicePath,    // device interface name
				GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
				FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
				NULL,                               // lpSecurityAttributes
				OPEN_EXISTING,                      // dwCreationDistribution
				0,                                  // dwFlagsAndAttributes
				NULL                                // hTemplateFile
				);
   if (Devicehandle[SSDID] == INVALID_HANDLE_VALUE) 
	    return 0;
   
   query.PropertyId = StorageAdapterProperty;
   query.QueryType = PropertyStandardQuery;

   status = DeviceIoControl(
                        Devicehandle[SSDID],                
                        IOCTL_STORAGE_QUERY_PROPERTY,
                        &query,
                        sizeof( STORAGE_PROPERTY_QUERY ),
                        &outBuf,                   
                        512,                      
                        &returnedLength,      
                        NULL                    
                        );
    
   adpDesc = (PSTORAGE_ADAPTER_DESCRIPTOR) outBuf;
   if( adpDesc->BusType==1)				//RAID 接口							
		InterfaceType[SSDID]=1;
   else if(adpDesc->BusType==7 )		//USB接口
		InterfaceType[SSDID]=7;
   else
	   InterfaceType[SSDID]=0;

	IDEREGS regs;
	regs.bFeaturesReg		= SLW_CMD_VND_SUB_ENDBG;
	regs.bSectorCountReg	= 0;
	regs.bSectorNumberReg	= 0x53;
	regs.bCylLowReg			= 0;
	regs.bCylHighReg		= 0;	
	regs.bDriveHeadReg		= 0;	
	regs.bCommandReg		= SLW_ATA_CMD_VNDNDCMD;	

	if( !ata_pass_through_ioctl_pio(SSDID, &regs, NULL, NULL, true) )
	{
		CloseHandle(Devicehandle[SSDID]);
		Devicehandle[SSDID] = INVALID_HANDLE_VALUE;
		return 1;
	}
	else
	   SSDID++;

	free (interfaceDetailData);
	return 1;
}

BOOL GetNewDeviceProperty(HDEVINFO IntDevInfo, DWORD Index,int sataport )
/*++

Routine Description:

    This routine enumerates the disk devices using the Device interface
    GUID DiskClassGuid. Gets the Adapter & Device property from the port
    driver. Then sends IOCTL through SPTI to get the device Inquiry data.

Arguments:

    IntDevInfo - Handles to the interface device information list

    Index      - Device member 

Return Value:

  TRUE / FALSE. This decides whether to continue or not

--*/
{
    SP_DEVICE_INTERFACE_DATA            interfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    interfaceDetailData = NULL;
    BOOL                                status;
    DWORD                               errorCode,reqSize,interfaceDetailDataSize;
	HANDLE handle;
    interfaceData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);
	status = SetupDiEnumDeviceInterfaces ( 
                IntDevInfo,             // Interface Device Info handle
                0,                      // Device Info data
                (LPGUID)&DiskClassGuid, // Interface registered by driver
                Index,                  // Member
                &interfaceData          // Device Interface Data
                );

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode == ERROR_NO_MORE_ITEMS ) {
        }
        else {
        }
        return FALSE;
    }        

    // Find out required buffer size, so pass NULL 
    status = SetupDiGetDeviceInterfaceDetail (
                IntDevInfo,         // Interface Device info handle
                &interfaceData,     // Interface data for the event class
                NULL,               // Checking for buffer size
                0,                  // Checking for buffer size
                &reqSize,           // Buffer size required to get the detail data
                NULL                // Checking for buffer size
                );

    //
    // This call returns ERROR_INSUFFICIENT_BUFFER with reqSize 
    // set to the required buffer size. Ignore the above error and
    // pass a bigger buffer to get the detail data
    //

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode != ERROR_INSUFFICIENT_BUFFER ) {
            return FALSE;
        }
    }

    //
    // Allocate memory to get the interface detail data
    // This contains the devicepath we need to open the device
    //

    interfaceDetailDataSize = reqSize;
    interfaceDetailData =(PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc (interfaceDetailDataSize);
    if ( interfaceDetailData == NULL ) {
        return FALSE;
    }
    interfaceDetailData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);

    status = SetupDiGetDeviceInterfaceDetail (
                  IntDevInfo,               // Interface Device info handle
                  &interfaceData,           // Interface data for the event class
                  interfaceDetailData,      // Interface detail data
                  interfaceDetailDataSize,  // Interface detail data size
                  &reqSize,                 // Buffer size required to get the detail data
                  NULL);                    // Interface device info

    if ( status == FALSE ) {
        return 0;
    }

   char devsn[128];
   memset(devsn,0,128);
	
   status=SearchDevSnMap(interfaceDetailData->DevicePath,devsn);	
   if(status == FALSE)
	   return 1;
   Devicehandle[sataport] = CreateFile(
				interfaceDetailData->DevicePath,    // device interface name
				GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
				FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
				NULL,                               // lpSecurityAttributes
				OPEN_EXISTING,                      // dwCreationDistribution
				0,                                  // dwFlagsAndAttributes
				NULL                                // hTemplateFile
				);
   if ( Devicehandle[sataport] == INVALID_HANDLE_VALUE) 
	    return 0;
   
   CString drive,SSDSN;
   BYTE Buffer[512];
   int i;
   memset(Buffer,0,512);

   	if(! IDFY_IDE(drive, Buffer,sataport) )
	{
		Sleep(1000);
		if(! IDFY_IDE(drive, Buffer,sataport) )
		{
			LogInfo(sataport,"****IDFY 失败!!!\r\n",3);
			return 0;
		}
	}
	
	SSDSN.Empty();
	for(i=10;i<20;i++)
	{
		SSDSN+=Buffer[2*i+1];
		SSDSN+=Buffer[2*i];
	}
	SSDSN.TrimRight();
	

	if( Devicemap[sataport].ID!=SSDSN )
	{
		CloseHandle(Devicehandle[sataport]);
		Devicehandle[sataport]=INVALID_HANDLE_VALUE;
		free (interfaceDetailData);
		return 1;
	}
	else
	{	   
	   Devicemap[sataport].DevSn=devsn;
	   free (interfaceDetailData);
	   return 1;
	}

}


BOOL GetRamDiskProperty(HDEVINFO IntDevInfo, DWORD Index )
{
    SP_DEVICE_INTERFACE_DATA            interfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    interfaceDetailData = NULL;
    BOOL                                status;
    DWORD                               errorCode,reqSize,interfaceDetailDataSize;
    STORAGE_PROPERTY_QUERY              query;
    PSTORAGE_ADAPTER_DESCRIPTOR         adpDesc;
    UCHAR                               outBuf[512];
    ULONG                               returnedLength;
   
    interfaceData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);
	status = SetupDiEnumDeviceInterfaces ( 
                IntDevInfo,             // Interface Device Info handle
                0,                      // Device Info data
                (LPGUID)&DiskClassGuid, // Interface registered by driver
                Index,                  // Member
                &interfaceData          // Device Interface Data
                );

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode == ERROR_NO_MORE_ITEMS ) {
        }
        else {
        }
        return FALSE;
    }        

    // Find out required buffer size, so pass NULL 
    status = SetupDiGetDeviceInterfaceDetail (
                IntDevInfo,         // Interface Device info handle
                &interfaceData,     // Interface data for the event class
                NULL,               // Checking for buffer size
                0,                  // Checking for buffer size
                &reqSize,           // Buffer size required to get the detail data
                NULL                // Checking for buffer size
                );


    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode != ERROR_INSUFFICIENT_BUFFER ) {
            return FALSE;
        }
    }


    interfaceDetailDataSize = reqSize;
    interfaceDetailData =(PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc (interfaceDetailDataSize);
    if ( interfaceDetailData == NULL ) {
        return FALSE;
    }
    interfaceDetailData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);

    status = SetupDiGetDeviceInterfaceDetail (
                  IntDevInfo,               // Interface Device info handle
                  &interfaceData,           // Interface data for the event class
                  interfaceDetailData,      // Interface detail data
                  interfaceDetailDataSize,  // Interface detail data size
                  &reqSize,                 // Buffer size required to get the detail data
                  NULL);                    // Interface device info

    if ( status == FALSE ) {
        return 0;
    }

   Devicehandle[SSDID] = CreateFile(
				interfaceDetailData->DevicePath,    // device interface name
				GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
				FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
				NULL,                               // lpSecurityAttributes
				OPEN_EXISTING,                      // dwCreationDistribution
				0,                                  // dwFlagsAndAttributes
				NULL                                // hTemplateFile
				);
   if (Devicehandle[SSDID] == INVALID_HANDLE_VALUE) 
	    return 0;
	
   query.PropertyId = StorageAdapterProperty;
   query.QueryType = PropertyStandardQuery;

   status = DeviceIoControl(
                        Devicehandle[SSDID],                
                        IOCTL_STORAGE_QUERY_PROPERTY,
                        &query,
                        sizeof( STORAGE_PROPERTY_QUERY ),
                        &outBuf,                   
                        512,                      
                        &returnedLength,      
                        NULL                    
                        );
    
   adpDesc = (PSTORAGE_ADAPTER_DESCRIPTOR) outBuf;
   if( adpDesc->BusType==1)				//RAID 接口							
		InterfaceType[SSDID]=1;
   else if(adpDesc->BusType==7 )		//USB接口
		InterfaceType[SSDID]=7;
   else
	   InterfaceType[SSDID]=0;

	BYTE buf[512];
	IDEREGS regs;
	
	regs.bFeaturesReg		= SLW_CMD_VND_SUB_ENDBG;
	regs.bSectorCountReg	= 0;
	regs.bSectorNumberReg	= 0x53;
	regs.bCylLowReg			= 0;
	regs.bCylHighReg		= 0;	
	regs.bDriveHeadReg		= 0;	
	regs.bCommandReg		= SLW_ATA_CMD_VNDNDCMD;	
	
	if( !ata_pass_through_ioctl_pio(SSDID, &regs, NULL, NULL, true) )
	{
		CloseHandle(Devicehandle[SSDID]);
		Devicehandle[SSDID] = INVALID_HANDLE_VALUE;
		return 1;
	}

	regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
	regs.bSectorCountReg	= 0x00;	
	regs.bSectorNumberReg	= 0;  // request command status
	regs.bCylLowReg			= 0x01;							// abort last command
	if( !PIO_VENDOR_OPS( SSDID,false, buf, 0, &regs))
	{
		CloseHandle(Devicehandle[SSDID]);
		Devicehandle[SSDID] = INVALID_HANDLE_VALUE;
		return 1;
	}
	
	else
	   SSDID++;

    free (interfaceDetailData);
    return 1;
}


int GetSSDStatus(int device)
{	
	int i;
	unsigned char CSWBuffer[512];
	CString drive;
	if(! IDFY_IDE(drive, CSWBuffer,device) )
	{
		AddInfo("****IDFY FAILED!!!\r\n");
		return -1;
	}
	for(i=216;i<223;i++)
	{
		if(CSWBuffer[i]!=0)
			return 1;
	}
	return 0;

}

void SetMultibootIdentify(PUCHAR buf,DWORD multibootsize,CString model)
{
	char output[256];
	int checksum=0,i;
	DWORD	tmp ;
	buf[62]=0x30;
	buf[63]=0x30;
	buf[64]=0x47;
	buf[65]=0x38;
		
	buf[166]=0;
	buf[167]=0x50;

	buf[172]=0;
	buf[173]=0x10;


	buf[120]=multibootsize;
	buf[121]=multibootsize>>8;
	buf[122]=multibootsize>>16;
	buf[123]=multibootsize>>24;

	buf[2]=multibootsize/(16*63);
	buf[3]=multibootsize/(16*63)>>8;

	buf[6]=0x10;
	buf[110]=0xf;

	buf[200]=buf[120];
	buf[201]=buf[121];
	buf[202]=buf[122];
	buf[203]=buf[123];

	buf[108]=multibootsize/(buf[110]*63);
	buf[109]=(multibootsize/(buf[110]*63))>>8;
	

	tmp = buf[108] | buf[109]<<8;
	buf[114]=tmp*15*63;
	buf[115]=(tmp*15*63)>>8;
	buf[116]=(tmp*15*63)>>16;
	buf[117]=(tmp*15*63)>>24;

	sprintf(output,"%s",model);
	model.TrimRight();
	for(i=0;i<model.GetLength();i++)
	{
		buf[54+i]=output[i];
	}
	for(i=model.GetLength();i<40;i++)
	{
		buf[54+i]=0x20;
	}
	for(i=27;i<47;i++)
	{
		output[0]=buf[2*i];
		buf[2*i]=buf[2*i+1];
		buf[2*i+1]=output[0];
	}

	buf[418]=0;
	buf[419]=0;
	buf[434]=0;
	buf[435]=0;

	buf[510]=0xa5;

	for(i=0;i<511;i++)
	{
		checksum+=buf[i];
	}
	buf[511]=~checksum+1;
}

void AddInfo(LPCTSTR buf)
{
	CString str;
	CString temp;
	
	str.Format("%s\r\n",buf);
#if _INITD 
	int len = pGuiServerExplorer->m_editresult.GetWindowTextLength();
	if(len >= 30000)
	{
		pGuiServerExplorer->m_editresult.GetWindowTextA(temp);
		temp = temp.Right(10000);
		pGuiServerExplorer->m_editresult.SetWindowTextA(temp);
		len = 10000;
	}
	pGuiServerExplorer->m_editresult.SetSel(len,len);
	pGuiServerExplorer->m_editresult.ReplaceSel(str);
	pGuiServerExplorer->m_editresult.SetFocus();
#endif
}

void LogInfo(int device,LPCTSTR buf,int loglevel)
{
	CString str;
	CString temp;
	
	str.Format("%s\r\n",buf);
#if _MASSINIT 
	FILE *fp;
	char file[128];

	if(BadBlockFilename[device].GetLength()==0)
		return ;
    GetModuleFileName(NULL,file,128); 
    //Scan a string for the last occurrence of a character.
    (strrchr(file,'\\'))[1] = 0; 
	strcat(file,"LOG\\");
	strcat(file,BadBlockFilename[device]);
	strcat(file,".log");
	fp = fopen(file, "a+");
	if(fp==NULL)
	{
		AfxMessageBox("Open write file fail!");
		return ;
	}
	fprintf(fp,"%s",str);
	fclose(fp);
#endif
}


/*
PRODUCTION SERIALNUMBER: 1C927A001
SAMSUNG 4GB MLC
CHANNEL = 4
ROW = 4
LAYER = 2
PLANE = 2
row = 1 1 1 1

void SLWUpdateBadblockFile(CString strDldFile)
{
	FILE *fp,*fpw;
	int a;
	int i,j;
	char output[128];
	CString str;
	CString filename;
	int idie,irow,ilayer;
	int badblocknum2=0;

	filename=strDldFile+".txt";
	fp = fopen(filename, "rb");
	if(fp==NULL)
	{
		AfxMessageBox("Open read file fail!");
		return ;
	}
	
	filename=strDldFile+".bck";
	fpw = fopen(filename, "w");
	if(fp==NULL)
	{
		AfxMessageBox("Open write file fail!");
		return ;
	}

	for(i=0;i<9;i++)
	{
		fscanf(fp,"%s ",output);
		if((i%3)==0 && i!=0)
			fprintf(fpw,"\r\n");
		fprintf(fpw,"%s ",output);

	}

	fscanf(fp,"%s ",output);
	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&irow);

	fscanf(fp,"%s ",output);
	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&ilayer);

	idie=irow*ilayer;
	fprintf(fpw,"\r\nDIE = %d\r\n",idie);	//写die
	grdie=idie;
	fscanf(fp,"%s ",output);
	fprintf(fpw,"%s ",output);
	fscanf(fp,"%s ",output);
	fprintf(fpw,"%s ",output);
	fscanf(fp,"%d",&a);
	fprintf(fpw,"%d \r\n",a);			//写plane
	
	fscanf(fp,"%s ",output);
	fscanf(fp,"%s ",output);
	for(i=0;i<4;i++)
	{
		fscanf(fp,"%d",&a);
	}

	for(i=0;i<channel*grdie;i++)
	{
		for(j=0;j<8;j++)
		{
			fscanf(fp,"%s ",output);
			str=output;	
			if(j==1)
				ilayer=atol(output);
			else if(j==3)
			{
				irow=atol(output);
				idie=irow+ilayer*8;
				fprintf(fpw,"DIE %d ",idie);	//写die
			}
			else if(j>3)
			{
				fprintf(fpw,"%s ",output);
			}
			if(str=="End!")
			{
				m_result+="\r\n\r\n****文件格式不正确!!!\r\n\r\n";
				fclose(fp);
				return ;
			}
		}
		fscanf(fp,"%d ",&xblock[GlobalDevice][i]);
		fprintf(fpw,"%d \r\n",xblock[GlobalDevice][i]);
		for(j=0;j<xblock[GlobalDevice][i];j++)
		{
			fscanf(fp,"%d ",&badblock[GlobalDevice][i][j]);
			if(idie>=8)
				badblock[GlobalDevice][i][j]=badblock[GlobalDevice][i][j]+4096;
			fprintf(fpw,"%d ",badblock[GlobalDevice][i][j]);
		}
		fprintf(fpw,"\r\n");
	}
	fscanf(fp,"%s ",output);
	fprintf(fpw,"%s ",output);
	str=output;	
	if(str!="End!")
	{
		m_result+="\r\n\r\n****文件格式不正确!!!\r\n\r\n";
		fclose(fp);
		return ;
	}
	
	fclose(fp);
	fclose(fpw);
	return ;
}
*/

/*对应格式
PRODUCTION SERIALNUMBER: 1C8401013
SAMSUNG 4GB MLC
CHANNEL = 4
ROW = 4
LAYER = 2
PLANE = 4
row = 1 1 1 1

void SLWUpdateBadblockFile(CString strDldFile)
{
	FILE *fp,*fpw;
	int a;
	int i,j;
	char output[128];
	CString str;
	CString filename;
	int idie,irow,ilayer;
	int badblocknum2=0;

	filename=strDldFile+".txt";
	fp = fopen(filename, "rb");
	if(fp==NULL)
	{
		AfxMessageBox("Open read file fail!");
		return ;
	}
	
	filename=strDldFile+".bck";
	fpw = fopen(filename, "w");
	if(fp==NULL)
	{
		AfxMessageBox("Open write file fail!");
		return ;
	}

	for(i=0;i<9;i++)
	{
		fscanf(fp,"%s ",output);
		if((i%3)==0 && i!=0)
			fprintf(fpw,"\r\n");
		fprintf(fpw,"%s ",output);

	}

	fscanf(fp,"%s ",output);
	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&irow);

	fscanf(fp,"%s ",output);
	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&ilayer);

	idie=irow*ilayer*2;
	fprintf(fpw,"\r\nDIE = %d\r\n",idie);	//写die

	fscanf(fp,"%s ",output);
	fprintf(fpw,"%s ",output);
	fscanf(fp,"%s ",output);
	fprintf(fpw,"%s ",output);
	fscanf(fp,"%d",&a);
	fprintf(fpw,"%d \r\n",a/2);			//写plane
	
	fscanf(fp,"%s ",output);
	fscanf(fp,"%s ",output);
	for(i=0;i<4;i++)
	{
		fscanf(fp,"%d",&a);
	}
	for(i=0;i<channel*gcs;i++)
	{
		str.Empty();
		for(j=0;j<8;j++)
		{
			fscanf(fp,"%s ",output);
			if(j==1)
				ilayer=atol(output);
			else if(j==3)
			{
				irow=atol(output);
				idie=irow+ilayer*4;
			//	fprintf(fpw,"DIE %d ",idie);	//写die
			}
			else if(j>3)
			{
			//	fprintf(fpw,"%s ",output);				
				str+=output;
				str+=" ";
			}
		}
		fscanf(fp,"%d ",&xblock[GlobalDevice][i]);
		badblocknum2=0;
		for(j=0;j<xblock[GlobalDevice][i];j++)
		{
			fscanf(fp,"%d ",&badblock[GlobalDevice][i][j]);
			if(badblock[GlobalDevice][i][j]>=4096)
				badblocknum2++;
		}

		//写第一个die的坏块
		fprintf(fpw,"DIE %d ",idie);
		fprintf(fpw,"%s ",str.GetBuffer(0));
		fprintf(fpw," %d",xblock[GlobalDevice][i]-badblocknum2);
		fprintf(fpw,"\r\n");
		for(j=0;j<xblock[GlobalDevice][i]-badblocknum2;j++)
		{
			fprintf(fpw,"%d ",badblock[GlobalDevice][i][j]);

		}
		fprintf(fpw,"\r\n");
		//写第二个die的坏块
		fprintf(fpw,"DIE %d ",idie+8);
		fprintf(fpw,"%s ",str.GetBuffer(0));
		fprintf(fpw," %d",badblocknum2);
		fprintf(fpw,"\r\n");
		for(j=xblock[GlobalDevice][i]-badblocknum2;j<xblock[GlobalDevice][i];j++)
		{
			fprintf(fpw,"%d ",badblock[GlobalDevice][i][j]);
		}
		fprintf(fpw,"\r\n");
	}
	fscanf(fp,"%s ",output);
	fprintf(fpw,"%s ",output);
	str=output;	
	if(str!="End!")
	{
		m_result+="\r\n\r\n****文件格式不正确!!!\r\n\r\n";
		fclose(fp);
		return ;
	}
	
	fclose(fp);
	fclose(fpw);
	return ;
}
*/


/*对应格式

PRODUCTION SERIALNUMBER: 1B927A001
SAMSUNG 2GB SLC
CHANNEL = 4
ROW = 4
LAYER = 1
PLANE = 4
row = 1 1 1 1*/
void SLWUpdateBadblockFile(CString strDldFile)
{
	FILE *fp,*fpw;
	int a;
	int i,j;
	char output[128];
	CString str;
	CString filename;
	int idie,irow,ilayer;
	int badblocknum2=0;

	filename=strDldFile+".txt";
	fp = fopen(filename, "rb");
	if(fp==NULL)
	{
		AfxMessageBox("Open read file fail!");
		return ;
	}
	
	filename=strDldFile+".bck";
	fpw = fopen(filename, "w");
	if(fp==NULL)
	{
		AfxMessageBox("Open write file fail!");
		return ;
	}

	for(i=0;i<9;i++)
	{
		fscanf(fp,"%s ",output);
		if((i%3)==0 && i!=0)
			fprintf(fpw,"\r\n");
		fprintf(fpw,"%s ",output);

	}

	fscanf(fp,"%s ",output);
	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&irow);

	fscanf(fp,"%s ",output);
	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&ilayer);

	idie=irow*ilayer*2;
	fprintf(fpw,"\r\nDIE = %d\r\n",idie);	//写die

	fscanf(fp,"%s ",output);
	fprintf(fpw,"%s ",output);
	fscanf(fp,"%s ",output);
	fprintf(fpw,"%s ",output);
	fscanf(fp,"%d",&a);
	fprintf(fpw,"%d \r\n",a/2);			//写plane
	
	fscanf(fp,"%s ",output);
	fscanf(fp,"%s ",output);
	for(i=0;i<4;i++)
	{
		fscanf(fp,"%d",&a);
	}
	gcs=4;
	for(i=0;i<channel*gcs;i++)
	{
		str.Empty();
		for(j=0;j<8;j++)
		{
			fscanf(fp,"%s ",output);
			if(j==1)
				ilayer=atol(output);
			else if(j==3)
			{
				irow=atol(output);
				idie=irow;
			//	fprintf(fpw,"DIE %d ",idie);	//写die
			}
			else if(j>3)
			{
			//	fprintf(fpw,"%s ",output);				
				str+=output;
				str+=" ";
			}
		}
		fscanf(fp,"%d ",&xblock[GlobalDevice][i]);
		badblocknum2=0;
		for(j=0;j<xblock[GlobalDevice][i];j++)
		{
			fscanf(fp,"%d ",&badblock[GlobalDevice][i][j]);
			if(badblock[GlobalDevice][i][j]>=4096)
				badblocknum2++;
		}

		//写第一个die的坏块
		fprintf(fpw,"DIE %d ",idie);
		fprintf(fpw,"%s ",str.GetBuffer(0));
		fprintf(fpw," %d",xblock[GlobalDevice][i]-badblocknum2);
		fprintf(fpw,"\r\n");
		for(j=0;j<xblock[GlobalDevice][i]-badblocknum2;j++)
		{
			fprintf(fpw,"%d ",badblock[GlobalDevice][i][j]);

		}
		fprintf(fpw,"\r\n");
		//写第二个die的坏块
		fprintf(fpw,"DIE %d ",idie+8);
		fprintf(fpw,"%s ",str.GetBuffer(0));
		fprintf(fpw," %d",badblocknum2);
		fprintf(fpw,"\r\n");
		for(j=xblock[GlobalDevice][i]-badblocknum2;j<xblock[GlobalDevice][i];j++)
		{
			fprintf(fpw,"%d ",badblock[GlobalDevice][i][j]);
		}
		fprintf(fpw,"\r\n");
	}
	fscanf(fp,"%s ",output);
	fprintf(fpw,"%s ",output);
	str=output;	
	if(str!="End!")
	{
		m_result+="\r\n\r\n****文件格式不正确!!!\r\n\r\n";
		fclose(fp);
		return ;
	}
	
	fclose(fp);
	fclose(fpw);
	return ;
}



BOOL SLWDownSRAM(int device,CString strDlgFile)
{
	// load FW
	
	PUCHAR DATABuffer;
	int datasize = 64*1024;
	DATABuffer=(PUCHAR)malloc(datasize);
	memset(DATABuffer, 0xFF, datasize);

	CFile fl;
	if( !fl.Open(strDlgFile, CFile::modeRead, NULL) )
	{
		AddInfo("Open File Failed !");
		free(DATABuffer);
		return 0;		
	}

	if(fl.GetLength() > datasize )
	{
		AddInfo("File size should below 64 x 1024KB !");
		free(DATABuffer);
		return 0;	
	}

	if( !fl.Read(DATABuffer, fl.GetLength()) )
	{
		AddInfo("Read File Failed !");
		free(DATABuffer);
		fl.Close();
		return 0;	
	}
	fl.Close();
	
	// download FW

	BYTE ch;
	int i;
	//LOADER最大64K，目前一般不超过29K，在SECTOR 61写入SSD FLASH参数。
	int addr=512*61;
	BYTE C1,C2;
	DATABuffer[addr]=0x77;
	DATABuffer[addr+1]=0xAA;
	DATABuffer[addr+2]=0x66;
	DATABuffer[addr+3]=0x55;

	DATABuffer[addr+4]=flash_type.ID[0];
	DATABuffer[addr+5]=flash_type.ID[1];
	DATABuffer[addr+6]=flash_type.ID[2];
	DATABuffer[addr+7]=flash_type.ID[3];
	GetFlashTypePar(&C1,&C2);
	DATABuffer[addr+12]=C1;
	DATABuffer[addr+13]=C2;

	for(i=0;i < datasize/2;i++)
	{
		ch = DATABuffer[2*i];
		DATABuffer[2*i] = DATABuffer[2*i+1];
		DATABuffer[2*i+1] = ch;
	}
	

	if( !DMA_OPS(device,DATABuffer, datasize, 0x00, true, ata_WRITE_DMA))  // LBA > 0x1000000
	{
		free(DATABuffer);
		AddInfo("DMA WRITE FW Failed!");
		return 0;
	}

	PUCHAR buf;
	buf=(PUCHAR)malloc(datasize);
	if( !DMA_OPS(device,buf, datasize, 0x00, false, ata_READ_DMA))  // LBA > 0x1000000
	{
		free(DATABuffer);
		free(buf);
		AddInfo("DMA READ FW Failed!");
		return 0;
	}

	for(i = 0; i < datasize; i++)
	{
		if( *(DATABuffer + i) != *(buf + i))
		{
			CString str;
			str.Format("diff at: %d", i);
			AddInfo(str);
			str.Format("EX: %02X %02X %02X %02X", *(DATABuffer + i), *(DATABuffer + i +1), *(DATABuffer + i +2), *(DATABuffer + i +3));
			AddInfo(str);
			str.Format("AC: %02X %02X %02X %02X", *(buf + i), *(buf + i +1), *(buf + i +2), *(buf + i +3));
			AddInfo(str);
			free(DATABuffer);
			free(buf);
			return 0;
		}
	}
	free(buf);
	
	if( !SLW_SSD_DBG(device,true))
	{
		free(DATABuffer);
		AddInfo("**Can't ENTER DBG MODE!**");
		return 0;
	}
	
	IDEREGS regs;
	regs.bFeaturesReg		= SLW_CMD_VND_SUB_DWN_FW_RUN;
	regs.bSectorCountReg	= 0;	
	regs.bSectorNumberReg	= 0x5F;

	if( !PIO_VENDOR_OPS( device,true, DATABuffer, 0, &regs))
	{
		free(DATABuffer);
		AddInfo("****Run FW Failed!\r\n");
		return 0;

	}
	else
	{
		free(DATABuffer);
		AddInfo("****Run FW successfully!\r\n");
		return 1;
	}
}

BOOL SLWUpdateLoadFw(int device)
{
	UINT datasize = 64 * 1024;
	BYTE DATABuffer[64 * 1024];
	memset(DATABuffer, 0xFF, 64 * 1024);
	FILE *fp;
	CString strDldFile="";	
	int readnum;
	int line;

#if _MASSINIT 
	char filename[128];
	GetModuleFileName(NULL,filename,128); 
	(strrchr(filename,'\\'))[1] = 0; 
	strDldFile = filename+LoadFilename;
#endif

	fp = fopen(strDldFile, "rb");
	if(fp)
	{
		line=0;
		while(1)
		{
			readnum=fread(DATABuffer + 32*line, 1, 32, fp);
			if(readnum==0)
			{
				break;
			}				
			line++;
		}	
	}
	else
	{
		AddInfo("打开.b文件失败!");
		return 0;
	}
	fclose(fp);

	//LOADER最大64K，目前一般不超过29K，在SECTOR 61写入SSD FLASH参数。
	int addr=512*61;
	BYTE C1,C2;
	DATABuffer[addr]=0x77;
	DATABuffer[addr+1]=0xAA;
	DATABuffer[addr+2]=0x66;
	DATABuffer[addr+3]=0x55;

	DATABuffer[addr+4]=flash_type.ID[0];
	DATABuffer[addr+5]=flash_type.ID[1];
	DATABuffer[addr+6]=flash_type.ID[2];
	DATABuffer[addr+7]=flash_type.ID[3];
	GetFlashTypePar(&C1,&C2);
	DATABuffer[addr+12]=C1;
	DATABuffer[addr+13]=C2;

	if(VNDWriteSPI(device,DATABuffer,0,12))
	{
		AddInfo("Update Fw Loader successfully!");
		return 1;
	}
	else
	{
		AddInfo("Update Fw Loader failed!");
		return 0;
	}
}

int slwatoh(char c)
{
	int hex=0;
	if(c >='0'&& c<='9')
		hex=c-'0';
	else if(c >='a'&& c<='f')
		hex=c-'a'+10;
	else if(c >='A'&& c<='F')
		hex=c-'A'+10;
	return hex;
}

BOOL GetFlashTypePar(BYTE	*type, BYTE	*par)
{
	BYTE flashp1,flashp2,flashp3,flashp4,flashp5,flashp6;

	if(flash_type.size_page==4)
		flashp1=2;
	if(flash_type.size_page==8)
		flashp1=3;

	if(flash_type.size_block==256)
		flashp2=2;
	if(flash_type.size_block==512)
		flashp2=3;

	
	if(flash_type.blocknum==2048)
		flashp6=1;
	if(flash_type.blocknum==4096)
		flashp6=2;


	if(diepercs==1) //每个CS两个DIE
	{
		flashp3=1;
		flashp5=1;
	}
	else
	{
		flashp3=0;
		flashp5=0;
	}

	if(flash_type.pages_per_block==64)
		flashp4=0;
	if(flash_type.pages_per_block==128)
		flashp4=1;

	*type=(unsigned char)( flashp1 | (flashp2<<4) | (flashp6<<6) );
	*par=(unsigned char)( flashp3 | (flashp4<<2) | (flashp5<<6) );

	return 1;
}

BOOL VNDSetFLASHArg(int device)
{
	CString str;
	BYTE buf[512];
	BYTE C1,C2;
	IDEREGS regs;
	regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
	regs.bSectorCountReg	= 0x00;	
	regs.bSectorNumberReg	= 0;  
	regs.bCylLowReg			= 0x01;							// abort last command
	if( !PIO_VENDOR_OPS(device, false, buf, 0, &regs))
	{
		AddInfo("*Abort command rejected*");
		return 0;
	}
	
	GetFlashTypePar(&C1,&C2);
		
	regs.bFeaturesReg		=  SLW_CMD_VND_SUB_SETFLASHPARA;	// send  command
	regs.bSectorCountReg	= C1;			
	regs.bSectorNumberReg	= C2;					
	regs.bCylLowReg	= 0;
	regs.bCylHighReg = 0;
	regs.bDriveHeadReg = 0;	

	if( !PIO_VENDOR_OPS(device, true, buf, 0, &regs))
	{
		AddInfo("* command rejected*");
		return 0;
	}

	int timeout = 10;
	while(timeout--)
	{
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= SLW_CMD_VND_SUB_RW_PARA;  // request command status
		regs.bCylLowReg			= 0x00;
		if( !PIO_VENDOR_OPS(device, false, buf, 0, &regs))
		{
			AddInfo("*READSTA command rejected*");
			return 0;
		}

		if( regs.bCylHighReg != 0xFF)
		{
			str.Format(" command status:%02X. SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
			AddInfo(str);
		}
		else
		{
			str.Format("Command finished. status:%02X, SM:%d",regs.bCylHighReg, regs.bFeaturesReg);
		//	AddInfo(str);
			break;
		}
	}
	if(timeout < 0)
	{
		AddInfo("*Command busy and timeout*");
		AddInfo("*cancel command *");
		
		regs.bFeaturesReg		= SLW_CMD_VND_SUB_READSTA;		// read status
		regs.bSectorCountReg	= 0x00;	
		regs.bSectorNumberReg	= SLW_CMD_VND_SUB_RW_PARA;  // request command status
		regs.bCylLowReg			= 0x01;							// abort this command
		if( !PIO_VENDOR_OPS(device, false, buf, 0, &regs))
		{
			AddInfo("*Abort command rejected*");
		}
		return 0;
	}

	return 1;
}


/*


  SW80 函数 开始


*/
BOOL SW80FindRamDisk()
{
	HDEVINFO        hIntDevInfo;
    DWORD           index;
    BOOL            status;
    //
    // Open the device using device interface registered by the driver
    //

    //
    // Get the interface device information set that contains all devices of event class.
    //
	int i;
	for(i=0;i<6;i++)
		Devicehandle[i]=INVALID_HANDLE_VALUE;

    hIntDevInfo = SetupDiGetClassDevs (
                 (LPGUID)&DiskClassGuid,
                 NULL,                                   // Enumerator
                 NULL,                                   // Parent Window
                 (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE  // Only Devices present & Interface class
                 ));

    if( hIntDevInfo == INVALID_HANDLE_VALUE ) {
        return 0;
    }

    //
    //  Enumerate all the disk devices
    //
    index = 0;
	SSDID = 0;
    while (TRUE) 
    {
        status = SW80GetDeviceProperty( hIntDevInfo, index );
        if ( status == FALSE ) {
            break;
        }
        index++;
    }
    SetupDiDestroyDeviceInfoList(hIntDevInfo);
	if(Devicehandle[0]!=INVALID_HANDLE_VALUE)
		return 1;
	else 
		return 0;
}

ULONGLONG SW80GetVendorLBA(int device)
{
	ULONGLONG vendorlba;
	BYTE databuf[512];
	CString str;
	if(IDFY_IDE(str,databuf,device)==0)
		return 0;
	else
	{
		vendorlba=databuf[203]<<24 | databuf[202]<<16 | databuf[201]<< 8 | databuf[200];
		vendorlba=vendorlba+0x800000;
		return vendorlba;
	}
}

BOOL SW80IDFYSSD(int device,ULONGLONG vendorlba)
{
	BYTE buffer[512];
	BOOL ret;
	int i;
	CString str;
	memset(buffer,0,512);

	/*LBA值不对，直接退出*/
	if(vendorlba==0 || vendorlba==0x800000)
		return 0;

	ret=ReadSectors(device,vendorlba,1,buffer);
			return ret;
	if(ret==0)
		return ret;

	
	for(i=256;i<270;i++)
		str+=buffer[i];

	if(str.Compare("WS08R_MAD_SI_K")==0)
		return 1;
	else
		return 0;

}

BOOL SW80GetDeviceProperty(HDEVINFO IntDevInfo, DWORD Index )
/*++

Routine Description:

    This routine enumerates the disk devices using the Device interface
    GUID DiskClassGuid. Gets the Adapter & Device property from the port
    driver. Then sends IOCTL through SPTI to get the device Inquiry data.

Arguments:

    IntDevInfo - Handles to the interface device information list

    Index      - Device member 

Return Value:

  TRUE / FALSE. This decides whether to continue or not

--*/
{
   SP_DEVICE_INTERFACE_DATA            interfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    interfaceDetailData = NULL;
    BOOL                                status;
    DWORD                               errorCode,reqSize,interfaceDetailDataSize;
   	STORAGE_PROPERTY_QUERY              query;
    PSTORAGE_ADAPTER_DESCRIPTOR         adpDesc;
    UCHAR                               outBuf[512];
    ULONG                               returnedLength;
	
    interfaceData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);
	status = SetupDiEnumDeviceInterfaces ( 
                IntDevInfo,             // Interface Device Info handle
                0,                      // Device Info data
                (LPGUID)&DiskClassGuid, // Interface registered by driver
                Index,                  // Member
                &interfaceData          // Device Interface Data
                );

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode == ERROR_NO_MORE_ITEMS ) {
        }
        else {
        }
        return FALSE;
    }        

    // Find out required buffer size, so pass NULL 
    status = SetupDiGetDeviceInterfaceDetail (
                IntDevInfo,         // Interface Device info handle
                &interfaceData,     // Interface data for the event class
                NULL,               // Checking for buffer size
                0,                  // Checking for buffer size
                &reqSize,           // Buffer size required to get the detail data
                NULL                // Checking for buffer size
                );

    //
    // This call returns ERROR_INSUFFICIENT_BUFFER with reqSize 
    // set to the required buffer size. Ignore the above error and
    // pass a bigger buffer to get the detail data
    //

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode != ERROR_INSUFFICIENT_BUFFER ) {
            return FALSE;
        }
    }

    //
    // Allocate memory to get the interface detail data
    // This contains the devicepath we need to open the device
    //

    interfaceDetailDataSize = reqSize;
    interfaceDetailData =(PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc (interfaceDetailDataSize);
    if ( interfaceDetailData == NULL ) {
        return FALSE;
    }
    interfaceDetailData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);

    status = SetupDiGetDeviceInterfaceDetail (
                  IntDevInfo,               // Interface Device info handle
                  &interfaceData,           // Interface data for the event class
                  interfaceDetailData,      // Interface detail data
                  interfaceDetailDataSize,  // Interface detail data size
                  &reqSize,                 // Buffer size required to get the detail data
                  NULL);                    // Interface device info

    if ( status == FALSE ) {
        return 0;
    }
	
   Devicehandle[SSDID] = CreateFile(
				interfaceDetailData->DevicePath,    // device interface name
				GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
				FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
				NULL,                               // lpSecurityAttributes
				OPEN_EXISTING,                      // dwCreationDistribution
				0,                                  // dwFlagsAndAttributes
				NULL                                // hTemplateFile
				);
   if (Devicehandle[SSDID] == INVALID_HANDLE_VALUE) 
	    return 0;

   query.PropertyId = StorageAdapterProperty;
   query.QueryType = PropertyStandardQuery;

   status = DeviceIoControl(
                        Devicehandle[SSDID],                
                        IOCTL_STORAGE_QUERY_PROPERTY,
                        &query,
                        sizeof( STORAGE_PROPERTY_QUERY ),
                        &outBuf,                   
                        512,                      
                        &returnedLength,      
                        NULL                    
                        );
    
   adpDesc = (PSTORAGE_ADAPTER_DESCRIPTOR) outBuf;
   if(adpDesc->BusType==7)
		InterfaceType[SSDID]=1;
   else
		InterfaceType[SSDID]=0;


   ULONGLONG vendorlba;
   vendorlba=SW80GetVendorLBA(SSDID);

   if(strstr(interfaceDetailData->DevicePath,"gucssd")==NULL &&  SW80DistSSD(SSDID)==0 )
// if(strstr(interfaceDetailData->DevicePath,"gucssd")==NULL /* && SW80IDFYSSD(SSDID,vendorlba)==0 */&& SW80SSDVenderMode(SSDID,vendorlba,1)==0 )
   {
       free (interfaceDetailData);
	   CloseHandle(Devicehandle[SSDID]);
	   Devicehandle[SSDID] = INVALID_HANDLE_VALUE;
	   return 1;
   }	
   
	SSDID++;
	if(strstr(interfaceDetailData->DevicePath,"gucssd"))
		gucflag=1;
	else
		gucflag=0;

    free (interfaceDetailData);
    return 1;
}

int SW80DistSSD(int device)
/*
根据IDFY的值判断是不是SW80 SSD
返回值
1: SW80 RAM DISK
2: SW80 SSD(量产完成后的盘)
0: 非SW80 系列
*/
{	unsigned char SSDflag[32]="SOLIWARE-80",RAMflag[32]="SW80_RAM_DISK_V1",tmp[32];
	BYTE databuf[512];
	CString model;
	int i;
	if(IDFY_IDE(model,databuf,device)==0)
		return 0;
	
	model.Empty();
	for(i=27;i<47;i++)
	{
		model+=databuf[2*i+1];
		model+=databuf[2*i];
	}
	model.TrimRight();
	if(memcmp(model,RAMflag,16)==0)
		return 1;

	for(i=0;i<16;i++)
		tmp[i]=databuf[258+i];

	if(memcmp(tmp,SSDflag,11)==0)
		return 1;

	return 0;
}

BOOL DownSRAM(int device) 
{
	CString strDlgFile;
	PUCHAR DATABuffer;
	int datasize = 64*512;	

	DATABuffer=(PUCHAR)malloc(datasize);
	memset(DATABuffer, 0, datasize);


	char filename[128];
	GetModuleFileName(NULL,filename,128); 
	(strrchr(filename,'\\'))[1] = 0;
	strDlgFile=filename;
	strDlgFile+=LoadFilename;

	CFile fl;
	if( !fl.Open(strDlgFile, CFile::modeRead, NULL) )
	{
		AfxMessageBox("Open File Failed !");
		free(DATABuffer);
		return 0;		
	}

	if(fl.GetLength() > datasize )
	{
		AfxMessageBox("File size should below 32KB !");
		free(DATABuffer);
		return 0;	
	}

	if( !fl.Read(DATABuffer, fl.GetLength()) )
	{
		AfxMessageBox("Read File Failed !");
		free(DATABuffer);
		fl.Close();
		return 0;	
	}
	fl.Close();

	if(gucflag)
	{
		if(SW80WriteRam(device,FW_POS)==0)
		{
			AfxMessageBox("Down FW failed!");
			free(DATABuffer);
			return 0;
		}
		Sleep(50);

		if(WriteSectors(device,0,64,DATABuffer)==0)
		{
			AfxMessageBox("Down FW failed");
			free(DATABuffer);
			return 0;
		}

		Sleep(50);
		if(SW80RunFW(device,FW_POS)==0)
		{
			AfxMessageBox("Run FW failed");
			free(DATABuffer);
			return 0;
		}
	}

	free(DATABuffer);
	CloseSSDHandle();
	return 1;		
}

BOOL SW80WriteRam(int device,DWORD Addr)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"GUCSSDTOOLING.");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=SW80_CMD_VND_SUB_WSRAM;
	buffer[16]=BYTE( (Addr >> 0) & 0xFF);
	buffer[17]=BYTE( (Addr >> 8) & 0xFF);
	buffer[18]=BYTE( (Addr >> 16) & 0xFF);
	buffer[19]=BYTE( (Addr >> 24) & 0x0F);
	buffer[20]=SW80_CMD_VND;
	ret=WriteSectors(device,0,1,buffer);
	return ret;
}


BOOL SW80RunFW(int device,DWORD Addr)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"GUCSSDTOOLING.");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=SW80_CMD_VND_SUB_FW_RUN;
	buffer[16]=BYTE( (Addr >> 0) & 0xFF);
	buffer[17]=BYTE( (Addr >> 8) & 0xFF);
	buffer[18]=BYTE( (Addr >> 16) & 0xFF);
	buffer[19]=BYTE( (Addr >> 24) & 0x0F);
	buffer[20]=SW80_CMD_VND;
	ret=WriteSectors(device,0,1,buffer);
	return ret;
}

BOOL SW80RunSWFW(int device, BYTE *buf)
{
	int ret;
	BYTE buffer[512*57];
	memset(buffer,0,512*57);
	strcpy((char*)buffer,"WS08R_MAD_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=SW80_CMD_VND_SUB_RUNFWSW;
	buffer[20]=SW80_CMD_VND;
	memcpy(buffer+512,buf,512*56);
	ret=WriteSectors(device,0xF800000,57,buffer);
	return ret;
}

BOOL SW80ReadFlashid(int device,int channel,int die,BYTE *ID)
{
	BYTE buffer[512],readbuf[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08R_MAD_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=SW80_CMD_VND_SUB_READFLASHID;
	buffer[16]=die;
	buffer[17]=channel;
	buffer[20]=SW80_CMD_VND;
	ret=WriteSectors(device,0xF800000,1,buffer);
	if(ret==0)
		return ret;
	Sleep(50);
	ret=ReadSectors(device,0xF800000,1,readbuf);
	if(ret==0)
		return ret;

	if(readbuf[0]=='F'&&readbuf[1]=='I')
	{
		ID[0]=readbuf[26];
		ID[1]=readbuf[27];
		ID[2]=readbuf[28];
		ID[3]=readbuf[29];
		ID[4]=readbuf[30];
	}
	else
		return 0;

	return 1;
}

BOOL SW80Eraseblock(int device,int channel,int die,int block)
{
	BYTE buffer[512],readbuf[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08R_MAD_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=SW80_CMD_VND_SUB_ERASEBLOCK;
	buffer[16]=die;
	buffer[17]=channel;
	buffer[18]=block;
	buffer[19]=block>>8;
	buffer[20]=SW80_CMD_VND;
	ret=WriteSectors(device,0xF800000,1,buffer);
	if(ret==0)
		return ret;

/*	
	Sleep(1);
*/
	ret=ReadSectors(device,0xF800000,1,readbuf);
	if(ret==0)
		return ret;

	if(readbuf[0]=='F'&&readbuf[1]=='I')
		ret=(readbuf[14] & 0x1);
	else
		return 0;

	return !ret;
}

BOOL SW80WriteFWInfo(int device,PUCHAR DATABuffer,long offset)	
{
	long addptr;
	int i,j;
	int respageperpb;

	j=SLWFlashIDPos();
	if(j>=ConfNum)
		return 0;

	addptr=offset;


	//标志，0x221155AA
	DATABuffer[0x50+addptr]=0xAA;		
	DATABuffer[0x51+addptr]=0x55;		
	DATABuffer[0x52+addptr]=0x11;
	DATABuffer[0x53+addptr]=0x22;

	//PLL Clock, 默认值0xFFFF4977, 150MHz
	DATABuffer[0x60+addptr]=FlashEndurance[j].PllClock;		
	DATABuffer[0x61+addptr]=FlashEndurance[j].PllClock>>8;		
	DATABuffer[0x62+addptr]=FlashEndurance[j].PllClock>>16;
	DATABuffer[0x63+addptr]=FlashEndurance[j].PllClock>>24;

	//NFC time register 1, 默认值0X0B0E0B1F
	DATABuffer[0x64+addptr]=FlashEndurance[j].NFCtimeregister1;		
	DATABuffer[0x65+addptr]=FlashEndurance[j].NFCtimeregister1>>8;		
	DATABuffer[0x66+addptr]=FlashEndurance[j].NFCtimeregister1>>16;
	DATABuffer[0x67+addptr]=FlashEndurance[j].NFCtimeregister1>>24;

	//NFC time register 2, 默认值0X1F0B0000
	DATABuffer[0x68+addptr]=FlashEndurance[j].NFCtimeregister2;		
	DATABuffer[0x69+addptr]=FlashEndurance[j].NFCtimeregister2>>8;		
	DATABuffer[0x6A+addptr]=FlashEndurance[j].NFCtimeregister2>>16;
	DATABuffer[0x6B+addptr]=FlashEndurance[j].NFCtimeregister2>>24;

	//NFC time register 3, 配置NFC timing
	DATABuffer[0x6C+addptr]=FlashEndurance[j].NFCtimeregister3;		
	DATABuffer[0x6D+addptr]=FlashEndurance[j].NFCtimeregister3>>8;		
	DATABuffer[0x6E+addptr]=FlashEndurance[j].NFCtimeregister3>>16;
	DATABuffer[0x6F+addptr]=FlashEndurance[j].NFCtimeregister3>>24;

	//NFC BCH value register
	DATABuffer[0x70+addptr]=FlashEndurance[j].NFCBCH;		
	DATABuffer[0x71+addptr]=FlashEndurance[j].NFCBCH>>8;		
	DATABuffer[0x72+addptr]=FlashEndurance[j].NFCBCH>>16;
	DATABuffer[0x73+addptr]=FlashEndurance[j].NFCBCH>>24;

	//Flash ID，4B
	DATABuffer[0x74+addptr]=FlashEndurance[j].ID[0];		
	DATABuffer[0x75+addptr]=FlashEndurance[j].ID[1];		
	DATABuffer[0x76+addptr]=FlashEndurance[j].ID[2];
	DATABuffer[0x77+addptr]=FlashEndurance[j].ID[3];

	//Spare area size
	DATABuffer[0x78+addptr]=FlashEndurance[j].Sparesize;		
	DATABuffer[0x79+addptr]=FlashEndurance[j].Sparesize>>8;	

	//Page Mark 0，原始坏块需要检查的第一个page
	DATABuffer[0x7A+addptr]=FlashEndurance[j].Pagemark0;		
	DATABuffer[0x7B+addptr]=FlashEndurance[j].Pagemark0>>8;	

	//Page Mark 1，原始坏块需要检查的第二个page,如果不存在，则置为0xFFFF
	DATABuffer[0x7C+addptr]=FlashEndurance[j].Pagemark1;		
	DATABuffer[0x7D+addptr]=FlashEndurance[j].Pagemark1>>8;	

	//Byte Mark, 原始坏块需要检查的col address
	DATABuffer[0x7E+addptr]=FlashEndurance[j].Bytemark;		
	DATABuffer[0x7F+addptr]=FlashEndurance[j].Bytemark>>8;	

	if(flash_type.isMLC==0)
		DATABuffer[0x80+addptr]=0;	/*使用MLC标志*/
	else
		DATABuffer[0x80+addptr]=1;

	DATABuffer[0x82+addptr]=EnableTwoplane;		//使用two plane标志
	DATABuffer[0x84+addptr]=1;		//使用SATA接口标志
	DATABuffer[0x86+addptr]=ECCThreshold;	//ECC Alarm Threshold
	DATABuffer[0x88+addptr]=flash_type.ID[0];		//Flash vender
	DATABuffer[0x8A+addptr]=Totalbadblock[device];		//原始坏块总数
	DATABuffer[0x8B+addptr]=Totalbadblock[device]>>8;
	DATABuffer[0x8C+addptr]=FunCtlWord;
	DATABuffer[0x8D+addptr]=FunCtlWord>>8;
	
	double n;
	int pageoffset;
	n=log((double)flash_type.pages_per_block)/log(2.0);
	pageoffset=n;
	if(n-pageoffset>0)
		pageoffset++;
	DATABuffer[144+addptr]=pageoffset;  //每个block含有的page数偏移

	if(flash_type.sectors_per_page==16)
		DATABuffer[146+addptr]=4;
	else if(flash_type.sectors_per_page==8)
		DATABuffer[146+addptr]=3;//page含有的sector的个数偏移，3, 表示4K page
	
	DATABuffer[148+addptr]=12;		//每个DIE下block的个数偏移, 12，表示4096 block每个DIE


	int pudie;

	DATABuffer[152+addptr]=flash_id[0][2]&0x01;			//每个CE下DIE个数偏移，1，表示每个CE下有两个DIE(以上两个值相等，DIE interleave 置1 否则置0)
	
//	if(DATABuffer[150+addptr]==DATABuffer[152+addptr])
//		DATABuffer[134+addptr]=1;	//DIE interleave标志
//	else
//		DATABuffer[134+addptr]=0;
								
	DATABuffer[154+addptr]=(unsigned char)(grdie/pow(2.0,(double)DATABuffer[152+addptr]))>>1;	//每个Channel下CE个数偏移，1，表示每个Channel下有两个CE
	DATABuffer[156+addptr]=channel/2;	//Channel 数偏移，2，表示共有4个channel
	DATABuffer[158+addptr]=Resratio;	//保留比例，以x/128为单位, 1，表示每个PU共保留1/12块为free块，该参数要求做成可人工配置
	
	if(EnableNewrule)
		respageperpb=Respage*flash_type.pages_per_block/64;
	else
	{
		if(flash_type.ID[0]==0xEC)
			DATABuffer[160+addptr]=8;	//每个logic block保留的page数，三星为8
		else if(flash_type.ID[0]==0x89 || flash_type.ID[0]==0x2C)
			DATABuffer[160+addptr]=16;	//每个logic block保留的page数，INTEL MICRON为16
	}

	if(EnableNewrule)
	{
		
		MaxPU=(flash_type.size_page*1024-MaptableOffset)*2/(3*(flash_type.pages_per_block-respageperpb));
		if(MaxPU>=grdie*channel)
		{
			MaxPU=grdie*channel;
			Mapphyblock=(flash_type.size_page*1024-MaptableOffset)*2/(3*MaxPU*(flash_type.pages_per_block-respageperpb));
		}
		else
			Mapphyblock=1;

		DATABuffer[162+addptr]=Mapphyblock;		//每个logic block映射到的物理块的个数，新算法下用户手动输入
		DATABuffer[160+addptr]=respageperpb*Mapphyblock;
	}
	else
	{
		respageperpb=Respage*flash_type.pages_per_block/64;
		if(flash_type.pages_per_block==64)
			Mapphyblock=2;
		else
			Mapphyblock=1;

		MaxPU=(flash_type.size_page*1024-512)*2/(3*(flash_type.pages_per_block-respageperpb)*Mapphyblock);
		if(MaxPU>=16)
			MaxPU=16;
		else if(MaxPU<16 && MaxPU>=8)
			MaxPU=8;
		else if(MaxPU<8 && MaxPU>=4)
			MaxPU=4;
		else if(MaxPU<4 && MaxPU>=2)
			MaxPU=2;
		else if(MaxPU<2 && MaxPU>=1)
			MaxPU=1;
		if(MaxPU>grdie*channel)
			MaxPU=grdie*channel;
		DATABuffer[162+addptr]=Mapphyblock;		//每个logic block映射到的物理块的个数
		DATABuffer[160+addptr]=respageperpb*Mapphyblock;
	}

	DATABuffer[164+addptr]=Incphyblock;		//每个logic block下总共可以挂的物理块的个数，用户手动输入

		
	if(grdie*channel>MaxPU)
		pudie=grdie*channel/MaxPU;
	else
		pudie=1;

	//每个PU下DIE的个数偏移, 0, 表示每个PU下1个DIE
	DATABuffer[150+addptr]=(pudie>>1);		
	
	unsigned long lcapa;
	DWORD tmp;
	ULONGLONG endurblock=0;

	DATABuffer[176+addptr]=grdie*channel*flash_type.blocknum;
	DATABuffer[177+addptr]=(grdie*channel*flash_type.blocknum)>>8;	//总物理块数
	DATABuffer[178+addptr]=(grdie*channel*flash_type.blocknum)>>16;	//总物理块数
	DATABuffer[179+addptr]=(grdie*channel*flash_type.blocknum)>>24;	//总物理块数

	/*计算block总擦除次数*/
	for(i=0;i<64;i++)
	{
		if(flash_type.ID[0]==FlashEndurance[i].ID[0] && flash_type.ID[1]==FlashEndurance[i].ID[1] && flash_type.ID[2]==FlashEndurance[i].ID[2] && flash_type.ID[3]==FlashEndurance[i].ID[03]) 
		{
			endurblock=grdie*channel*4096*FlashEndurance[i].Endurance;
			break;
		}
	}

	DATABuffer[0xDC+addptr]=endurblock;
	DATABuffer[0xDD+addptr]=endurblock>>8;	
	DATABuffer[0xDE+addptr]=endurblock>>16;	
	DATABuffer[0xDF+addptr]=endurblock>>24;	

	DATABuffer[0xE0+addptr]=endurblock>>32;
	DATABuffer[0xE1+addptr]=endurblock>>40;	
	DATABuffer[0xE2+addptr]=endurblock>>48;	
	DATABuffer[0xE3+addptr]=endurblock>>56;	

	tmp=grdie*channel*flash_type.blocknum*DATABuffer[158+addptr]/128;
	DATABuffer[180+addptr]=tmp;
	DATABuffer[181+addptr]=tmp>>8;	//总保留的物理块数
	DATABuffer[182+addptr]=tmp>>16;	
	DATABuffer[183+addptr]=tmp>>24;							
	
	tmp=grdie*channel*flash_type.blocknum*(128-DATABuffer[158+addptr])/(DATABuffer[162+addptr]*128);			
	DATABuffer[184+addptr]=tmp;
	DATABuffer[185+addptr]=tmp>>8;	//总逻辑块数
	DATABuffer[186+addptr]=tmp>>16;	
	DATABuffer[187+addptr]=tmp>>24;		
	
	lcapa=tmp*(flash_type.pages_per_block*DATABuffer[162+addptr]-DATABuffer[160+addptr])*flash_type.sectors_per_page;
	DATABuffer[188+addptr]=lcapa;
	DATABuffer[189+addptr]=lcapa>>8;	
	DATABuffer[190+addptr]=lcapa>>16;	
	DATABuffer[191+addptr]=lcapa>>24;		//给用户的最大LBA

	/*写DIEMAP信息*/
	for(i=0;i<grdie;i++)
	{
		DATABuffer[addptr++] = DIEMAP[i];
		DATABuffer[addptr++] = DIEMAP[i] >> 8;
	}

	return 1;
}


BOOL SW80SetNFC (int device)
{
	BYTE buffer[1024],readbuf[512];
	BOOL ret;
	memset(buffer,0,1024);
	strcpy((char*)buffer,"WS08R_MAD_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=SLW_CMD_VND_SUB_NFC;
	buffer[20]=SW80_CMD_VND;

	SW80WriteFWInfo(device,buffer,512);

	ret=WriteSectors(device,0xF800000,2,buffer);
	if(ret==0)
		return ret;

	ret=ReadSectors(device,0xF800000,1,readbuf);
	if(ret==0)
		return ret;

	if(readbuf[0]=='F'&&readbuf[1]=='I')
	{
		return 1;
	}
	else
		return 0;
}

BOOL SW80UpdateLoad(int device,BYTE* DATABuffer)
{
	CString strDlgFile;
	PUCHAR readbuf;

	int datasize = 64*512,i;
	DWORD oobnum=0,tmpnum=0;
	BYTE oob[24],pagebuf[512*32];
	CString str;

	memset(oob,0,24);
	memset(pagebuf,0,512*32);
	if(SW80SetBCH(device,1)==0)
	{
		return 0;
	}
	if(SW80Eraseblock(device,0,0,0)==0)
	{
		return 0;
	}

	long addr;
	addr=512*63;

	SW80WriteFWInfo(device,DATABuffer,addr);/*写入FW 运行参数到和loader中(32K的最后一个sector)*/
	
	for(i=0;i<8*1024;i++)
	{
		tmpnum=DATABuffer[4*i]+(DATABuffer[4*i+1]<<8)+(DATABuffer[4*i+2]<<16)+(DATABuffer[4*i+3]<<24);
		oobnum=oobnum+tmpnum;
	}
	

	for(i=0;i<32;i++)
	{
		oob[0]=oobnum;
		oob[1]=oobnum>>8;
		oob[2]=oobnum>>16;
		oob[3]=oobnum>>24;
		memcpy(pagebuf,DATABuffer+i*1024,1024);
		if(SW80PageWrite(device, 0, 0, i,oob,pagebuf)==0)
		{
			return 0;
		}
	}

	readbuf=(PUCHAR)malloc(datasize);
	memset(readbuf, 0, datasize);
	for(i=0;i<32;i++)
	{
		if(SW80PageRead(device, 0, 0, i,0,pagebuf)==0)
		{
			return 0;
		}
		memcpy(readbuf+i*1024,pagebuf+512,1024);
	}
	

	for(i=0;i<64*512;i++)
	{
		if(readbuf[i]!=DATABuffer[i])
		{
			str.Format(" %x w: %x  r:%x",i,DATABuffer[i],readbuf[i]);
			break;
		}
	}
	free(readbuf);



	if(SW80SetBCH(device,0)==0)
	{
		return 0;
	}
	return 1;
}


// 对磁盘扇区数据的写入
BOOL WriteData(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff) 
{
	LARGE_INTEGER li;
	BOOL bRet;
	li.QuadPart = dwStartSector*512;
	SetFilePointer(Devicehandle[device], li.LowPart, &li.HighPart, FILE_BEGIN);
	DWORD dwCB;


	bRet = WriteFile(Devicehandle[device], lpSectBuff, 512 * wSectors, &dwCB, NULL);

	return bRet;
}

BOOL ReadData(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff)
// 对磁盘扇区数据的读取
{
	LARGE_INTEGER li;
	BOOL bRet;
	li.QuadPart = dwStartSector*512;
	SetFilePointer(Devicehandle[device], li.LowPart, &li.HighPart, FILE_BEGIN);
	DWORD dwCB;
	

	bRet = ReadFile(Devicehandle[device], lpSectBuff, 512 * wSectors, &dwCB, NULL);

	return bRet;
}



// 对磁盘扇区数据的写入
BOOL WriteSectors(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff) 
{
	LARGE_INTEGER li;
	BOOL bRet;
	li.QuadPart = dwStartSector*512;
	SetFilePointer(Devicehandle[device], li.LowPart, &li.HighPart, FILE_BEGIN);
	DWORD dwCB;

	if(	InterfaceType[device]==1)
	{
		bRet = ata_via_scsi_write(device, 0, dwStartSector, wSectors, lpSectBuff);
	}
	else
		bRet = WriteFile(Devicehandle[device], lpSectBuff, 512 * wSectors, &dwCB, NULL);

	return bRet;
}

BOOL ReadSectors(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff)
// 对磁盘扇区数据的读取
{
	LARGE_INTEGER li;
	BOOL bRet;
	li.QuadPart = dwStartSector*512;
	SetFilePointer(Devicehandle[device], li.LowPart, &li.HighPart, FILE_BEGIN);
	DWORD dwCB;
	
	if(	InterfaceType[device]==1)
	{
		bRet = ata_via_scsi_read(device, 0, dwStartSector, wSectors, lpSectBuff);
	}
	else
		bRet = ReadFile(Devicehandle[device], lpSectBuff, 512 * wSectors, &dwCB, NULL);

	return bRet;
}

BOOL SW80SetBCH(int device,int BCHmode)
/*
BCHmode 1：BCH16，  0：BCH8
1)	BCH8, 13bit ECC + 24Byte OOB， 4K page + 128 spare: 适用SW80三星flash；SW80运行默认值。
2)	BCH16, 26bit ECC：适用于更新SW80 loader, GUC boot loader默认模式加载FW到sram。
*/
{
	BYTE buffer[512],readbuf[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08R_MAD_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=SW80_CMD_VND_SUB_SETBCH;
	buffer[16]=BCHmode;
	buffer[20]=SW80_CMD_VND;
	ret=WriteSectors(device,0xF800000,1,buffer);
	if(ret==0)
		return ret;
	Sleep(50);
	ret=ReadSectors(device,0xF800000,1,readbuf);
	if(ret==0)
		return ret;

	if(readbuf[0]=='F'&&readbuf[1]=='I')
		return 1;
	else
		return 0;
}

BOOL SW80PageRead(int device,int channel,int die,DWORD page,BOOL bypassflag,BYTE *databuf)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08R_MAD_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=SW80_CMD_VND_SUB_READPAGE;
	buffer[16]=die;
	buffer[17]=channel;
	buffer[18]=page;
	buffer[19]=page>>8;
	buffer[20]=SW80_CMD_VND;
	buffer[21]=page>>16;
	buffer[22]=page>>24;
	buffer[23]=bypassflag;

	ret=WriteSectors(device,0xF800000,1,buffer);
	if(ret==0)
		return ret;

	ret=ReadSectors(device,0xF800000,9,databuf);
	if(ret==0)
		return ret;

	if(databuf[0]=='F'&&databuf[1]=='I')
	{
		if(bypassflag)
			return 1;
		else
		//	ret=databuf[14];
			ret=0;
	}
	else
		return 0;

	return !ret;
}



BOOL SW80PageWrite(int device,int channel,int die,DWORD page,BYTE *oob,BYTE *databuf)
{
	BYTE buffer[512*9],readbuf[512];
	BOOL ret;
	memset(buffer,0,512*9);
	memset(readbuf,0,512);
	int i;
	strcpy((char*)buffer,"WS08R_MAD_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=SW80_CMD_VND_SUB_WRITEPAGE;
	buffer[16]=die;
	buffer[17]=channel;
	buffer[18]=page;
	buffer[19]=page>>8;
	buffer[20]=SW80_CMD_VND;
	buffer[21]=page>>16;
	buffer[22]=page>>24;
	for(i=0;i<24;i++)
		buffer[23+i]=oob[i];

	memcpy(buffer+512,databuf,8*512);
	//写命令包和数据包
	ret=WriteSectors(device,0xF800000,9,buffer);
	if(ret==0)
		return ret;
//	Sleep(50);

	ret=ReadSectors(device,0xF800000,1,readbuf);
	if(ret==0)
		return ret;

	if(readbuf[0]=='F'&&readbuf[1]=='I')
		ret=(readbuf[14] & 0x1);
	else
		return 0;

	return !ret;
}

BOOL EnterVND(int device) 
{
	unsigned char result;
	ULONGLONG vendorlba;

	vendorlba=SW80GetVendorLBA(device);
	if(SW80ReadCMDStatus(device,vendorlba,&result)>0)
	{
		if(SW80SSDVenderMode(device,vendorlba,1)==0)
		{
			return 0;
		}
	}
	else
	{
		if(SW80VenderMode(device,1)==0)	
		{
			return 0;
		}
	}
	return 1;

}

BOOL SW80ReadCMDStatus(int device,ULONGLONG vendorlba,BYTE *result)
{
	BYTE buffer[512],readbuf[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=0xA5;
	ret=WriteSectors(device,vendorlba,1,buffer);
	if(ret==0)
		return ret;
	Sleep(50);
	ret=ReadSectors(device,vendorlba,1,readbuf);
	if(ret==0)
		return ret;

	if(readbuf[1]=='F'&&readbuf[2]=='I')
		*result=readbuf[1];
	else
		return 0;

	return 1;
}

BOOL SW80SSDVenderMode(int device,ULONGLONG vendorlba,BOOL venderflag)
{
	BYTE buffer[512];
	BOOL ret;

	/*LBA值不对，直接退出*/
	if(vendorlba==0 || vendorlba==0x800000)
		return 0;

	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	if(venderflag)
		buffer[15]=SW80_CMD_VND_SUB_ENDBG;
	else
		buffer[15]=SW80_CMD_VND_SUB_OUTBG;

	ret=WriteSectors(device,vendorlba,1,buffer);
	return ret;
}

BOOL SW80VenderMode(int device,BOOL venderflag)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"GUCSSDTOOLING.");

	buffer[14]=SW80_CMD_FEATURE;
	if(venderflag)
		buffer[15]=SW80_CMD_VND_SUB_ENDBG;
	else
		buffer[15]=SW80_CMD_VND_SUB_OUTBG;

	buffer[16]=0x55;
	buffer[17]=0xAA;
	buffer[18]=0x55;
	buffer[19]=0;
	buffer[20]=SW80_CMD_VND;
	ret=WriteSectors(device,0,1,buffer);
	return ret;
}

BOOL DoIdentifyDeviceSat(int device, BYTE target, BYTE *data)
{
	BOOL	bRet;

	DWORD	dwReturned;
	DWORD	length;

	SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;

	if(data == NULL)
	{
		return	FALSE;
	}

	::ZeroMemory(data, 512);

	
	if(Devicehandle[device] == INVALID_HANDLE_VALUE)
	{
		return	FALSE;
	}

	::ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

    sptwb.Spt.Length = sizeof(SCSI_PASS_THROUGH);
    sptwb.Spt.PathId = 0;
    sptwb.Spt.TargetId = 0;
    sptwb.Spt.Lun = 0;
    sptwb.Spt.SenseInfoLength = 32;
    sptwb.Spt.DataIn = SCSI_IOCTL_DATA_IN;
    sptwb.Spt.DataTransferLength = 512;
    sptwb.Spt.TimeOutValue = 2;
    sptwb.Spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, DataBuf);
    sptwb.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, SenseBuf);


	sptwb.Spt.CdbLength = 12;
	sptwb.Spt.Cdb[0] = 0xA1;//ATA PASS THROUGH(12) OPERATION CODE(A1h)
	sptwb.Spt.Cdb[1] = (4 << 1) | 0; //MULTIPLE_COUNT=0,PROTOCOL=4(PIO Data-In),Reserved
	sptwb.Spt.Cdb[2] = (1 << 3) | (1 << 2) | 2;//OFF_LINE=0,CK_COND=0,Reserved=0,T_DIR=1(ToDevice),BYTE_BLOCK=1,T_LENGTH=2
	sptwb.Spt.Cdb[3] = 0;//FEATURES (7:0)
	sptwb.Spt.Cdb[4] = 1;//SECTOR_COUNT (7:0)
	sptwb.Spt.Cdb[5] = 0;//LBA_LOW (7:0)
	sptwb.Spt.Cdb[6] = 0;//LBA_MID (7:0)
	sptwb.Spt.Cdb[7] = 0;//LBA_HIGH (7:0)
	sptwb.Spt.Cdb[8] = target;
	sptwb.Spt.Cdb[9] = 0xEC;//COMMAND

	length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, DataBuf) + sptwb.Spt.DataTransferLength;

	bRet = ::DeviceIoControl(Devicehandle[device], IOCTL_SCSI_PASS_THROUGH, 
		&sptwb, sizeof(SCSI_PASS_THROUGH),
		&sptwb, length,	&dwReturned, NULL);

	
	if(bRet == FALSE || dwReturned != length)
	{
		return	FALSE;
	}

	memcpy(data, sptwb.DataBuf, 512);

	return	TRUE;
}


BOOL ata_via_scsi_write(int device, BYTE target,LONGLONG dwStartSector, WORD wSectors, BYTE *data)
{
	BOOL	bRet;
	int i;
	DWORD	dwReturned;
	DWORD	length,errorcode;
	BYTE	buf[512];
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptwb;

	if(data == NULL)
	{
		return	FALSE;
	}


	if(Devicehandle[device] == INVALID_HANDLE_VALUE)
	{
		return	FALSE;
	}
	

	::ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));

	sptwb.Spt.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptwb.Spt.PathId = 0;
	sptwb.Spt.TargetId = 0;
	sptwb.Spt.Lun = 0;
	sptwb.Spt.SenseInfoLength = 32;
	sptwb.Spt.DataIn = SCSI_IOCTL_DATA_OUT;  //%%%%%%%%%%%%%%%%%%read 1 write 0
	sptwb.Spt.DataTransferLength = 512*wSectors;
	sptwb.Spt.DataBuffer = data;
	sptwb.Spt.TimeOutValue = 2;
	sptwb.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);

	sptwb.Spt.CdbLength = 12;
	sptwb.Spt.Cdb[0] = 0xA1;
	sptwb.Spt.Cdb[1] = (0x5 << 1) | 0; //MULTIPLE_COUNT=0,PIO OUT=5,Reserved
//	sptwb.Spt.Cdb[1] = (0x6 << 1) | 0; //MULTIPLE_COUNT=0,PIO OUT=5,Reserved
	sptwb.Spt.Cdb[2] = (0 << 3) | (1 << 2) | 2;//OFF_LINE=0,CK_COND=0,Reserved=0,T_DIR=1(ToDevice),BYTE_BLOCK=1,T_LENGTH=2
	sptwb.Spt.Cdb[3] = 0;//FEATURES (7:0)
	sptwb.Spt.Cdb[4] = wSectors;//SECTOR_COUNT (7:0)
	sptwb.Spt.Cdb[5] = dwStartSector;//LBA_LOW (7:0)
	sptwb.Spt.Cdb[6] = dwStartSector>>8;//LBA_MID (7:0)
	sptwb.Spt.Cdb[7] = dwStartSector>>16;//LBA_HIGH (7:0)
//	sptwb.Spt.Cdb[8] = 0xB0 |(dwStartSector>>24);//LBA_HIGH (7:0)
	sptwb.Spt.Cdb[8] =0xA0 | (dwStartSector>>24);//LBA_HIGH (7:0)

	sptwb.Spt.Cdb[9] = 0x30;//COMMAND
//	sptwb.Spt.Cdb[9] = 0xCA;//COMMAND write dma
//	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER)+sptwb.Spt.DataTransferLength;
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);

	bRet = ::DeviceIoControl(Devicehandle[device], IOCTL_SCSI_PASS_THROUGH_DIRECT, 
		&sptwb, length,
		&sptwb, length,	&dwReturned, FALSE);
	
	if(bRet == FALSE)
	{
		errorcode=GetLastError();
		return	FALSE;
	}
	return	TRUE;
}


BOOL ata_via_scsi_read(int device, BYTE target,LONGLONG dwStartSector, WORD wSectors, BYTE *data)
{
	BOOL	bRet;
	int i;
	DWORD	dwReturned;
	DWORD	length,errorcode;
	BYTE	buf[512];

	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptwb;

	if(data == NULL)
	{
		return	FALSE;
	}


	if(Devicehandle[device] == INVALID_HANDLE_VALUE)
	{
		return	FALSE;
	}

	::ZeroMemory(data, wSectors*512);
	::ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));

	sptwb.Spt.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptwb.Spt.PathId = 0;
	sptwb.Spt.TargetId = 0;
	sptwb.Spt.Lun = 0;
	sptwb.Spt.SenseInfoLength = 32;
	sptwb.Spt.DataIn = SCSI_IOCTL_DATA_IN;
  	sptwb.Spt.DataTransferLength = wSectors*512;
	sptwb.Spt.DataBuffer = data;
	sptwb.Spt.TimeOutValue = 2;
	sptwb.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);

	sptwb.Spt.CdbLength = 12;
	sptwb.Spt.Cdb[0] = 0xA1;//ATA PASS THROUGH(12) OPERATION CODE(A1h)
	sptwb.Spt.Cdb[1] = (4 << 1) | 0; //MULTIPLE_COUNT=0,PROTOCOL=4(PIO Data-In),Reserved
//	sptwb.Spt.Cdb[1] = (0x6 << 1) | 0; //MULTIPLE_COUNT=0,PIO OUT=5,Reserved
	sptwb.Spt.Cdb[2] = (1 << 3) | (1 << 2) | 2;//OFF_LINE=0,CK_COND=0,Reserved=0,T_DIR=1(ToDevice),BYTE_BLOCK=1,T_LENGTH=2
	sptwb.Spt.Cdb[3] = 0;//FEATURES (7:0)
	sptwb.Spt.Cdb[4] = wSectors;//SECTOR_COUNT (7:0)
	sptwb.Spt.Cdb[5] = dwStartSector;//LBA_LOW (7:0)
	sptwb.Spt.Cdb[6] = dwStartSector>>8;//LBA_MID (7:0)
	sptwb.Spt.Cdb[7] = dwStartSector>>16;//LBA_HIGH (7:0)
	sptwb.Spt.Cdb[8] = dwStartSector>>24;//LBA_HIGH (7:0)

	sptwb.Spt.Cdb[9] = 0x20;//COMMAND
//	sptwb.Spt.Cdb[9] = 0xC8;//COMMAND read dma
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	Sleep(3);
	bRet = ::DeviceIoControl(Devicehandle[device], IOCTL_SCSI_PASS_THROUGH_DIRECT, 
		&sptwb, length,
		&sptwb, length,	&dwReturned, NULL);
	
	if(bRet == FALSE)
	{
		errorcode=GetLastError();
		return	FALSE;
	}

	return	TRUE;
}



BOOL SW80ReadGlobalinfoblockTxt(CString File)
{
	FILE *fp;
	int i,num;
	char output[128];
	CString str;

	CHAR FilePath[255]; 

	GetModuleFileName(NULL,FilePath,255); 
    (strrchr(FilePath,'\\'))[1] = 0; 

    strcat(FilePath,File);

	fp = fopen(FilePath, "rb");
	if(fp==NULL)
	{
		AfxMessageBox("Open read file fail!");
		AfxMessageBox(FilePath);
		return 0;
	}
	
	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&num);
	for(i=0;i<num;i++)
	{
		fscanf(fp,"%X,",&IDFY32G[i]);	
	}

	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&ConfNum);
	for(i=0;i<num;i++)
	{
		fscanf(fp,"%X,",&FlashEndurance[i].ID[0]);	
		fscanf(fp,"%X,",&FlashEndurance[i].ID[1]);	
		fscanf(fp,"%X,",&FlashEndurance[i].ID[2]);	
		fscanf(fp,"%X,",&FlashEndurance[i].ID[3]);	
		fscanf(fp,"%X,",&FlashEndurance[i].Endurance);
		fscanf(fp,"%X,",&FlashEndurance[i].pagesize);
		fscanf(fp,"%X,",&FlashEndurance[i].blocksize);
		fscanf(fp,"%X,",&FlashEndurance[i].blocknum);
		fscanf(fp,"%X,",&FlashEndurance[i].PllClock);	
		fscanf(fp,"%X,",&FlashEndurance[i].NFCtimeregister1);	
		fscanf(fp,"%X,",&FlashEndurance[i].NFCtimeregister2);	
		fscanf(fp,"%X,",&FlashEndurance[i].NFCtimeregister3);
		fscanf(fp,"%X,",&FlashEndurance[i].NFCBCH);
		fscanf(fp,"%X,",&FlashEndurance[i].Sparesize);
		fscanf(fp,"%X,",&FlashEndurance[i].Pagemark0);
		fscanf(fp,"%X,",&FlashEndurance[i].Pagemark1);
		fscanf(fp,"%X,",&FlashEndurance[i].Bytemark);
		fscanf(fp,"%X,",&FlashEndurance[i].Flashtype);
		fscanf(fp,"%X,",&FlashEndurance[i].Planeperdie);
	}


	fclose(fp);
	return 1;
}


int SW80GetDIEnumPerChip()
{
	int i,ldie;
	int dieperchip=0;
	int diepos[8];//记录每个chip中的8个die是否为空
	int tmp;

	memset(diepos,0,8*sizeof(int));
	for(i=0;i<64;i++)
	{
		if(flash_id[i][0]==0xEC ||flash_id[i][0]==0x89 || flash_id[i][0] == 0x2C ||flash_id[i][0]==0x98)
		{
			tmp=i%16;
			if(tmp>=0 && tmp<2)
				diepos[0]=1;
			if(tmp>=2 && tmp<4)
				diepos[1]=1;

			if(tmp>=4 && tmp<6)
				diepos[2]=1;
			if(tmp>=6 && tmp<8)
				diepos[3]=1;

			if(tmp>=8 && tmp<10)
				diepos[4]=1;
			if(tmp>=10 && tmp<12)
				diepos[5]=1;

			if(tmp>=12 && tmp<14)
				diepos[6]=1;
			if(tmp>=14 && tmp<16)
				diepos[7]=1;
		}
	}
			
	for(i=0;i<8;i++)
	{
		if(diepos[i])
			dieperchip++;
	}
	return dieperchip;
}



BOOL SW80ReadConfigureIni(CString FilePath)
{
	char str[128];
	char filename[128];
	COPini ini;

	CString mdesconf;
	CString sn;
	int i;

    GetModuleFileName(NULL,filename,128); 
    //Scan a string for the last occurrence of a character.
    (strrchr(filename,'\\'))[1] = 0; 
    strcat(filename,FilePath);	
	
#if _INITD
	if(sn.IsEmpty()==0)
	{
		for(i=0;i<DeviceCount;i++)
			BadBlockFilename[i] = sn;
	}
#endif	


	ini.ReadString("FW Setting","Reserve Ratio",str,filename);
	Resratio=atoi(str);
	ini.ReadString("FW Setting","Reserve Page Per 64 Pages",str,filename);
	Respage=atoi(str);
	ini.ReadString("FW Setting","Function Control Word",str,filename);
	FunCtlWord=atoi(str);
	ini.ReadString("FW Setting","Logic Block Include Phy Block",str,filename);
	Incphyblock=atoi(str);

	ini.ReadString("FW Setting","Enable Two Plane",str,filename);
	EnableTwoplane=atoi(str);
	ini.ReadString("FW Setting","Enable New Rule",str,filename);
	EnableNewrule=atoi(str);
	ini.ReadString("FW Setting","Max PU Num",str,filename);
	MaxPU=atoi(str);
	ini.ReadString("FW Setting","Maptable Offset",str,filename);
	MaptableOffset=atoi(str);
	
	ini.ReadString("FW Setting","ECC Alarm Threshold",str,filename);
	ECCThreshold=atol(str);

	return 1;
}

BOOL ConvertData(char *date)
{
	char *token;
	char sep=' ';
	char c[3][32],tmp[8];
	int i,j,day;

	memset(tmp,0,8);
	for(i=0;i<3;i++)
		for(j=0;j<32;j++)
			c[i][j]=0;

	i=0;
	token = strtok( date, &sep);
	while( token != NULL )
	{
	  /* While there are tokens in "string" */
	  printf( " %s\n", token );
	  strcpy(c[i],token);
	  /* Get next token: */
	  token = strtok( NULL, &sep );
	  i++;
	}
	strcpy(date,c[2]);
	if(memcmp(c[0],"Jan",3)==0)
		strcpy(tmp,"01");
	if(memcmp(c[0],"Feb",3)==0)
		strcpy(tmp,"02");
	if(memcmp(c[0],"Mar",3)==0)
		strcpy(tmp,"03");
	if(memcmp(c[0],"Apr",3)==0 )
		strcpy(tmp,"04");
	if(memcmp(c[0],"May",3)==0)
		strcpy(tmp,"05");
	if(memcmp(c[0],"Jun",3)==0)
		strcpy(tmp,"06");
	if(memcmp(c[0],"Jul",3)==0)
		strcpy(tmp,"07");
	if(memcmp(c[0],"Aug",3)==0)
		strcpy(tmp,"08");
	if(memcmp(c[0],"Sept",4)==0)
		strcpy(tmp,"09");
	if(memcmp(c[0],"Oct",3)==0)
		strcpy(tmp,"10");
	if(memcmp(c[0],"Nov",3)==0)
		strcpy(tmp,"11");
	if(memcmp(c[0],"Dec",3)==0)
		strcpy(tmp,"12");
	strcat(date,tmp);
	day=atoi(c[1]);
	sprintf(tmp,"%02d",day);
	strcat(date,tmp);
	return 1;
}



BOOL VNDFW2ATA48b(int device,BYTE* buf)
{
	IDEREGS regs,regspre;
	CString str;

	int i;
	BYTE Data[4096];
	memset(Data,0,4096);
	memset(&regs,0,sizeof(IDEREGS));
	memset(&regspre,0,sizeof(IDEREGS));

	for(i=0;i<64;i++)
	{
		regs.bFeaturesReg		= 0x31;	// send  command
		regs.bSectorCountReg	= 8;
		regs.bSectorNumberReg	= 0 ;					
		regs.bCylLowReg	= 8;
		regs.bCylHighReg = 0x80;
		regs.bDriveHeadReg = 0xE0;		
		regs.bCommandReg = 0xFE;

		regspre.bFeaturesReg		= 0;	// send  command
		regspre.bSectorCountReg	= 0;
		regspre.bSectorNumberReg	= i ;					
		regspre.bCylLowReg	= 0;
		regspre.bCylHighReg = 0;
		regspre.bDriveHeadReg = 0;		
		regspre.bCommandReg = 0;

		if( !ata_pass_through_ioctl_pio48b(device, &regs, &regspre,(BYTE *)&buf[i * 4096], 4096, false) )
		{
			str.Format("读出FW到ATA command rejected !    %d",i);
			AddInfo(str);
		
	//		CloseHandle(Devicehandle[device]);
	//		Devicehandle[device]=INVALID_HANDLE_VALUE;
	//		return FALSE;
		}
#if _INITD 
		pGuiServerExplorer->m_progress.SetPos(100*i/128);
#endif
	}
	AddInfo("*读出FW到ATA成功*");
	return 1;
}

BOOL VNDDriveRSPI48b(int device,int pageoff,BYTE* buf,int pagenum)
{
	IDEREGS regs,regspre;
	CString str;

	int i;
	BYTE Data[4096];
	memset(Data,0,4096);

	for(i=0;i<pagenum;i++)
	{
		regs.bFeaturesReg		= 0x30;	// send  command
		regs.bSectorCountReg	= 8;	
		regs.bSectorNumberReg	= 0 ;					
		regs.bCylLowReg	= 8;
		regs.bCylHighReg = 0x80;
		regs.bDriveHeadReg = 0xE0;		
		regs.bCommandReg = 0xFE;

		regspre.bFeaturesReg		= 0;	// send  command
		regspre.bSectorCountReg	= 0;
		regspre.bSectorNumberReg	= i+pageoff ;					
		regspre.bCylLowReg	= 0;
		regspre.bCylHighReg = 0;
		regspre.bDriveHeadReg = 0;		
		regspre.bCommandReg = 0;

		if( !ata_pass_through_ioctl_pio48b(device, &regs, &regspre,(BYTE *)&buf[i * 4096], 4096, false) )
		{
			AddInfo("从SPI 读出数据到主机 command rejected !");
			CloseHandle(Devicehandle[device]);
			Devicehandle[device]=INVALID_HANDLE_VALUE;
			return FALSE;
		}
#if _INITD 
		pGuiServerExplorer->m_progress.SetPos(100*i/208);
#endif
	}
	AddInfo("*从SPI 读出数据到主机 成功*");
	return 1;
}

BOOL ata_pass_through_ioctl_pio48b(int device, IDEREGS * regs,IDEREGS * regspre, unsigned char * data, unsigned long datasize, bool IsWR)
{ 
	typedef struct 
	{
		ATA_PASS_THROUGH_EX apt;
		UCHAR ucDataBuf[64*1024];
	} ATA_PASS_THROUGH_EX_WITH_BUFFERS;
	
	CString str;

	if(InterfaceType[device]==1)
	{
		int ret;
		ret=ata_via_scsi_pio48b(Devicehandle[device],regs,regspre,data,datasize,IsWR);
		
	
		if (regs->bCommandReg != 0x50) 
		{
			str.Format("ATA Command ERROR: 0x%02X", regs->bCommandReg);
			return FALSE;
		}

		return ret;
	}
}


BOOL ata_via_scsi_pio48b(HANDLE hdevice, IDEREGS * regs, IDEREGS * regspre, unsigned char * data, unsigned long datasize, bool IsWR)
{
	BOOL	bRet;
	DWORD	dwReturned;
	DWORD	length,errorcode;
	int tlength=0;
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptwb;

	::ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));

	if(hdevice == INVALID_HANDLE_VALUE)
	{
		return	FALSE;
	}
	
	
	if (datasize > (64 * 1024)) 
	{
		return FALSE;
	}

	if(datasize > 0)
	{
		tlength=2;
		if( ! IsWR)
		{
			sptwb.Spt.DataIn = SCSI_IOCTL_DATA_IN ;
		}
		else
		{
			sptwb.Spt.DataIn = SCSI_IOCTL_DATA_OUT;
		}

	}
	else
	{
		sptwb.Spt.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
		tlength=0;
	}

	sptwb.Spt.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptwb.Spt.PathId = 0;
	sptwb.Spt.TargetId = 0;
	sptwb.Spt.Lun = 0;
	sptwb.Spt.SenseInfoLength = 32;
	sptwb.Spt.DataTransferLength = datasize;
	sptwb.Spt.TimeOutValue = 1;
	sptwb.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);
	sptwb.Spt.DataBuffer = data;
	
	sptwb.Spt.CdbLength = 16;
	sptwb.Spt.Cdb[0] = 0x85;
	if(datasize==0)
		sptwb.Spt.Cdb[1] = (0x3 << 1) | 0; //MULTIPLE_COUNT=0,Non-data=3,Reserved
	else
	{
		if(IsWR)
			sptwb.Spt.Cdb[1] = (0x5 << 1) | 0; //MULTIPLE_COUNT=0,PIO OUT=5,Reserved
		else
			sptwb.Spt.Cdb[1] = (0x4 << 1) | 0; //MULTIPLE_COUNT=0,PIO IN=4,Reserved
	}

	if(IsWR)
		sptwb.Spt.Cdb[2] = (1 << 5) | (0 << 3) | (1 << 2) | tlength;//OFF_LINE=0,CK_COND=1,Reserved=0,T_DIR=0(FromDevice),BYTE_BLOCK=1,T_LENGTH=2
	else
		sptwb.Spt.Cdb[2] = (1 << 5) | (1 << 3) | (1 << 2) | tlength;//OFF_LINE=0,CK_COND=1,Reserved=0,T_DIR=1(ToDevice),BYTE_BLOCK=1,T_LENGTH=2
	
	sptwb.Spt.Cdb[4] = regs->bFeaturesReg;//FEATURES (7:0)
	sptwb.Spt.Cdb[6] = regs->bSectorCountReg;//SECTOR_COUNT (7:0)
	sptwb.Spt.Cdb[8] = regs->bSectorNumberReg;//LBA_LOW (7:0)
	sptwb.Spt.Cdb[10] = regs->bCylLowReg;//LBA_MID (7:0)
	sptwb.Spt.Cdb[12] = regs->bCylHighReg;//LBA_HIGH (7:0)
	sptwb.Spt.Cdb[13] =(0x4<<4)  | regs->bDriveHeadReg;//DriveHeadReg (4:0)
	sptwb.Spt.Cdb[14] = regs->bCommandReg;//COMMAND

	sptwb.Spt.Cdb[3] = regspre->bFeaturesReg;//FEATURES 
	sptwb.Spt.Cdb[5] = regspre->bSectorCountReg;//SECTOR_COUNT 
	sptwb.Spt.Cdb[7] = regspre->bSectorNumberReg;//LBA_LOW 
	sptwb.Spt.Cdb[9] = regspre->bCylLowReg;//LBA_MID 
	sptwb.Spt.Cdb[11] = regspre->bCylHighReg;//LBA_HIGH 


	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);

	bRet = ::DeviceIoControl(hdevice, IOCTL_SCSI_PASS_THROUGH_DIRECT, 
		&sptwb, length,
		&sptwb, length,	&dwReturned, FALSE);
	
	if(bRet == FALSE)
	{
		errorcode=GetLastError();
		AddInfo("Write error code 121");
//		return	FALSE;
	}

	regs->bCommandReg=sptwb.ucSenseBuf[21];
	regs->bDriveHeadReg=sptwb.ucSenseBuf[20];
    regs->bCylHighReg=sptwb.ucSenseBuf[19];
	regs->bCylLowReg=sptwb.ucSenseBuf[17];
	regs->bSectorNumberReg=sptwb.ucSenseBuf[15];
	regs->bSectorCountReg=sptwb.ucSenseBuf[13];
	regs->bFeaturesReg=sptwb.ucSenseBuf[11];
	return	TRUE;
}
