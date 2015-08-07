#ifndef _MYIDE_H_
#define _MYIDE_H_

#if defined(_WIN64)
 typedef unsigned __int64 ULONG_PTR;
#else
 typedef unsigned long ULONG_PTR;
#endif
typedef unsigned long DWORD_PTR; 

#include <stdio.h> 
#include <stdlib.h> 
#include <stddef.h>
#include <windows.h>  
#include <initguid.h>   // Guid definition
#include <devguid.h>    // Device guids
#include <setupapi.h>   // for SetupDiXxx functions.
#include <cfgmgr32.h>   // for SetupDiXxx functions.
#include <devioctl.h>  
#include <ntdddisk.h>
#include <ntddscsi.h>

#if _INITD 
#include "guisolexplorer.h"
#include "guiserverexplorer.h"
#include "FormMenoa.h"
#include "mainfrm.h"
#include "FormIniconf.h"
#include "NVDlg.h"
#endif

#if _MASSINIT 

#endif

#define  InitialCount 16		//初始化重试次数
#define  DeviceCount 6
#define	 DISK_FLAG_BYTE1		O	// "O" SLW PCBT_FW SSD FLAG
#define  DISK_FLAG_BYTE2		S	// "S"
#define  GLOBALINFOFILE          "globalinfoblock.txt";
#define  SLWCONFINI          "SLW_Configure.ini";
#define  SWMPTOOLVER		  "Soliware  MPTool Version : 2010 7 9\r\n";	


typedef  unsigned char INT8U;
typedef  unsigned short INT16U;
typedef  unsigned int INT32U;
typedef unsigned short HAND_STATUS;
typedef unsigned short ATA_CMD;

typedef   struct   
{
	int ctlport;	//对应cypress的控制口
	int sataport;	//测试板上的sata端口，范围0-5
	CString DevSn;	//系统列举设备的设备识别号
	CString ID;		//SSD的ID
}   Device_Map,*pDevice_Map;

typedef   struct   
{
	int device;	
}   TEST_STRU,*pTEST_STRU;


typedef struct _flash_Features
{
	BYTE ID[4];
	INT32U Endurance;
	INT16U pagesize;
	INT16U blocksize;
	INT16U blockperdie;	/*每个DIE的block数目*/
	INT16U PageMark0;
	INT16U PageMark1;
	INT16U ByteMark0;
	INT16U ByteMark1;
	INT16U Flashtype;
	INT16U Planeperdie;
}FLASHFeatures;

typedef struct _flash_endurance
{
	BYTE ID[4];
	INT32U Endurance;
	INT16U pagesize;
	INT16U blocksize;
	INT16U blocknum;	/*每个DIE的block数目*/
	INT32U PllClock;
	INT32U NFCtimeregister1;
	INT32U NFCtimeregister2;
	INT32U NFCtimeregister3;
	INT32U NFCBCH;
	INT16U Sparesize;
	INT16U Pagemark0;
	INT16U Pagemark1;
	INT16U Bytemark;
	INT16U Flashtype;
	INT16U Planeperdie;
}FLASHENDURANCE;

// ATA command
#define		ata_WRITE_SECTOR		0x30
#define		ata_READ_SECTOR			0x20
#define		ata_WRITE_DMA			0xCA
#define		ata_READ_DMA			0xC8
#define		ata_WRITE_DMA_EXT		0x35
#define		ata_READ_DMA_EXT		0x25
#define		ata_FLUSH_CACHE			0xE7
#define		ata_VND_CMD				0xFD

