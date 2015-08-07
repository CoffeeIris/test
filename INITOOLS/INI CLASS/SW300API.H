#ifndef _SW300API_H_
#define _SW300API_H_

#if defined(_WIN64)
 typedef unsigned __int64 ULONG_PTR;
#else
 typedef unsigned long ULONG_PTR;
#endif
typedef unsigned long DWORD_PTR; 
#include  <winioctl.h>
#include <setupapi.h>   // for SetupDiXxx functions.


// SW300 vendor command
#define SW300_CMD_VND 						    0xFD	// vendor command
#define SW300_CMD_RWSPI 						0xFC	// Read/Write FSI
#define SW300_CMD_RWNFLASH 						0xFE	// Read/Write NOR Flash
#define SW300_CMD_VND_SUB_ENDBG 				0xFF	// 进入vendor模式或者WRITE命令
#define SW300_CMD_VND_SUB_OUTBG 				0xFE	// 退出vendor模式或者READ命令


#define  DeviceCount 6
#define  InitialCount 16		//初始化重试次数
#define  MAX_IDE_DRIVES  6
#define  IDENTIFY_BUFFER_SIZE  512
#define	 FW_POS	0x2000000
//  Valid values for the bCommandReg member of IDEREGS.
#define  IDE_ATAPI_IDENTIFY  0xA1  //  Returns ID sector for ATAPI.
#define  IDE_ATA_IDENTIFY    0xEC  //  Returns ID sector for ATA.

#define  SLWCONFINI          "SW300_Configure.ini";
#define  GLOBALINFOFILE      "SW300Gblock.txt";
#define	 USEPIN				 "solissd3764$3#jkKJF09*OPSCB"
#define	 SOPIN				 "!90EFEKOL*SW300KALSYECJANCLL"
#define  SW300RULEFILE		 "SW300ProductionRule.txt"

//  IOCTL commands
#define  DFP_GET_VERSION          0x00074080
#define  DFP_SEND_DRIVE_COMMAND   0x0007c084
#define  DFP_RECEIVE_DRIVE_DATA   0x0007c088

#define IOCTL_STORAGE_QUERY_PROPERTY   CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef  unsigned char INT8U;
typedef  unsigned short INT16U;
typedef  unsigned int INT32U;
 
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
 
//  IDE registers
typedef struct _IDEREGS
{
	BYTE bFeaturesReg;       // Used for specifying SMART "commands".
	BYTE bSectorCountReg;    // IDE sector count register
	BYTE bSectorNumberReg;   // IDE sector number register
	BYTE bCylLowReg;         // IDE low order cylinder value
	BYTE bCylHighReg;        // IDE high order cylinder value
	BYTE bDriveHeadReg;      // IDE drive/head register
	BYTE bCommandReg;        // Actual IDE command.
	BYTE bReserved;          // reserved for future use.  Must be zero.
} IDEREGS, *PIDEREGS, *LPIDEREGS;

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

// Status returned from driver
typedef struct _DRIVERSTATUS
{
	BYTE  bDriverError;  //  Error code from driver, or 0 if no error.
	BYTE  bIDEStatus;    //  Contents of IDE Error register.
	//  Only valid when bDriverError is SMART_IDE_ERROR.
	BYTE  bReserved[2];  //  Reserved for future expansion.
	DWORD  dwReserved[2];  //  Reserved for future expansion.
} DRIVERSTATUS, *PDRIVERSTATUS, *LPDRIVERSTATUS;

typedef struct _SENDCMDINPARAMS
{
	DWORD     cBufferSize;   //  Buffer size in bytes
	IDEREGS   irDriveRegs;   //  Structure with drive register values.
	BYTE bDriveNumber;       //  Physical drive number to send 
	//  command to (0,1,2,3).
	BYTE bReserved[3];       //  Reserved for future expansion.
	DWORD     dwReserved[4]; //  For future use.
	BYTE      bBuffer[1];    //  Input buffer.
} SENDCMDINPARAMS, *PSENDCMDINPARAMS, *LPSENDCMDINPARAMS;

