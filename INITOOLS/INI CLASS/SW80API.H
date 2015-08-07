#ifndef _SW80API_H_
#define _SW80API_H_

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
//#include <devioctl.h>  
//#include <ntdddisk.h>
#include <ntddscsi.h>
#include "devioctl.h"
#include "ntdddisk.h"


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
#define SW80_CMD_VND_SUB_ERASEBLOCK4CH			0x60	// 擦除指定4个channel的block
#define SW80_CMD_VND_SUB_FINDBLOCK4CH			0x64	// 查找4个channel的原始坏块
#define SW80_CMD_VND_SUB_READFLASHUID			0x63	//读指定flash的UNIQUE ID
#define SLW_CMD_VND_SUB_REBUILD					0x01	//读取Rebuild状态
#define SLW_CMD_VND_SUB_NFC						0x65	//传输flash参数给RAM DISK，用于配置NFC。
#define SLW_CMD_VND_SMART_RESET					0xAE	//Smart信息清零
#define SLW_CMD_VND_SMART_RESTORE				0xAF	//Smart信息还原



#define  DeviceCount 16			//最多可同时初始化16个SSD
#define  InitialCount 16		//初始化重试次数
#define  MAX_IDE_DRIVES  16
#define  IDENTIFY_BUFFER_SIZE  512
#define	 FW_POS	0x2000000
//  Valid values for the bCommandReg member of IDEREGS.
#define  IDE_ATAPI_IDENTIFY  0xA1  //  Returns ID sector for ATAPI.
#define  IDE_ATA_IDENTIFY    0xEC  //  Returns ID sector for ATA.

#define  SLWCONFINI          "SW80_Configure.ini";
#define  GLOBALINFOFILE      "SW80Gblock.txt";
#define	 USEPIN				 "solissd3764$3#jkKJF09*OPSCB"
#define	 SOPIN				 "!90EFEKOL*SW80KALSYECJANCLL"
#define  SW80RULEFILE		 "SW80ProductionRule.txt"

//  IOCTL commands
#define  DFP_GET_VERSION          0x00074080
#define  DFP_SEND_DRIVE_COMMAND   0x0007c084
#define  DFP_RECEIVE_DRIVE_DATA   0x0007c088

#define IOCTL_STORAGE_QUERY_PROPERTY   CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef  unsigned char INT8U;
typedef  unsigned short INT16U;
typedef  unsigned int INT32U;
 
//  GETVERSIONOUTPARAMS contains the data returned from the 
//  Get Driver Version function.
typedef struct _GETVERSIONOUTPARAMS
{
	BYTE bVersion;      // Binary driver version.
	BYTE bRevision;     // Binary driver revision.
	BYTE bReserved;     // Not used.
	BYTE bIDEDeviceMap; // Bit map of IDE devices.
	DWORD fCapabilities; // Bit mask of driver capabilities.
	DWORD dwReserved[4]; // For future use.
} GETVERSIONOUTPARAMS, *PGETVERSIONOUTPARAMS, *LPGETVERSIONOUTPARAMS;

struct ExDriverInfo{
	INT8U DeviceID;
	INT16U BaseAddr;
	INT8U DriverNum;
	INT8U type;
};


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
	INT16U plane_per_die;
}Flash_Para, *PFlash_Para;

typedef struct _produce_rule
{
	INT16U enable_two_plane;
	INT16U ECC_alarm_threshold;
	INT16U res_ratio;
	INT16U FCW;	  /*FW功能控制字*/
	INT16U respage_per_64Pages;
	INT16U lblock_include_pblock;
}PRODUCERULE;

typedef struct  _idfy_struct 
{
	INT8U word1;
	INT8U length1;
	INT32U Mask1;
	INT8U offset1;

	INT8U word2;
	INT8U length2;
	INT32U Mask2;
	INT8U offset2;

	bool IsRetired;
	bool IsFlag;
	
	char strNameCN[256];
	char strNameEN[256];
	
	bool IsString;


}IDFY_DATA;

typedef struct _smart_data_attri
{
	INT8U id;
	char name[50];
	char des[150];
	INT8U currentval;
	INT8U worstval;
	INT8U thresholdval;
}SMARTDATA;

typedef   struct   
{
	int device;	
}   TEST_STRU,*pTEST_STRU;