// soliware vendor command
#define SLW_ATA_CMD_VND 						0xFF	// vendor command
#define SLW_CMD_VND_SUB_ENDBG 					0xFF	// 进入或者退出vendor模式
#define	SLW_CMD_VND_SUB_RW_PARA					0xFE	// 指定读写block命令的起始sector地址， 以及sector写入数目
#define SLW_CMD_VND_SUB_DWN_FW_RUN 				0xFD	// 下载新FW ，并运行
#define SLW_CMD_VND_SUB_SETFLASHPARA			0xFC	// 设置flash参数
#define SLW_CMD_VND_SUB_READSTA 				0xFB	// read command status
#define	SLW_CMD_VND_SUB_READBLOCK				0x05    //read block
#define SLW_CMD_VND_SUB_READFLASHID 			0x00	// read flash id
//#define SLW_CMD_VND_SUB_CHKBADBLOCK				0xF9	// 连续读取指定起始block地址共128block的坏块标志
#define SLW_CMD_VND_SUB_ERASEBLOCK				0x01	// 擦除指定block
#define SLW_CMD_VND_SUB_WRITEBLOCK 				0x02	// 编程指定block
#define SLW_CMD_VND_SUB_WRITESPISECTOR			0x03	// 编程SPI FLASH指定Sector地址
#define SLW_CMD_VND_SUB_READSPISECTOR			0x04	// 读SPI FLASH指定Sector刂?#define SLW_CMD_VND_SUB_PROGLOGICBLOCK			0xF4	// 编程逻辑block
#define	SLW_CMD_VND_SUB_READPAGE				0x06	// 读取指定page数据
#define SLW_CMD_VND_SUB_WRITELOGICBLOCK			0x07    //写逻辑block
#define SLW_CMD_VND_SUB_READPAGEPAD			0x08    //读取一个page内部所有sector的pad信息
#define	SLW_CMD_VND_SUB_ERASESPIFLASHSECTOR	0x09	//擦除SPI FLASH一个指定Sector

#define SLW_VU_R_FW2DRAM				0x10     /*读出FW到DRAM*/
#define SLW_VU_R_FW2ATA				0x11     /*读出FW到ATA*/
#define SLW_VU_W_FW2DRAM				0x12    /*写入FW到DRAM*/
#define SLW_VU_W_FW2FLASH			0x13    /*写入FW到FLASH*/
#define SLW_VU_R_SPI     0x20  /*从SPI 读出数据到主机*/  
#define SLW_VU_W_SPI     0x21   /*从主机写入数据到SPI FLASH*/
#define SLW_VU_R_NV2DRAM				0x14     /*读出NV到DRAM*/
#define SLW_VU_R_GC2DRAM				0x15     /*读出GC到DRAM*/
#define SLW_VU_R_SWAP2DRAM				0x16     /*读出SWAP到DRAM*/
#define SLW_ATA_CMD_VNDNDCMD                        0xFD     /* ATA 命令自定义命令之无数据命令*/
#define SLW_ATA_CMD_VNDRDCMD                         0xFE     /* ATA 命令自定义命令之读数据命令*/



typedef struct _Flash_Para
{
	BYTE ID[4];
	BOOL isMLC;
	DWORD size_block;
	DWORD size_page;
	DWORD pages_per_block;
	DWORD sectors_per_page;
	DWORD sectors_per_block;
	DWORD total_blocks;
	DWORD cap_in_gb;
	INT16U blocknum;

}Flash_Para, *PFlash_Para;


extern Flash_Para flash_type;

extern BYTE flash_id[128][4];

typedef struct _VND_ARGS
{
	BYTE die_lay_row_ch;
	BYTE channel;
	BYTE row;
	BYTE chip;
	BYTE page;
	DWORD block;
}VND_ARGS, *PVND_ARGS;

extern VND_ARGS args;

extern int SSDInterface;
extern BOOL checkpower;
extern BOOL checkpcb;
extern BOOL checkinit;
extern BOOL checkupdatefw;
extern BOOL checkupdatefpga;
extern int Hardwarereset;