// Structure returned by PhysicalDrive IOCTL for several commands
typedef struct _SENDCMDOUTPARAMS
{
	DWORD         cBufferSize;   //  Size of bBuffer in bytes
	DRIVERSTATUS  DriverStatus;  //  Driver status structure.
	BYTE          bBuffer[1];    //  Buffer of arbitrary length in which to store the data read from the                                                       // drive.
} SENDCMDOUTPARAMS, *PSENDCMDOUTPARAMS, *LPSENDCMDOUTPARAMS;

struct ExDriverInfo{
	INT8U DeviceID;
	INT16U BaseAddr;
	INT8U DriverNum;
	INT8U type;
};

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


typedef struct _SW300_Para
{
	BYTE par0;
	BYTE par1;
	BYTE par2;
	BYTE par3;
	BYTE par4;
	BYTE par5;
	BYTE par6;
}SW300_Para, *PSW300_Para;

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

typedef   struct   
{
	int device;	
}   TEST_STRU,*pTEST_STRU;

extern Flash_Para flash_type;
extern HANDLE Devicehandle[DeviceCount];
extern int SSDID;
extern DWORD MaxLBA;
extern INT8U DeviceID;
extern int GlobalDevice;
extern char LocalPath[128];
extern int channel;
extern int plane;
extern int gldie ; //物理die,约定一个channel有16个物理die,允许为空，但物理die号保留
extern int grdie ;   //每个channel实际存在的物理die数目，当前是8，可以根据DIEMAP取
extern int gcs ; //片选数目，一般一个片选有2个die(当前使用的坏块表文件是4个channel,8个cs)
extern int step ;				//SS MC:step=2; HY step=1; plane内block地址递增是偶数还是基数
extern BYTE flash_id[64][4];
extern int page4k ;		// 1: 2K		2: 4K	4: 8K
extern int blocksize; 	// 1: 128K	2: 256k	4: 512k
extern int sectorperblock;		//block包含sector的数目，MLC 1024 SLC 512 
extern int DIEMAP[16];
extern IDFY_DATA IdfyData[256];
extern int badblock[DeviceCount][128][512];		//坏块表,最多128 die,
extern int xblock[DeviceCount][128];			//坏块个数,最多128 die,
extern int IniMode;
extern TEST_STRU IOtest[DeviceCount];

extern CString SelectBadblockfile;
extern CString BadBlockFilename[DeviceCount];
extern CString capacity[DeviceCount];
extern CString	m_fw[DeviceCount];
extern CString	m_descriptor[DeviceCount];
extern CString FWFilename;
extern CString	LoadFilename;
extern BOOL checkinit;
extern CString manufacture;
extern BOOL checkvalid;
extern int Resratio;

extern int InterfaceType[DeviceCount];
extern FLASHENDURANCE FlashEndurance[64];
extern int ECCThreshold;
extern unsigned int FLASHNum;
extern CString	RamdiskFilename;
extern CString RAMDISKSN;
extern long Vernum;

BOOL FindSW300Disk();
BOOL CloseSW300SSDHandle();
BOOL GetDeviceProperty(HDEVINFO IntDevInfo, DWORD Index );
BOOL DoIDENTIFY (HANDLE hPhysicalDriveIOCTL, PSENDCMDINPARAMS pSCIP,
                 PSENDCMDOUTPARAMS pSCOP, BYTE bIDCmd, BYTE bDriveNum,
                 PDWORD lpcbBytesReturned);