extern BOOL checkinputid;
extern Flash_Para flash_type[DeviceCount];
extern HANDLE Devicehandle[DeviceCount];
extern int SSDID;
extern DWORD MaxLBA;
extern int GlobalDevice;
extern char LocalPath[256];
extern int channel;
extern int plane;
extern int gldie ; //物理die,约定一个channel有16个物理die,允许为空，但物理die号保留
extern int grdie[DeviceCount] ;   //每个channel实际存在的物理die数目，当前是8，可以根据DIEMAP取
extern int gcs[DeviceCount] ; //片选数目，一般一个片选有2个die(当前使用的坏块表文件是4个channel,8个cs)
extern int step ;				//SS MC:step=2; HY step=1; plane内block地址递增是偶数还是基数
extern BYTE flash_id[DeviceCount][64][4];
extern int Retrycnt;
extern int DIEMAP[DeviceCount][16];
extern IDFY_DATA IdfyData[256];
extern int badblock[DeviceCount][128][512];		//坏块表,最多128 die,
extern int xblock[DeviceCount][128];			//坏块个数,最多128 die,
extern int IniMode;
extern TEST_STRU IOtest[DeviceCount];
extern int StartFlag;
extern int SelectDevice;
extern CString SelectBadblockfile;
extern CString BadBlockFilename[DeviceCount];
extern CString capacity[DeviceCount];
extern CString	m_fw[DeviceCount];
extern CString	m_descriptor;
extern CString FWFilename;
extern CString	LoadFilename;
extern BOOL checkinit;
extern CString manufacture;
extern BOOL checkvalid;
extern int Resratio;
extern int Respage;
extern int Mapphyblock;
extern int Incphyblock;
extern int Datalength;
extern BOOL EnableSata1;
extern int MaxPU;
extern int EnableTwoplane;
extern int MaptableOffset;
extern BOOL EnableNewrule;
extern int InterfaceType[DeviceCount];
extern FLASHENDURANCE FlashEndurance[64];
extern int RuleNum;
extern PRODUCERULE ProduceRule[64];
extern int FunCtlWord;
extern int ECCThreshold;
extern unsigned int FLASHNum;
extern CString	RamdiskFilename;
extern CString RAMDISKSN;
extern long Vernum;
extern int AcessControlFlag;
extern int AcessControlblock;
extern DWORD PartionSize;
extern DWORD MirrorSize;
extern DWORD CapcityGB;
extern BOOL PIOFlag[DeviceCount];
extern int Purgetime;
extern DWORD StartTime;
extern int   FWBlockNum;
extern int	 Restartflag;
extern _int64 SetCapacity;


int  InitDisk(int device);
BOOL FindGUCDisk();
BOOL CloseSW80SSDHandle();
BOOL GetDeviceProperty(HDEVINFO IntDevInfo, DWORD Index );
BOOL DoIDENTIFY (HANDLE hPhysicalDriveIOCTL, PSENDCMDINPARAMS pSCIP,
                 PSENDCMDOUTPARAMS pSCOP, BYTE bIDCmd, BYTE bDriveNum,
                 PDWORD lpcbBytesReturned);