extern DWORD SSDCapacity;
extern INT32U ItemId;
extern CString m_result;
extern int GlobalDevice;
extern BOOL BReadid[DeviceCount];
extern BOOL BVND[DeviceCount];
extern int badblock[DeviceCount][128][512];	//坏块表//4k
extern int xblock[DeviceCount][128];			//坏块个数//4k
extern int page4k;		// 1: 2K		2: 4K	4: 8K
extern int blocksize;	// 1: 128K	2: 256k	4: 512k
extern int channel;
extern int plane;
extern int gldie ; //物理die,约定一个channel有32个物理die,允许为空，但物理die号保留
extern int grdie ;   //每个channel实际存在的物理die数目，当前是8，可以根据DIEMAP取
extern int gcs ; //片选数目，一般一个片选有2个die(当前使用的坏块表文件是4个channel,8个cs)
extern int step ;				//SS MC:step=2; HY step=1; plane内block地址递增是偶数还是基数
extern int datablock[DeviceCount][2][128];			//4k		最多128 die, 每个die 有2 plane
extern int startblock[DeviceCount][2][128];			//4k  	最多128 die, die 有2 plane
extern int endblock[DeviceCount][2][128];			//4k		最多128 die, die 有2 plane
extern int swapblock[DeviceCount][2][128];			//4k		最多128 die, die 有2 plane
extern int replaceblock[DeviceCount][2][128][128];	//4k		每个plane 最多128 个坏块
extern int replaceblocknum[DeviceCount][2][128];		//4k
extern int reserveblock[DeviceCount][128];			//4k 坏块保留块,最多128 die
extern int Multiblock[DeviceCount];
extern int globleblock[DeviceCount];
extern int smartblock[DeviceCount];
extern int securityblock[DeviceCount];
extern int mapblock[DeviceCount][3][4];	// 最多每个channel 一个map，4个用于map的block地址
extern int mapblockcnt[DeviceCount];	// 用于map的block 数
extern TEST_STRU IOtest[DeviceCount];
extern int DIEMAP[32];
extern int IniMode;
extern BOOL PurgeFlag;
extern int mapsize[DeviceCount];	
extern Device_Map Devicemap[DeviceCount];
extern CString BadBlockFilename[DeviceCount];
extern CString RebuildFilename;
extern CString FWFilename;
extern CString	FPGAFilename;
extern CString	LoadFilename;
extern CString manufacture[DeviceCount];
extern CString capacity[DeviceCount];
extern CString SelectBadblockfile;
extern HANDLE Devicehandle[DeviceCount];
extern HANDLE	hEventEnumSSD;
extern CString	DiskInfo;
extern DWORD MaxLBA;
extern int sectorperblock;		//block包含sector的数目，MLC 1024 SLC 512 
extern CString	m_fw[DeviceCount];
extern CString	m_descriptor[DeviceCount];
extern WORD MLCFLASH[128];
extern WORD SLCFLASH[128];
extern int SSDID;
extern BOOL FlagTime;
extern BOOL FlagStop;
extern int chargetime;
extern int InterfaceType[DeviceCount];
extern FLASHFeatures FlashFeature[16];
extern int FLASHtypeNum;
extern int diepercs;
extern WORD BCKMLCFLASH[128];
extern WORD BCKSLCFLASH[128];
#if _INITD 
extern CGuiSolExplorer *pGuiSolExplorer;
extern CGuiServerExplorer* pGuiServerExplorer;
extern CFormMenoa *pFormMenoa;
extern CMainFrame *pMainFrame;
extern CFormIniconf *pFormIniconf;
extern CNVDlg* pNVDlg;
#endif