CString GetDriveGeometry(HANDLE hDevice);
char *ConvertToString (USHORT diskdata [256], int firstIndex, int lastIndex);
BOOL WriteSectors(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff) ;
BOOL ReadSectors(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff);
BOOL SW300ReadFlashid(int device,int channel,int CE,BYTE *ID);
BOOL SW300PageRW(int device,int channel,int CE,DWORD page,BYTE *databuf,int flag);
BOOL SW300SetBCH(int device,int BCHmode);
BOOL SW300IDFYSSD(int device,ULONGLONG vendorlba);
int SLWFlashIDPos();
BOOL ata_pass_through_ioctl_pio(HANDLE hdevice, IDEREGS * regs, unsigned char * data, unsigned long datasize, bool IsWR);
BOOL IDFY_IDE(int device,BYTE *idfy);
void AddInfo(int device,LPCTSTR buf,int level);
BOOL ReadResult(int device);
BOOL SW300ReadBadblock(int device);
BOOL WriteResult(int device,BOOL CreatFlag);
BOOL SW300UpdateBadblock(int device,int channelnum,int dienume,int blocknum);
int SWInitial(int device,int Initnum );
int SSDInitialize(int device);
BOOL SW300WriteGlobalBlock(int device,unsigned char * data,int channelnum,int dienume,int blocknum);
BOOL SW300ReadGlobalBlock(int device,unsigned char * data,int channelnum,int dienume,int blocknum);
void ResetGlobal(int device,int Initnum);
BOOL ReadConfigureIni(CString FilePath);
BOOL SWBuildGlobleInfoBlock(int device);
BOOL SW300ReadGlobalinfoblockTxt(CString File);
int slwatoh(char c);
BOOL SLWGetSn(int device);
void SLWSetCapacity(int device);
long SLWGlobalinfo(int device,PUCHAR buf,int Initnum,long offset,BOOL CodeFlag);
BOOL VerifyBadblock(int device,int channelnum,int dienume,int blocknum);
void Setblockaddr(int device,int die,int block,BYTE *blockaddr);
ULONGLONG SW300GetVendorLBA(int device);
BOOL SW300UpdateFWInRamdisk(int device);
BOOL SW300SSDReadFW(int device,ULONGLONG vendorlba,BYTE *buf);
BOOL SW300SSDWriteFW(int device,ULONGLONG vendorlba,BYTE *buf);
BOOL SW300SSDUpdateFW(int device,ULONGLONG vendorlba);
BOOL SW300UpdateLoad(int device,CString strDlgFile,int flag);
BOOL SW300SSDUpdateLoad(int device,ULONGLONG vendorlba,CString strDlgFile);
BOOL SW300SSDWriteLoad(int device,ULONGLONG vendorlba,DWORD OOB,BYTE *buf);
BOOL SW300SSDReadLoad(int device,ULONGLONG vendorlba,BYTE *buf);
BOOL SW300SSDReadDiskinfo(int device,ULONGLONG vendorlba,BYTE *buf);
BOOL EnterVND(int device);
BOOL DownSRAM(int device,int fwflag);  
BOOL OutVND(int device); 
BOOL ReadflashPara(int device);
BOOL ReadFlashID(int device);
BOOL Updateloader(int device,int flag); 
BOOL SW300ReadFlashUID(int device,int channel,int die,BYTE *UID);
BOOL ReadFlashUID(int device,BOOL fileflag);
BOOL DoIdentifyDeviceSat(int device, BYTE target, BYTE *data);
void SW300EraseAll(int device);
int SW300DistSSD(int device);
int SW300GetDIEnumPerChip();
BOOL ConvertData(char *date);
BOOL SW300SSDVenderMode(int device,ULONGLONG vendorlba,BOOL venderflag);
BOOL SW300FindBadblockAll(int device,int CE,int blkcnt,int sblk);
BOOL SW300Eraseblk(int device,int channelmask,int CE,int blk);
void SW300WirteBadblockflag(int device,int channel,int CE,int blk);
BOOL SW300ReadNORFLASH(int device,int sctcount,int sctoffset,BYTE *SectBuff);
BOOL SW300WriteNORFLASH(int device,int sctcount,int sctoffset,BYTE *SectBuff);
BOOL SW300ATARWNORFLASH(int device,INT16U sctoffset,BOOL venderflag,BYTE * buf);
BOOL SW300ATARWSPI(int device,SW300_Para VAR,BOOL IsWR,BYTE * buf,int datasize);
BOOL SW300VenderMode(int device,BOOL venderflag);
BOOL SW300RunFW(int device,BYTE* Databuf,DWORD length);
BOOL SW300RWSPI(int device,int flag,INT16U sectoroffset,BYTE* sctbuf);
BOOL SW300RunRam80(int device,BYTE* Databuf);
BOOL SW300RWRAM(int device,int flag,INT16U sectoroffset,BYTE* sctbuf);
BOOL SW300RWDDR(int device,int flag,INT16U sectoroffset,BYTE* sctbuf);
BOOL SW300RWFSIData(int device,int flag,int ECCflag,int bank,int bus,INT16U sectoroffset,BYTE* sctbuf);
BOOL ata_pass_through_ioctl_pio_48b(HANDLE hdevice, IDEREGS * regs,IDEREGS * regspre, unsigned char * data, unsigned long datasize, bool IsWR);

#endif