CString GetDriveGeometry(HANDLE hDevice);
char *ConvertToString (USHORT diskdata [256], int firstIndex, int lastIndex);
BOOL WriteSectors(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff) ;
BOOL ReadSectors(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff);
BOOL SW80VenderMode(int device,BOOL venderflag);
BOOL SW80WriteRam(int device,DWORD Addr);
BOOL SW80RunFW(int device,DWORD Addr);
BOOL SW80ReadFlashid(int device,int channel,int die,BYTE *ID);
BOOL SW80Eraseblock(int device,int channel,int die,int block);
BOOL SW80PageRead(int device,int channel,int die,DWORD page,BOOL bypassflag,BYTE *databuf);
BOOL SW80PageWrite(int device,int channel,int die,DWORD page,BYTE *oob,BYTE *databuf);
BOOL SW80SetBCH(int device,int BCHmode);
BOOL SW80RunSWFW(int device, BYTE *buf);
BOOL SW80IDFYSSD(int device,ULONGLONG vendorlba);
int SLWFlashIDPos(int device);
BOOL ata_pass_through_ioctl_pio(HANDLE hdevice, IDEREGS * regs, unsigned char * data, unsigned long datasize, bool IsWR);
BOOL IDFY_IDE(int device,BYTE *idfy);
BOOL SW80FindBadblock(int device);
BOOL SW80GetDeviceInfo(int device,int Initnum);
void AddInfo(int device,LPCTSTR buf,int level);
void SW80EraseAll(int device);
BOOL ReadResult(int device);
BOOL SW80ReadBadblock(int device);
BOOL WriteResult(int device,BOOL CreatFlag);
BOOL SW80UpdateBadblock(int device,int channelnum,int dienume,int blocknum);
int SWInitial(int device,int Initnum );
int SSDInitialize(int device);
BOOL SW80WriteGlobalBlock(int device,unsigned char * data,int channelnum,int dienume,int blocknum);
BOOL SW80ReadGlobalBlock(int device,unsigned char * data,int channelnum,int dienume,int blocknum);
BOOL SW80CompareGlobalBlock(int device,unsigned char * data,int channelnum,int dienum,int blocknum);
void ResetGlobal(int device,int Initnum);
BOOL ReadConfigureIni(CString FilePath);
BOOL SWBuildGlobleInfoBlock(int device);
void SW80WirteBadblockflag(int device);
BOOL SW80ReadGlobalinfoblockTxt(CString File);
int slwatoh(char c);
BOOL SLWGetSn(int device);
void SLWSetCapacity(int device);
long SLWGlobalinfo(int device,PUCHAR buf,int Initnum,long offset,BOOL CodeFlag);
BOOL SW80Eraseblock4CH(int device,BYTE *blockaddr,BYTE *status);
BOOL VerifyBadblock(int device,int channelnum,int dienume,int blocknum);
void Setblockaddr(int device,int die,int block,BYTE *blockaddr);
void SetFlashCap(int device);
BOOL SW80CheckPage(int device,int channel,int die,DWORD page,BOOL bypassflag,BYTE *databuf);
BOOL SW80Findbadblock4CH(int device,int dienum);
BOOL SW80ReadCMDStatus(int device,ULONGLONG vendorlba,BYTE *result);
BOOL SW80SSDVenderMode(int device,ULONGLONG vendorlba,BOOL venderflag);
ULONGLONG SW80GetVendorLBA(int device);
BOOL SW80UpdateFWInRamdisk(int device);
BOOL SW80SSDReadFW(int device,ULONGLONG vendorlba,BYTE *buf);
BOOL SW80SSDWriteFW(int device,ULONGLONG vendorlba,BYTE *buf);
BOOL SW80SSDUpdateFW(int device,ULONGLONG vendorlba);
BOOL SW80UpdateLoad(int device,CString strDlgFile,int flag);
BOOL SW80SSDUpdateLoad(int device,ULONGLONG vendorlba,CString strDlgFile);
BOOL SW80SSDWriteLoad(int device,ULONGLONG vendorlba,DWORD OOB,BYTE *buf);
BOOL SW80SSDReadLoad(int device,ULONGLONG vendorlba,BYTE *buf);
BOOL SW80SSDReadDiskinfo(int device,ULONGLONG vendorlba,BYTE *buf);
BOOL SW80WritesmartInfo(int device,PUCHAR DATABuffer,long offset);
BOOL EnterVND(int device);
BOOL DownSRAM(int device,CString file);  
BOOL OutVND(int device); 
BOOL ReadflashPara(int device);
BOOL ReadFlashID(int device);
BOOL Updateloader(int device,int flag); 
BOOL SW80ReadFlashUID(int device,int channel,int die,BYTE *UID);
BOOL ReadFlashUID(int device,BOOL fileflag);
BOOL Read_Rebuild(int device,BYTE *idfy);
BOOL DoIdentifyDeviceSat(int device, BYTE target, BYTE *data);
BOOL ata_via_scsi_write(int device, BYTE target,LONGLONG dwStartSector, WORD wSectors, BYTE *data);
BOOL ata_via_scsi_read(int device, BYTE target,LONGLONG dwStartSector, WORD wSectors, BYTE *data);
BOOL ata_via_scsi_write_dma(int device, BYTE target,LONGLONG dwStartSector, WORD wSectors, BYTE *data);
BOOL ata_via_scsi_read_dma(int device, BYTE target,LONGLONG dwStartSector, WORD wSectors, BYTE *data);
BOOL WriteData(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff); 
BOOL ReadData(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff);
BOOL SW80WriteFWInfo(int device,PUCHAR DATABuffer,long offset);
BOOL SW80SetNFC (int device);
BOOL SW80UpdateFWInfo(int device,PUCHAR DATABuffer,long offset)	;
BOOL CompareBadblock(int device,int blocknum,int channelnum,int dienum);
BOOL SW80FindGCblock(int device,int *channelnum,int *dienum,int *blocknum);
int SW80DistSSD(int device);
int SW80GetDIEnumPerChip(int device);
BOOL SW80VerifyBadblock(int device,int channelnum,int dienume,int blocknum);
BOOL SW80Twoplanebadblock(int device);
BOOL ConvertData(char *date);
BOOL SW80Readchanelnum(int device);
BOOL SW80ReadRuleTxt(CString File);
BOOL SW80SetSNtoRamdisk(int device,unsigned char* DATABuffer,unsigned char* sid);
BOOL SW80SaveNV(char* file,unsigned char* buf);
BOOL SW80SaveGC(char* file,unsigned char* buf);
BOOL SW80SaveInterpara(int device, char* file);
BOOL SW80UpdateFWInfoInRAM(int device,PUCHAR DATABuffer,long offset);	
BOOL SW80VerifyPassword(int device,ULONGLONG vendorlba,BYTE *buf,int length,BOOL Swithflag);
BOOL SW80ChangePassword(int device,ULONGLONG vendorlba,BYTE *oldps,int len1,BYTE *newps,int len2,int bypass);
BOOL SW80CheckProtectStatus(int device,ULONGLONG vendorlba,BYTE *result,BYTE *partitionflag);
int SW80CheckGlobal(int device,ULONGLONG vendorlba);
BOOL ata_reconnect(int device,BYTE *buf);
BOOL IDFYbyDoIDENTIFY(int device,BYTE *idfy);
BOOL SW80_DataClear(int device);
BOOL SW80CreateHandle(int device);
BOOL SetRestoreArgs(int device,BYTE *buf);
BOOL GetRestoreArgs(int device,BYTE *buf);
BOOL SW80CheckRetryStatus(int device,ULONGLONG vendorlba,int *cnt);
BOOL SW80CompareGlobalBlock(int device,unsigned char * data,int channelnum,int dienum,int blocknum);
BOOL SetTwoPlane(int device);
BOOL SW80SmartRestore(int device,ULONGLONG vendorlba);
BOOL SW80SmartReset(int device,ULONGLONG vendorlba);

#endif