BOOL ata_via_scsi_dma(HANDLE hdevice, IDEREGS * regs, unsigned char * data, unsigned long datasize, bool IsWR);
BOOL ata_via_scsi_pio(HANDLE hdevice, IDEREGS * regs, unsigned char * data, unsigned long datasize, bool IsWR);
BOOL ata_pass_through_ioctl_pio(int device, IDEREGS * regs, unsigned char * data, unsigned long datasize, bool IsWR);
BOOL ata_pass_through_ioctl_DMA(int device, IDEREGS * regs, unsigned char * data, unsigned long datasize, bool IsWR);
BOOL ata_pass_through_ioctl_DMAEXT(HANDLE hdevice, IDEREGS * regs,IDEREGS * regspre, unsigned char * data, unsigned long datasize, bool IsWR);
BOOL IDFY_IDE(LPCTSTR driveName,BYTE *idfy,int device);
BOOL Write_Disk_IDE(LPCTSTR drivename, BYTE *buf, DWORD *rebyte, int *pLBA);
BOOL Read_Disk_IDE(LPCTSTR drivename, BYTE *buf, DWORD *rebyte, int *pLBA);

BOOL SLWGetSn(int device);
void reversedstr(BYTE *buffer, int count);
BOOL CHK_DISK(int device,LPCTSTR driver);
BOOL DMA_OPS(int device,BYTE *buf, DWORD bytes, DWORD dLBA, bool IsWR, BYTE CMD);
//BOOL DMA_OPS(BYTE *buf, DWORD bytes, DWORDLONG dLBA, bool IsWR, BYTE CMD);
BOOL PIO_OPS(int device, BYTE *buf, DWORD bytes, DWORD dLBA, bool IsWR, BYTE CMD);
BOOL PIO_VENDOR_OPS(int device, bool IsWR, BYTE *buf, DWORD bytes, IDEREGS *regs);
BOOL SLW_SSD_DBG(int device,bool enter);
BOOL VNDReadflashid(int device);
BOOL VNDSetparam(int device);
BOOL VNDReadpage(int device,BYTE* buf,int channelnum,int dienum,DWORD page);
int VNDEraseblk(int device,int channelnum,int dienum,DWORD blocknum);
int VNDWriteblock(int device,BYTE* buf,int chdie,DWORD blocknum,int sector);
BOOL VNDReadblock(int device,BYTE* buf,int channelnum,int dienum,DWORD blocknum);
BOOL VNDReadSPI(int device,BYTE* DATABuffer, int SectorOff);
BOOL VNDWriteSPI(int device,BYTE* DATABuffer, DWORD LBA,int SectorOff);
BOOL VNDPadRead(int device,int channelnum,int dienum,DWORD page);
BOOL SLWFindBadblock(int device);
BOOL SLWReadResult(int device);
BOOL SLWAddressAssign(int device);
BOOL CompareBadblock(int device,int blocknum,int channelnum,int dienum);
int SLWEraseBlock(int device,int channelnum,int rownum,int blocknum);
BOOL WriteResult(int device,BOOL CreatFlag);
BOOL SLWGetDeviceInfo(int device,int Initnum);
BOOL SLWWriteGlobleInfoBlock(int device,int Initnum);
BOOL SLWWriteBadReserveBlock(int device,int Initnum);
BOOL SLWWriteCodeBlock(int device);
BOOL SLWWritesmartInfo(int device,PUCHAR buf,long offset);
BOOL SLWWriteMapTableToFLASH(int device,int Initnum);
BOOL SLWWritesecurityBlock(int device);
BOOL SLWWritePhyBlock(int device,PUCHAR DATABuffer,int channelnum,int rownum,int layernum,int blocknum,int elbn);
BOOL SLWRWSPI(int device,int sector,PUCHAR DATABuffer,int rw);
long SLWGlobalinfo(int device,PUCHAR buf,int Initnum,long offset,BOOL CodeFlag);
BOOL SLWReadfpga(int device);
BOOL SLWReadfpgaDr(int device);
BOOL SLWWritefpga(int device);
void SLWWritefw(int device);
BOOL SLWWritefpgaDr(int device);
BOOL SLWWritefwDr(int device);
void SLWUpdateIdentify(int device);
BOOL SLWReadfw(int device);
BOOL SLWReadfwDr(int device);
BOOL SLWReadGlobalinfoblockTxt(CString File);
BOOL SLWWriteGlobalinfoblockTxt(CString File);
int SLWInitial(int Device,int Initnum);
BOOL ReadConfigureIni(CString FilePath);
void AddInfo( LPCTSTR buf);
void ResetGlobal(int device,int Initnum);
BOOL SLWUpdateBadblock(int device,int channelnum,int dienume,int blocknum);
BOOL VNDFW2DRAM(int device);
BOOL VNDFW2ATA(int device,BYTE* buf);
BOOL VNDWFW2DRAM(int device,BYTE* buf);
BOOL VNDWFW2FLASH(int device);	
BOOL VNDDriveRSPI(int device,int pageoff,BYTE* buf,int pagenum);
BOOL VNDDriveWSPI(int device,int pageoff,BYTE* buf,int pagenum);
void SLWEraseAll(int device);
int SLWGetInterfaceflag(int device);
BOOL SLWSaveFile(PUCHAR ReadBuffer,int datasize,BOOL flag);
BOOL VNDSetBlockArg(int device,int offset,int length,int strip);
BOOL SLWWriteGMBlock(int device,int Initnum);
BOOL SLWIDFY(int device,unsigned char* CSWBuffer);
BOOL SLWReadBadblock(int device);
BOOL DMAWriteAll( int device, DWORD TotalSector, DWORD dLBA,int sector);
BOOL DMA_HANDLE( int device,BYTE *buf, DWORD bytes, DWORD dLBA, bool IsWR, BYTE CMD,HANDLE handle);
void SLWSetCapacity(int device);
BOOL VNDEraseSPI(int device,int SectorOff);
BOOL GetDeviceProperty(HDEVINFO IntDevInfo, DWORD Index );
BOOL FindSSD();
BOOL CloseSSDHandle();
BOOL VNDReadSector(int device,BYTE* buf,int channelnum,int dienum,DWORD page,int sector);
BOOL VNDReadNV(int device,BYTE* buf);
BOOL VNDReadGC(int device,BYTE* buf);
BOOL VNDReadSWAP(int device,BYTE* buf);
int WriteAll(int device);
BOOL WriteLoginblock(int device, DWORD total, DWORD dLBA,int sector);
int SSDInitialize(int device);
int GetSSDStatus(int device);
void SetMultibootIdentify(PUCHAR buf,DWORD multibootsize,CString model);
BOOL SLWWriteMultiboot(int device,int Initnum);
void SLWUpdateBadblockFile(CString strDldFile);
BOOL SLWDownSRAM(int device,CString strDlgFile);
BOOL SLWUpdateLoadFw(int device);
void LogInfo(int device,LPCTSTR buf,int loglevel);
int SLWFlashIDPos();
BOOL FindRamDisk();
BOOL GetRamDiskProperty(HDEVINFO IntDevInfo, DWORD Index );
int slwatoh(char c);
int  GetNewSSDHandle(int sataport);
BOOL GetNewDeviceProperty(HDEVINFO IntDevInfo, DWORD Index,int sataport );
BOOL SearchDevSnMap(char* devicepath,char* devsn);
BOOL GetFlashTypePar(BYTE	*type, BYTE	*par);
BOOL ConvertData(char *date);
/*

	SW80 函数 开始


*/
// SW80 vendor command
#define SW80_CMD_VND 						0xEF	// vendor command
#define SW80_CMD_FEATURE 						0xE0	// vendor feature
#define SW80_CMD_VND_SUB_ENDBG 					0xAD	// 进入vendor模式
#define SW80_CMD_VND_SUB_OUTBG 					0xDA	// 退出vendor模式
#define	SW80_CMD_VND_SUB_WSRAM					0x52	// 写入数据到指定SRAM地址
#define SW80_CMD_VND_SUB_FW_RUN 				0x53	//程序指针跳转到指定SRAM地址	 
#define SW80_CMD_VND_SUB_READFLASHID 			0x54	// read flash id
#define SW80_CMD_VND_SUB_ERASEBLOCK				0x55	// 擦除指定block
#define	SW80_CMD_VND_SUB_WRITEPAGE				0x56	// 编程指定page数据
#define	SW80_CMD_VND_SUB_READPAGE				0x57	// 读取指定page数据
#define	SW80_CMD_VND_SUB_SETBCH 				0x58	// 设置ECC模式
#define	SW80_CMD_VND_SUB_RUNFWSW				0x59	// 写入数据到指定SRAM地址
#define SW80_CMD_VND_SUB_ERASEBLOCK4CH				0x60	// 擦除指定4个channel的block
#define SW80_CMD_VND_SUB_FINDBLOCK4CH				0x64	// 查找4个channel的原始坏块
#define SLW_CMD_VND_SUB_NFC						0x65	//传输flash参数给RAM DISK，用于配置NFC。

#define  SW80GLOBALINFOFILE      "SW80Gblock.txt";

#define	 FW_POS	0x2000000
extern FLASHENDURANCE FlashEndurance[64];
extern int gucflag;
BOOL SW80FindRamDisk();
ULONGLONG SW80GetVendorLBA(int device);
BOOL SW80IDFYSSD(int device,ULONGLONG vendorlba);
BOOL SW80GetDeviceProperty(HDEVINFO IntDevInfo, DWORD Index );
BOOL SW80WriteRam(int device,DWORD Addr);
BOOL SW80RunFW(int device,DWORD Addr);
BOOL SW80ReadFlashid(int device,int channel,int die,BYTE *ID);
BOOL SW80Eraseblock(int device,int channel,int die,int block);
BOOL SW80PageRead(int device,int channel,int die,DWORD page,BOOL bypassflag,BYTE *databuf);
BOOL SW80PageWrite(int device,int channel,int die,DWORD page,BYTE *oob,BYTE *databuf);
BOOL SW80SetBCH(int device,int BCHmode);
BOOL SW80RunSWFW(int device, BYTE *buf);
BOOL WriteSectors(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff) ;
BOOL ReadSectors(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff);
int SW80FlashIDPos();
BOOL DownSRAM(int device); 
BOOL SW80UpdateLoad(int device,BYTE* DATABuffer);
BOOL SW80SSDVenderMode(int device,ULONGLONG vendorlba,BOOL venderflag);
BOOL SW80ReadCMDStatus(int device,ULONGLONG vendorlba,BYTE *result);
BOOL EnterVND(int device) ;
BOOL SW80VenderMode(int device,BOOL venderflag);
BOOL DoIdentifyDeviceSat(int device, BYTE target, BYTE *data);
BOOL ata_via_scsi_write(int device, BYTE target,LONGLONG dwStartSector, WORD wSectors, BYTE *data);
BOOL ata_via_scsi_read(int device, BYTE target,LONGLONG dwStartSector, WORD wSectors, BYTE *data);
BOOL SLWSetFlash() ;
int FindFlashtypePos();
int GetDIEnumPerChip();
BOOL VNDSetFLASHArg(int device);
int SW80DistSSD(int device);
BOOL SW80SetNFC (int device);
BOOL SW80WriteFWInfo(int device,PUCHAR DATABuffer,long offset)	;
BOOL SW80ReadGlobalinfoblockTxt(CString File);
int SW80GetDIEnumPerChip();
BOOL SW80ReadConfigureIni(CString FilePath);
BOOL VNDFW2ATA48b(int device,BYTE* buf);
BOOL ata_pass_through_ioctl_pio48b(int device, IDEREGS * regs,IDEREGS * regspre, unsigned char * data, unsigned long datasize, bool IsWR);
BOOL ata_via_scsi_pio48b(HANDLE hdevice, IDEREGS * regs, IDEREGS * regspre, unsigned char * data, unsigned long datasize, bool IsWR);
BOOL VNDDriveRSPI48b(int device,int pageoff,BYTE* buf,int pagenum);
BOOL WriteData(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff) ;
BOOL ReadData(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff) ;
#endif
