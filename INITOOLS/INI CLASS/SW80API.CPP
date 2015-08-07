#include "stdafx.h"
#include "SW80API.h"
#include "COMMANAPI.h"
#include "OPini.h"	
#include <math.h>

#if _INITD 
#include "..\\GUC INITOOLS\\DlgCmd.h"
extern CDlgCmd *pdlgcmd;
#endif

#if _MASSINIT 
#include "..\\SW80 MPTool\\Resource.h"
#include "..\\SW80 MPTool\\SW80 MPToolDlg.h"
#include "..\\SW80 MPTool\\Featuredlg.h"
extern CSW80MPToolDlg* pSW80MPToolDlg;
extern CFeaturedlg* pFeaturedlg;
#endif

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


Flash_Para flash_type[DeviceCount];
BYTE flash_id[DeviceCount][64][4];		// 	最多64 die, 每个die 有2 plane
int grdie[DeviceCount];   //每个channel实际存在的物理die数目，可以根据DIEMAP取
int gcs[DeviceCount] ; //片选数目，一般一个片选有2个die(当前使用的坏块表文件是4个channel,8个cs)
int DIEMAP[DeviceCount][16];
HANDLE Devicehandle[DeviceCount];
int badblock[DeviceCount][128][512];		//坏块表,最多128 die,
int xblock[DeviceCount][128];			//坏块个数,最多128 die,
DWORD Totalbadblock[DeviceCount];		//原始坏块统计值
TEST_STRU IOtest[DeviceCount];
CString BadBlockFilename[DeviceCount];
CString capacity[DeviceCount];
CString	m_fw[DeviceCount];
CString	m_descriptor;
PUCHAR GlobelBuffer[DeviceCount];
BOOL PIOFlag[DeviceCount];
int InterfaceType[DeviceCount];

int StartFlag=0;
int SelectDevice=0;	//在gird选择的设备号
int step =2;	
int channel = 4;
int plane = 2;
int gldie = 16; //逻辑die,约定一个channel有16个逻辑die,允许为空
int SSDID;
DWORD MaxLBA;
int GlobalDevice;
char LocalPath[256];
int IniMode;
WORD IDFY32G[256];
int Retrycnt;
unsigned int FLASHNum=0;
int RuleNum=0;
BOOL checkinit;
BOOL checkvalid;
BOOL checkinputid;
CString SelectBadblockfile;
CString FWFilename;
CString	LoadFilename;
CString	RamdiskFilename;
CString manufacture;
CString RAMDISKSN;
long Vernum=0;
int Purgetime;
int Resratio;
int Respage;
int Mapphyblock;
int FunCtlWord=0;
int Incphyblock;
int Datalength;
int MaxPU;
BOOL EnableSata1;
int EnableTwoplane=2;
BOOL EnableNewrule=0;
int MaptableOffset;
int ECCThreshold=0;
FLASHENDURANCE FlashEndurance[64];
PRODUCERULE ProduceRule[64];
int AcessControlFlag=0;
int AcessControlblock=0;
DWORD PartionSize=0;
DWORD MirrorSize=0;
DWORD CapcityGB=0;
DWORD StartTime=0;
int   FWBlockNum=1;				//FW所在块的位置
int	  Restartflag=0;
_int64 SetCapacity=0;

SMARTDATA SmartData[30]={
{0x05,"Reallocated Sectors Count","新增坏块数",100,100,0},
{0x09,"Power-On Hours Count","上电小时数",100,100,0},
{0x0C,"Power Cycle","上电次数",100,100,0},
{0xB0,"Initial Bad Block Count","出厂坏块数",100,100,0},
{0xB1,"Program fail block count","Program增长坏块数",100,100,0},
{0xB2,"Erase fail block count","Erase增长坏块数",100,100,0},
{0xB3,"Read fail block count","Read Fail增长坏块数",100,100,0},
{0xB4,"Host Writes sector count","以sectors为单位的写入数据量",100,100,0},
{0xB5,"Host reads sector count","以sectors为单位的读取数据量",100,100,0},
{0xB6,"Host Read commands","Host读命令个数",100,100,0},
{0xB7,"Host Write commands","Host写命令个数",100,100,0},
{0xCD,"Maximum PE count spec","介质的标称寿命",100,100,0},
{0xD0,"Total Erase Count","总擦除块数",100,100,0},
{0xE8,"Available Reserved Space","按PU统计的最少保留块个数",100,100,10},
{0xE9,"Media Wearout Indicator","介质损耗（百分比）",100,100,5},
{0xF1,"1bit ECC error coun","1bit ECC出现次数",100,100,0},
{0xF2,"2bit ECC error coun","2bit ECC出现次数",100,100,0},
{0xF3,"3bit ECC error coun","3bit ECC出现次数",100,100,0},
{0xF4,"4bit ECC error coun","4bit ECC出现次数",100,100,0},
{0xF5,"5bit ECC error coun","5bit ECC出现次数",100,100,0},
{0xF6,"6bit ECC error coun","6bit ECC出现次数",100,100,0},
{0xF7,">6bit ECC error count","7bit及以上 ECC出现次数",100,100,0},
{0xF8,"Reported Uncorrectable Errors","无法纠正ECC错误次数",100,100,0},
{0xF9,"CRC error count since power","本次上电以来sata CRC错误次数",100,100,0}
};



BOOL FindGUCDisk()
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

BOOL CloseSW80SSDHandle()
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


BOOL SW80CreateHandle(int device )
{
    SP_DEVICE_INTERFACE_DATA            interfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    interfaceDetailData = NULL;
    BOOL                                status;
    UCHAR                               outBuf[512];
    ULONG                               returnedLength;
	
   Devicehandle[device] = CreateFile(
				SSdlistItem[device].DeviceID ,    // device interface name
				GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
				FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
				NULL,                               // lpSecurityAttributes
				OPEN_EXISTING,                      // dwCreationDistribution
				0,                                  // dwFlagsAndAttributes
				NULL                                // hTemplateFile
				);
   if (Devicehandle[device] == INVALID_HANDLE_VALUE) 
	    return 0;

   CString str;

	//scsi 接口							
   InterfaceType[device]=1;


   ULONGLONG vendorlba;
   vendorlba=SW80GetVendorLBA(device);

   if( SW80DistSSD(device)==0 )
   {
	   CloseHandle(Devicehandle[device]);
	   Devicehandle[device] = INVALID_HANDLE_VALUE;
	   return 0;
   }	

   
#ifdef _PROTECT

    int fwnum,i;
	CString sver;
	
	BYTE databuf[512];
	if(IDFY_IDE(SSDID,databuf)==0)
		AfxMessageBox("Identify failed !");

	for (i=23;i<27;i=i+1)
	{
		sver+=databuf[2*i+1];
		sver+=databuf[2*i];
	}
	sver.TrimRight();
	fwnum=atol(sver.Left(4));
	//只有FW版本大于1121时，才支持SW80CheckGlobal命令
	if(fwnum<1121)
	{
	   free (interfaceDetailData);
	   CloseHandle(Devicehandle[SSDID]);
	   Devicehandle[SSDID] = INVALID_HANDLE_VALUE;
	   return 1;	
	}
	else
	{
		vendorlba=SW80GetVendorLBA(SSDID);
		if(SW80CheckGlobal(SSDID,vendorlba)!=1)  
		{
			   free (interfaceDetailData);
			   CloseHandle(Devicehandle[SSDID]);
			   Devicehandle[SSDID] = INVALID_HANDLE_VALUE;
			   return 1;	
		}
	}
		
#endif
	
#ifdef _RESTORE

    int fwnum,i;
	CString sver;
	
	BYTE databuf[512];
	if(IDFY_IDE(SSDID,databuf)==0)
		AfxMessageBox("Identify failed !");

	for (i=23;i<27;i=i+1)
	{
		sver+=databuf[2*i+1];
		sver+=databuf[2*i];
	}
	sver.TrimRight();
	fwnum=atol(sver.Left(4));
	//只有FW版本大于1121时，才支持SW80CheckGlobal命令
	if(fwnum<1121)
	{
	   free (interfaceDetailData);
	   CloseHandle(Devicehandle[SSDID]);
	   Devicehandle[SSDID] = INVALID_HANDLE_VALUE;
	   return 1;	
	}
	else
	{
		vendorlba=SW80GetVendorLBA(SSDID);
		if(SW80CheckGlobal(SSDID,vendorlba)!=2)  
		{
			   free (interfaceDetailData);
			   CloseHandle(Devicehandle[SSDID]);
			   Devicehandle[SSDID] = INVALID_HANDLE_VALUE;
			   return 1;	
		}
	}
		
#endif

#ifndef _PROTECT

   //如果是SW80 SSD，进入VENDER 模式
   	unsigned char result;
   	if(SW80ReadCMDStatus(device,vendorlba,&result)>0)
	{
		if(SW80SSDVenderMode(device,vendorlba,1)==0)
		{
			AddInfo(SSDID,"Enter VND failed in CreatHandle!",3);
			return 0;
		}
		else
			AddInfo(SSDID,"Enter VND successfully in CreatHandle!",1);
	}

#endif

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
			//	FILE_FLAG_WRITE_THROUGH,
				NULL                                // hTemplateFile
				);
   if (Devicehandle[SSDID] == INVALID_HANDLE_VALUE) 
	    return 0;

   CString str;
   str.Format("%s",interfaceDetailData->DevicePath);
  // AddInfo(SSDID,str,3);

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
   if( adpDesc->BusType==1 || adpDesc->BusType==10 )				//scsi 接口							
		InterfaceType[SSDID]=1;
   else if(adpDesc->BusType==7 )		//USB接口
		InterfaceType[SSDID]=7;
   else
		InterfaceType[SSDID]=0;

   str.Format("Type :%d",adpDesc->BusType);
  // AddInfo(SSDID,str,3);

   ULONGLONG vendorlba;
   vendorlba=SW80GetVendorLBA(SSDID);

   //if(strstr(interfaceDetailData->DevicePath,"gucssd")==NULL &&  SW80DistSSD(SSDID)==0 )
   if( SW80DistSSD(SSDID)==0 )
   {
       free (interfaceDetailData);
	   CloseHandle(Devicehandle[SSDID]);
	   Devicehandle[SSDID] = INVALID_HANDLE_VALUE;
	   return 1;
   }	

   
#ifdef _PROTECT

    int fwnum,i;
	CString sver;
	
	BYTE databuf[512];
	if(IDFY_IDE(SSDID,databuf)==0)
		AfxMessageBox("Identify failed !");

	for (i=23;i<27;i=i+1)
	{
		sver+=databuf[2*i+1];
		sver+=databuf[2*i];
	}
	sver.TrimRight();
	fwnum=atol(sver.Left(4));
	//只有FW版本大于1121时，才支持SW80CheckGlobal命令
	if(fwnum<1121)
	{
	   free (interfaceDetailData);
	   CloseHandle(Devicehandle[SSDID]);
	   Devicehandle[SSDID] = INVALID_HANDLE_VALUE;
	   return 1;	
	}
	else
	{
		vendorlba=SW80GetVendorLBA(SSDID);
		if(SW80CheckGlobal(SSDID,vendorlba)!=1)  
		{
			   free (interfaceDetailData);
			   CloseHandle(Devicehandle[SSDID]);
			   Devicehandle[SSDID] = INVALID_HANDLE_VALUE;
			   return 1;	
		}
	}
		
#endif
	
#ifdef _RESTORE

    int fwnum,i;
	CString sver;
	
	BYTE databuf[512];
	if(IDFY_IDE(SSDID,databuf)==0)
		AfxMessageBox("Identify failed !");

	for (i=23;i<27;i=i+1)
	{
		sver+=databuf[2*i+1];
		sver+=databuf[2*i];
	}
	sver.TrimRight();
	fwnum=atol(sver.Left(4));
	//只有FW版本大于1121时，才支持SW80CheckGlobal命令
	if(fwnum<1121)
	{
	   free (interfaceDetailData);
	   CloseHandle(Devicehandle[SSDID]);
	   Devicehandle[SSDID] = INVALID_HANDLE_VALUE;
	   return 1;	
	}
	else
	{
		vendorlba=SW80GetVendorLBA(SSDID);
		if(SW80CheckGlobal(SSDID,vendorlba)!=2)  
		{
			   free (interfaceDetailData);
			   CloseHandle(Devicehandle[SSDID]);
			   Devicehandle[SSDID] = INVALID_HANDLE_VALUE;
			   return 1;	
		}
	}
		
#endif

#ifndef _PROTECT

   //如果是SW80 SSD，进入VENDER 模式
   	unsigned char result;
   	if(SW80ReadCMDStatus(SSDID,vendorlba,&result)>0)
	{
		if(SW80SSDVenderMode(SSDID,vendorlba,1)==0)
		{
			AddInfo(SSDID,"Enter VND failed",3);
			return 0;
		}
		else
			AddInfo(SSDID,"Enter VND successfully",1);
	}

#endif


	SSDID++;

    free (interfaceDetailData);
    return 1;
}

// DoIDENTIFY
// FUNCTION: Send an IDENTIFY command to the drive
// bDriveNum = 0-3
// bIDCmd = IDE_ATA_IDENTIFY or IDE_ATAPI_IDENTIFY

BOOL DoIDENTIFY (HANDLE hPhysicalDriveIOCTL, PSENDCMDINPARAMS pSCIP,
                 PSENDCMDOUTPARAMS pSCOP, BYTE bIDCmd, BYTE bDriveNum,
                 PDWORD lpcbBytesReturned)
{
	// Set up data structures for IDENTIFY command.
	pSCIP -> cBufferSize = IDENTIFY_BUFFER_SIZE;
	pSCIP -> irDriveRegs.bFeaturesReg = 0;
	pSCIP -> irDriveRegs.bSectorCountReg = 1;
	pSCIP -> irDriveRegs.bSectorNumberReg = 1;
	pSCIP -> irDriveRegs.bCylLowReg = 0;
	pSCIP -> irDriveRegs.bCylHighReg = 0;
	
	// Compute the drive number.
	pSCIP -> irDriveRegs.bDriveHeadReg = 0xE0| 0;//0xA0 | ((bDriveNum & 1) << 4);
	
	// The command can either be IDE identify or ATAPI identify.
	pSCIP -> irDriveRegs.bCommandReg = bIDCmd;
	pSCIP -> bDriveNumber = bDriveNum;
	pSCIP -> cBufferSize = IDENTIFY_BUFFER_SIZE;
	
	return ( DeviceIoControl (hPhysicalDriveIOCTL, DFP_RECEIVE_DRIVE_DATA,
		(LPVOID) pSCIP,
		sizeof(SENDCMDINPARAMS) - 1,
		(LPVOID) pSCOP,
		sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
		lpcbBytesReturned, NULL) );
}

char *ConvertToString (USHORT diskdata [256], int firstIndex, int lastIndex)
{
	static char string [1024];
	int index = 0;
	int position = 0;
	
	//  each integer has two characters stored in it backwards
	for (index = firstIndex; index <= lastIndex; index++)
	{
		//  get high byte for 1st character
		string [position] = (char) (diskdata [index] / 256);
		position++;
		
		//  get low byte for 2nd character
		string [position] = (char) (diskdata [index] % 256);
		position++;
	}
	
	//  end the string 
	string [position] = '\0';
	
	//  cut off the trailing blanks
	for (index = position - 1; index > 0 && ' ' == string [index]; index--)
		string [index] = '\0';
	
	return string;
}

CString GetDriveGeometry(HANDLE hDevice)
{
    BOOL bResult;                // results flag
    DWORD junk;                  // discard results
	DISK_GEOMETRY pdg;            // disk drive geometry structure
    ULONGLONG DiskSize;          // size of the drive, in bytes
	CString str("(0MB)");
	
    if (hDevice == INVALID_HANDLE_VALUE) // cannot open the drive
    {
        return (str);
    }
	int ii=IOCTL_DISK_GET_DRIVE_GEOMETRY;
    bResult = DeviceIoControl(hDevice,    // device to be queried
        IOCTL_DISK_GET_DRIVE_GEOMETRY,    // operation to perform
		NULL, 0,              // no input buffer
		&pdg, sizeof(pdg),    // output buffer
		&junk,                // # bytes returned
		(LPOVERLAPPED) NULL);  // synchronous I/O

	if (bResult)
	{
		DiskSize = pdg.Cylinders.QuadPart * (ULONG)pdg.TracksPerCylinder *
			(ULONG)pdg.SectorsPerTrack * (ULONG)pdg.BytesPerSector;
		str.Format("( %I64d (MB) )",DiskSize / (1024 * 1024));
	}

    return (str);
}

// 对磁盘扇区数据的写入
BOOL WriteSectors(int device, LONGLONG dwStartSector, WORD wSectors, BYTE *lpSectBuff) 
{
	LARGE_INTEGER li;
	BOOL bRet;
	li.QuadPart = dwStartSector*512;
	SetFilePointer(Devicehandle[device], li.LowPart, &li.HighPart, FILE_BEGIN);
	DWORD dwCB;

	//以PIO的方式写
	if(PIOFlag[device]==1)
	{
		IDEREGS regs;
		regs.bFeaturesReg		= 0;	// send  command
		regs.bSectorCountReg	= wSectors;	
		regs.bSectorNumberReg	= BYTE( (dwStartSector >> 0) & 0xFF);
		regs.bCylLowReg			= BYTE( (dwStartSector >> 8) & 0xFF);
		regs.bCylHighReg		= BYTE( (dwStartSector >> 16) & 0xFF);	
		regs.bDriveHeadReg		= 0x40 | BYTE( (dwStartSector >> 24) & 0x0F);	// LBA mode
		regs.bCommandReg = 0x30;

		bRet=ata_pass_through_ioctl_pio(Devicehandle[device], &regs, lpSectBuff, 512 * wSectors, true); 	
		return bRet;
	}

/*	if(	InterfaceType[device]==1)
	{
		bRet = ata_via_scsi_write_dma(device, 0, dwStartSector, wSectors, lpSectBuff);
	}
	else*/ 
	if(InterfaceType[device]==7)
		bRet = ata_via_scsi_write(device, 0, dwStartSector, wSectors, lpSectBuff);
	else
	{
		bRet = WriteFile(Devicehandle[device], lpSectBuff, 512 * wSectors, &dwCB, NULL);
		if(bRet==0)
			bRet = ata_via_scsi_write_dma(device, 0, dwStartSector, wSectors, lpSectBuff);
	}

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
	
	//以PIO的方式读
	if(PIOFlag[device]==1)
	{
		IDEREGS regs;
		regs.bFeaturesReg		= 0;	// send  command
		regs.bSectorCountReg	= wSectors;	
		regs.bSectorNumberReg	= BYTE( (dwStartSector >> 0) & 0xFF);
		regs.bCylLowReg			= BYTE( (dwStartSector >> 8) & 0xFF);
		regs.bCylHighReg		= BYTE( (dwStartSector >> 16) & 0xFF);	
		regs.bDriveHeadReg		= 0x40 | BYTE( (dwStartSector >> 24) & 0x0F);	// LBA mode
		regs.bCommandReg = 0x20;

		bRet=ata_pass_through_ioctl_pio(Devicehandle[device], &regs, lpSectBuff, 512 * wSectors, false); 	
		return bRet;
	}

/*	if(	InterfaceType[device]==1)
	{
		bRet = ata_via_scsi_read_dma(device, 0, dwStartSector, wSectors, lpSectBuff);
	}
	else*/ 
	if(InterfaceType[device]==7)
		bRet = ata_via_scsi_read(device, 0, dwStartSector, wSectors, lpSectBuff);
	else
	{
		bRet = ReadFile(Devicehandle[device], lpSectBuff, 512 * wSectors, &dwCB, NULL);
		if(bRet==0)
			bRet = ata_via_scsi_read_dma(device, 0, dwStartSector, wSectors, lpSectBuff);
	}

	return bRet;
}

// 对磁盘数据的写入
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
// 对磁盘数据的读取
{
	LARGE_INTEGER li;
	BOOL bRet;
	li.QuadPart = dwStartSector*512;
	SetFilePointer(Devicehandle[device], li.LowPart, &li.HighPart, FILE_BEGIN);
	DWORD dwCB;	

	bRet = ReadFile(Devicehandle[device], lpSectBuff, 512 * wSectors, &dwCB, NULL);
	return bRet;
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
//	ret=WriteSectors(device,0xF800000,1,buffer);
	ret=WriteSectors(device,0xF800000,1,buffer);

	if(ret==0)
		return ret;
//	Sleep(50);
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

BOOL ReadFlashUID(int device,BOOL fileflag)
{
	CString str,struid[8];
	BYTE buf[512];
	int ldie;
	BYTE lchannel;
	int chip=0;
	int i;
	
	for(i=0;i<8;i++)
	{
		struid[i].Empty();
	}

	for(lchannel = 0; lchannel < channel; lchannel++)
	{
		for(ldie = 0; ldie < 2; ldie++)
		{
			if(SW80ReadFlashUID(device,lchannel,ldie,buf))
			{
				if(buf[0]!=0)
				{
					for(i=0;i<16;i++)
					{
						str.Format(" %02X ", buf[i]);
						struid[ldie+2*lchannel]+=str;
					}
					str.Format("Read UID: Channel:%d Chip:%d %s",lchannel, ldie, struid[ldie+2*lchannel]);
					AddInfo(device,str,1);
					str.Empty();
					chip++;
				}
			}
			else
			{
				str.Format("Read UID: Channel:%d Chip:%d failed",lchannel, ldie);
				AddInfo(device,str,3);
				return 0;
			}
		}
	}

	
	FILE *fp;
	char output[128];	
	int ret;	
	CString filename;
	if(fileflag)
	{
		filename ="UID"+BadBlockFilename[device]+".bck";
		char file[256];
		GetModuleFileName(NULL,file,256); 
		//Scan a string for the last occurrence of a character.
		(strrchr(file,'\\'))[1] = 0; 
		strcat(file,"BadblockFile\\");
		strcat(file,filename);

		//判断同目录下是否存在同名的坏块文件
		ret=GetFileAttributes(file);
		if(ret>0)
		{
			AfxMessageBox("当前目录下存在同名的FLASHUID文件，请重新选择文件名保存原始坏块文件!");
			CFileDialog FileDlg(FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
								"TextFiles (*.bck)|*.bck||");
			GetModuleFileName(NULL,file,256); 
			(strrchr(file,'\\'))[1] = 0; 
			FileDlg.m_ofn.lpstrInitialDir=file;   
			if (FileDlg.DoModal() == IDOK)			//打开文件对话框
				filename = FileDlg.GetPathName();	//得到文件路经
			else 
				return 0;
			
			if( filename.Right(4) != ".bck")
				filename += ".bck";	

			strcpy(file,filename);
		}
		


		fp = fopen(file, "w");
		if(fp==NULL)
		{
			AddInfo(device,"Open UID file fail!",3);
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
		str=manufacture+" ";
		if(flash_type[device].isMLC==0)
		{
			str+="SLC\r\n";
		}
		else
		{
			str+="MLC\r\n";
		}
		fprintf(fp,"%s",str);
		
		str.Format("CHANNEL = %d\r\n",channel);//channel number
		fprintf(fp,"%s",str);

		sprintf(output,"CHIP = %d\r\n",chip/4);//die number
		str=output;
		fprintf(fp,"%s",str);
		for(lchannel = 0; lchannel < channel; lchannel++)
		{
			for(i=0;i<2;i++)
			{
				if(struid[i+2*lchannel].GetLength()>0)
				{
					sprintf(output,"CHIP %d CHANNEL %d UID: %s\r\n",i,lchannel,struid[i+2*lchannel]);
					str=output;
					fprintf(fp,"%s",str);
				}
			}
		}

		str="End!";
		fprintf(fp,"%s",str);
		fclose(fp);
	}

	return  1;
}

BOOL SW80ReadFlashUID(int device,int channel,int die,BYTE *UID)
{
	BYTE buffer[512],readbuf[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08R_MAD_SI_K");
	int i;

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=SW80_CMD_VND_SUB_READFLASHUID;
	buffer[16]=die;
	buffer[17]=channel;
	buffer[20]=SW80_CMD_VND;
	ret=WriteSectors(device,0xF800000,1,buffer);
	if(ret==0)
		return ret;
//	Sleep(50);
	ret=ReadSectors(device,0xF800000,1,readbuf);
	if(ret==0)
		return ret;

	if(readbuf[0]=='F'&&readbuf[1]=='I')
	{
		for(i=0;i<16;i++)
		{
			UID[i]=readbuf[23+i];
		}
	}
	else
		return 0;

	return 1;
}


BOOL SW80Findbadblock4CH(int device,int dienum)
/*
Function:同时查找4个channel的原始坏块
返回值：1代表能执行查找动作，0表示不能支持，这意味不能继续查找
输入：dienum，物理die

*/
{
	BYTE buffer[512],readbuf[512*5];
	BOOL ret;
	int i,j,badblocknum=0,badblockaddr=0;
	CString str;

	memset(buffer,0,512);
	memset(readbuf,0,512*5);
	strcpy((char*)buffer,"WS08R_MAD_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=SW80_CMD_VND_SUB_FINDBLOCK4CH;
	buffer[16]=dienum;
	buffer[20]=SW80_CMD_VND;


	ret=WriteSectors(device,0xF800000,1,buffer);
	if(ret==0)
		return ret;

	ret=ReadSectors(device,0xF800000,5,readbuf);
	if(ret==0)
		return ret;

	if(readbuf[0]=='F'&&readbuf[1]=='I')
	{
		for(i=0;i<channel;i++)
		{
			badblocknum=readbuf[15+i];
		//	badblocknum=0;
			str.Format("channel %d die %d 坏块个数 %d\r\n", i,dienum, badblocknum);
			AddInfo(device,str,1);	
			for(j=0;j<badblocknum;j++)
			{
				if(j>=255)
					break;
				badblockaddr=readbuf[512*i+512+2*j] | readbuf[512*i+512+2*j+1]<<8;
				SW80UpdateBadblock(device,i,dienum,badblockaddr);
			}
		}

		
	}
	else
		return 0;

	return 1;
}


BOOL SW80Eraseblock4CH(int device,BYTE *blockaddr,BYTE *status)
/*
Function:同时擦除4个channel的block
返回值：1代表能执行擦除动作，0表示不能支持，这意味不能继续擦除
输入：blockaddr，存储4个物理block地址，一共16个byte
输出：status,存储擦除block的状态，一共8个byte
*/
{
	BYTE buffer[512],readbuf[512];
	BOOL ret;
	int i;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08R_MAD_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=SW80_CMD_VND_SUB_ERASEBLOCK4CH;
	buffer[20]=SW80_CMD_VND;
	for(i=0;i<16;i++)
		buffer[i+21]=blockaddr[i];

	ret=WriteSectors(device,0xF800000,1,buffer);
	if(ret==0)
		return ret;

	ret=ReadSectors(device,0xF800000,1,readbuf);
	if(ret==0)
		return ret;

	if(readbuf[0]=='F'&&readbuf[1]=='I')
	{
		for(i=0;i<8;i++)
			status[i]=readbuf[i+128];
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

/*
查找原始坏块，需要读10个sector,第1个sector包含返回的状态，后面8个sector是数据，第10个sector是spare区

*/
BOOL SW80CheckPage(int device,int channel,int die,DWORD page,BOOL bypassflag,BYTE *databuf)
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

	ret=ReadSectors(device,0xF800000,10,databuf);
	if(ret==0)
		return ret;

	if(databuf[0]=='F'&&databuf[1]=='I')
	{
		if(bypassflag)
			return 1;
		else
			ret=databuf[14];
	}
	else
		return 0;

	return !ret;
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
	if(flash_type[device].size_page==0)
		flash_type[device].size_page=2;
	ret=ReadSectors(device,0xF800000,flash_type[device].size_page*2+1,databuf);
//	ret=ReadSectors(device,0xF800000,flash_type[device].size_page*2+3,databuf);

	if(ret==0)
		return ret;

	if(databuf[0]=='F'&&databuf[1]=='I')
	{
		if(bypassflag)
			return 1;
		else
		//	ret=databuf[14];
			ret=0;	//由于GUC的硬件问题，8K的FLASH容易误报ECC错误，所以不报ECC错误
	}
	else
		return 0;

	return !ret;
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
	if(ret==0)
		return ret;

	
	for(i=256;i<270;i++)
		str+=buffer[i];

	if(str.Compare("WS08R_MAD_SI_K")==0)
		return 1;
	else
		return 0;
}

int SW80DistSSD(int device)
/*
根据IDFY的值判断是不是SW80 SSD
返回值
1: SW80 RAM DISK
2: SW80 SSD(量产完成后的盘)
0: 非SW80 系列
*/
{	unsigned char SSDflag[32]="SOLIWARE-80",RAMflag[32]="SW80_RAM_DISK_V1",tmp[32],GUCFlag[32]="GUCSSD";
	BYTE databuf[512];
	CString model;
	int i;
	if(IDFY_IDE(device,databuf)==0)
		return 0;
	
	model.Empty();
	for(i=27;i<47;i++)
	{
		model+=databuf[2*i+1];
		model+=databuf[2*i];
	}
	model.TrimRight();

	if(memcmp(model,GUCFlag,6)==0)
	{
		SSdlistItem[device].DriveStatus=1;	
		return 1;
	}

	if(memcmp(model,RAMflag,16)==0)
	{
		SSdlistItem[device].DriveStatus=2;
		return 1;
	}

	for(i=0;i<16;i++)
		tmp[i]=databuf[258+i];

	if(memcmp(tmp,SSDflag,11)==0)
	{
		SSdlistItem[device].DriveStatus=3;	
		return 1;
	}

	SSdlistItem[device].DriveStatus=0;
	return 0;
}

BOOL SW80PageWrite(int device,int channel,int die,DWORD page,BYTE *oob,BYTE *databuf)
{
	BYTE buffer[512*18],readbuf[512];
	BOOL ret;
	memset(buffer,0,512*18);
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

	memcpy(buffer+512,databuf,flash_type[device].size_page*1024);
	//写命令包和数据包
	ret=WriteSectors(device,0xF800000,flash_type[device].size_page*2+1,buffer);
	if(ret==0)
		return ret;
	Sleep(50);

	ret=ReadSectors(device,0xF800000,1,readbuf);
	if(ret==0)
		return ret;

	if(readbuf[0]=='F'&&readbuf[1]=='I')
		ret=(readbuf[14] & 0x1);
	else
		return 0;

	return !ret;
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

int SLWFlashIDPos(int device)
{
	int i,j;

	for(i=0;i<64;i++)
	{
		for(j=0;j<32;j++)
		{
			if(flash_id[device][i][0]==FlashEndurance[j].ID[0] && flash_id[device][i][1]==FlashEndurance[j].ID[1] && flash_id[device][i][2]==FlashEndurance[j].ID[2] && flash_id[device][i][3]==FlashEndurance[j].ID[3]) 
				return j;		
		}
	}
}

BOOL Read_Rebuild(int device,BYTE *buf)
{
	IDEREGS regs;
	CString str;
	regs.bFeaturesReg=SLW_CMD_VND_SUB_REBUILD;
	regs.bSectorCountReg=0x0;
	regs.bSectorNumberReg=0;
	regs.bCylLowReg=0;
	regs.bCylHighReg=0x0;	
	regs.bDriveHeadReg=0x0;
	regs.bCommandReg=0xFD;		// Vendor Command
		
	DWORD  cbBytesReturned = 0;
	
	if( !ata_pass_through_ioctl_pio(Devicehandle[device], &regs, buf, 512, false) )
		return FALSE;

	if ( regs.bCommandReg!= 0x50 )//success
		return FALSE;
	
	if( regs.bFeaturesReg <101)
	{		
		str.Format("当前Rebuild状态  %d!\r\n",regs.bFeaturesReg);
		AddInfo(device,str,1);
	}

	return TRUE;
}

BOOL SetRestoreArgs(int device,BYTE *buf)
{
	IDEREGS regs;
	CString str;
	regs.bFeaturesReg=0x20;
	regs.bSectorCountReg=0x1;
	regs.bSectorNumberReg=0;
	regs.bCylLowReg=0x1;
	regs.bCylHighReg=0x0;	
	regs.bDriveHeadReg=0x0;
	regs.bCommandReg=0xFF;		// Vendor Command
		
	DWORD  cbBytesReturned = 0;
	
	if( !ata_pass_through_ioctl_pio(Devicehandle[device], &regs, buf, 512, true) )
		return FALSE;

	if ( regs.bCommandReg!= 0x50 )//success
		return FALSE;
	
	return TRUE;
}

BOOL GetRestoreArgs(int device,BYTE *buf)
{
	IDEREGS regs;
	CString str;
	regs.bFeaturesReg=0x20;
	regs.bSectorCountReg=0x1;
	regs.bSectorNumberReg=0;
	regs.bCylLowReg=0x1;
	regs.bCylHighReg=0x80;	
	regs.bDriveHeadReg=0x00;
	regs.bCommandReg=0xFE;		// Vendor Command
		
	DWORD  cbBytesReturned = 0;
	
	if( !ata_pass_through_ioctl_pio(Devicehandle[device], &regs, buf, 512, false) )
		return FALSE;

	if ( regs.bCommandReg!= 0x50 )//success
		return FALSE;
	return TRUE;
}

BOOL IDFYbyDoIDENTIFY(int device,BYTE *idfy)
{
	int done = FALSE;
	int drive = 0;
	INT16U  baseAddress = 0;   //  Base address of drive controller
	BYTE IdOutCmd [sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];
	char HardDriveSerialNumber [1024];

	GETVERSIONOUTPARAMS VersionParams;
	DWORD               cbBytesReturned = 0;
	HANDLE hPhysicalDriveIOCTL = Devicehandle[device];
	
	// Get the version, etc of PhysicalDrive IOCTL
	memset ((void*) &VersionParams, 0, sizeof(VersionParams));
	hPhysicalDriveIOCTL=Devicehandle[device];
	if ( ! DeviceIoControl (hPhysicalDriveIOCTL, DFP_GET_VERSION,
		NULL, 
		0,
		&VersionParams,
		sizeof(VersionParams),
		&cbBytesReturned, NULL) )
	{         
	}

	BYTE             bIDCmd = 0;   // IDE or ATAPI IDENTIFY cmd
	SENDCMDINPARAMS  scip;

	// Now, get the ID sector for all IDE devices in the system.
	// If the device is ATAPI use the IDE_ATAPI_IDENTIFY command,
	// otherwise use the IDE_ATA_IDENTIFY command
	bIDCmd = (VersionParams.bIDEDeviceMap >> drive & 0x10) ? \
IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;
	
	memset (&scip, 0, sizeof(scip));
	memset (IdOutCmd, 0, sizeof(IdOutCmd));
	
	if ( DoIDENTIFY (hPhysicalDriveIOCTL, 
		&scip, 
		(PSENDCMDOUTPARAMS)&IdOutCmd, 
		(BYTE) bIDCmd,
		(BYTE) drive,
		&cbBytesReturned))
	{
		USHORT diskdata [256];
		int ijk = 0;
		USHORT *pIdSector = (USHORT *)
			((PSENDCMDOUTPARAMS) IdOutCmd) -> bBuffer;

		
		for (ijk = 0; ijk < 256; ijk++)
		{
			idfy [2*ijk] = pIdSector [ijk];
			idfy [2*ijk+1] = pIdSector [ijk]>>8;
		}

		return 1;
	}
	else
		return 0;


}

BOOL IDFY_IDE(int device,BYTE *idfy)
{
	BOOL ret;
	IDEREGS regs;

	if(	InterfaceType[device]==7)
	{
		ret=DoIdentifyDeviceSat(device,0,idfy);
		return ret;
	}
	if(IDFYbyDoIDENTIFY(device,idfy)>0)
	   return 1;
	//是SCSI设备时，先发IDFYbyDoIDENTIFY，如果不成功，再试SCSI_PASS_THROUGH
	else if(InterfaceType[device]==1 || InterfaceType[device]==10)        
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
		
		if( !ata_pass_through_ioctl_pio(Devicehandle[device], &regs, idfy, 512, false) )
		{
			return FALSE;
		}

		if ( regs.bCommandReg!= 0x50 )//success
		{
		//	return FALSE;
			;
		}
		
		return TRUE;
	}	
}

BOOL ata_pass_through_ioctl_pio(HANDLE hdevice, IDEREGS * regs, unsigned char * data, unsigned long datasize, bool IsWR)
{ 
	typedef struct 
	{
		ATA_PASS_THROUGH_EX apt;
		UCHAR ucDataBuf[64*1024];
	} ATA_PASS_THROUGH_EX_WITH_BUFFERS;
	
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

	CString str;
	ASSERT(sizeof(ab.apt.CurrentTaskFile) == sizeof(IDEREGS));
	IDEREGS * ctfregs = (IDEREGS *)ab.apt.CurrentTaskFile;
	*ctfregs = *regs;
	
	OVERLAPPED wrOverlapped;
	ZeroMemory(&wrOverlapped,sizeof(wrOverlapped));
	wrOverlapped.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	ResetEvent(wrOverlapped.hEvent);

	DWORD num_out;
	if (!DeviceIoControl(hdevice, IOCTL_ATA_PASS_THROUGH,
		&ab, size, &ab, size, &num_out, &wrOverlapped)) 
	{
		if (GetLastError() == ERROR_IO_PENDING) 
		{
			if( WAIT_TIMEOUT == WaitForSingleObject(wrOverlapped.hEvent,1000))  //wait for 5s 
			{
				return FALSE;
			}
			GetOverlappedResult(hdevice,&wrOverlapped,&num_out,TRUE);		
			if(num_out != size)
			{
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

	//	return FALSE;
	}
	
	// Check and copy data
	if (!IsWR && datasize) 
	{
		memcpy(data, ab.ucDataBuf, datasize);
	}
	
	return TRUE;
}

BOOL SW80FindBadblock(int device)
{
	CString str;
	unsigned char DATABuffer[512];

	int i;
	int j;
		
	int m_datasize=512;	//512
	int datasize=512;	//512	
	int sectorcount=1;	//1 sector

	
	for(i=0;i<datasize;i++)
	{
		DATABuffer[i]=0xff;		
	}


	for(i=0;i<channel*gldie;i++)
		xblock[device][i]=0;

	for(i=0;i<grdie[device];i++)
	{
		if(SW80Findbadblock4CH(device,DIEMAP[device][i])==0)
			return 0;
	}

	int errorflag=0;	
	for(i=0;i<2;i++)
	{
		for(j=0;j<xblock[device][i];j++)
		{
			if(badblock[device][i][j]==0)
			{
				errorflag=1;				
				str.Format("Channel %d die %d  block 0 块为坏块\r\n",(i/gldie),DIEMAP[device][i%gldie]);
				AddInfo(device,str,3);					
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
			if(a>(flash_type[device].blocknum*Resratio/(2*128*flash_type[device].plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d plane 0 坏块>50\r\n",(i/gldie),DIEMAP[device][i%gldie]);
				AddInfo(device,str,3);
					
			}
			if(b>(flash_type[device].blocknum*Resratio/(2*128*flash_type[device].plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d plane 1 坏块>50\r\n",(i/gldie),DIEMAP[device][i%gldie]);
				AddInfo(device,str,3);
					
			}
			if(c>(flash_type[device].blocknum*Resratio/(2*128*flash_type[device].plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d  plane 0 坏块>50\r\n",(i/gldie),DIEMAP[device][i%gldie]);
				AddInfo(device,str,3);
					
			}
			if(d>(flash_type[device].blocknum*Resratio/(2*128*flash_type[device].plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d  plane 1 坏块>50\r\n",(i/gldie),DIEMAP[device][i%gldie]);
				AddInfo(device,str,3);
					
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
			if(a>(flash_type[device].blocknum*Resratio/(2*128*flash_type[device].plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d plane 0 坏块>50\r\n",(i/gldie),DIEMAP[device][i%gldie]);
				AddInfo(device,str,3);
					
			}
			if(b>(flash_type[device].blocknum*Resratio/(2*128*flash_type[device].plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d plane 1 坏块>50\r\n",(i/gldie),DIEMAP[device][i%gldie]);
				AddInfo(device,str,3);
					
			}
			if(c>(flash_type[device].blocknum*Resratio/(2*128*flash_type[device].plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d  plane 0 坏块>50\r\n",(i/gldie),DIEMAP[device][i%gldie]);
				AddInfo(device,str,3);
					
			}
			if(d>(flash_type[device].blocknum*Resratio/(2*128*flash_type[device].plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d  plane 1 坏块>50\r\n",(i/gldie),DIEMAP[device][i%gldie]);
				AddInfo(device,str,3);
					
			}
		}		
	}
	if(errorflag==1)
	{				
		return 0;
	}

	return 1;
}


BOOL SW80GetDeviceInfo(int device,int Initnum)
{
	int i=0;
	int j=0;

	CString str;
	if(Initnum==0)
	{
		if(IniMode==1)
		{
			if(SW80FindBadblock(device)==0)
			{
				AddInfo(device,"****GetDeviceInfo FindBadblock failed!!!",3);		
				return 0;
			}
		
			if(WriteResult(device,0)==0)
			{
				AddInfo(device,"****Save bad block table failed!!!",3);		
				return 0;
			}	
			ReadFlashUID(device,1);

					
		}
		else if(IniMode==2)	
		{
			if(SW80ReadBadblock(device)==0)
			{
				AddInfo(device,"****GetDeviceInfo Read BadBlockTables failed!!!",3);		
				return 0;
			}	
			if(WriteResult(device,0)==0)
			{
				AddInfo(device,"****Save bad block table failed!!!",3);		
				return 0;
			}		
		}
		else if(IniMode==3)	
		{
			/*擦除所有block，找出坏块*/
			SW80EraseAll(device);
			if(WriteResult(device,0)==0)
			{
				AddInfo(device,"****Save bad block table failed!!!",3);		
				return 0;
			}	
		}
		else if(IniMode==0)	//从文件初始化
		{
			if(ReadResult(device)==0)
			{
				AddInfo(device,"****GetDeviceInfo ReadResult failed!!!",3);		
				return 0;
			}	
			SW80EraseAll(device);/*擦除所有块*/
		}
		else
		{
			AddInfo(device,"****Please select the mode for bad block !!!",3);							
			return 0;
		}
	}
	
	return 1;
}

/*
说明：检查当前BLOCK是否是坏块。
*/
BOOL VerifyBadblock(int device,int channelnum,int dienume,int blocknum)
{
	int loc,j;
	loc=channelnum*gldie+dienume;
	for(j=0;j<xblock[device][loc];j++)
	{
		if(blocknum==badblock[device][loc][j])
		{
			return 1;
		}
	}

	return 0;
}

void Setblockaddr(int device,int die,int block,BYTE *blockaddr)
{
	int i;
	for(i=0;i<channel;i++)
	{
		//if(VerifyBadblock(device,i,die,block))
		//	blockaddr[4*i]=0xff;
		//else      由于需要手动添加新增坏块，所有块都擦除（手动添加的坏块如果没有擦除，里面可能有数据，FW会把它当成好块）

			blockaddr[4*i]=i;

		blockaddr[4*i+1]=die;
		blockaddr[4*i+2]=block;
		blockaddr[4*i+3]=block>>8;
	}
}

void SW80EraseAll(int device)
{
	int idie;
	DWORD dblock = 4096,num;
	DWORDLONG LBAADD = 0;
	CString str;
	int loc=0,i;
	BYTE blockaddr[16],status[8];

	for (idie = 0; idie <gldie ;idie ++)
	{
#if _INITD 
		pdlgcmd->m_progress.SetPos(idie*80/gldie);

#endif
		if(DIEMAP[device][idie]==65535)
			continue;
		for(dblock = 0;dblock <flash_type[device].blocknum ; dblock++)
		{

			if( (DIEMAP[device][idie]>7&&DIEMAP[device][idie]<16) || (DIEMAP[device][idie]>23&&DIEMAP[device][idie]<32) )
			{
				num=dblock+flash_type[device].blocknum;
				if(flash_type[device].blocknum%1024!=0)
					num=(flash_type[device].blocknum/1024)*2048+dblock;
			}
			else
				num=dblock;
			
			memset(blockaddr,0,16);
			memset(status,0,8);
			Setblockaddr(device,DIEMAP[device][idie],num,blockaddr);
			if(SW80Eraseblock4CH(device,blockaddr,status)==0)
				return ;
			else
			{
				for(i=0;i<4;i++)
				{
					if(status[2*i]=='F' && (status[2*i+1]&0x1)==1 )
						SW80UpdateBadblock(device,i,DIEMAP[device][idie],num);

				}
			}
		}
	}
	
	AddInfo(device,"Erase all block and find new bad block successfully !\r\n",1);
}

void AddInfo(int device,LPCTSTR buf,int level)
/*
输入参数：
buf:调试信息
level:调试信息级别
1：正常
2：警告
3：错误
*/
{
	CString str;
	CString temp;
	
	str.Format("%s\r\n",buf);

#if _INITD 
	int len =pdlgcmd->m_editresult.GetWindowTextLength();
	if(len >= 30000)
	{
		pdlgcmd->m_editresult.GetWindowTextA(temp);
		temp = temp.Right(10000);
		pdlgcmd->m_editresult.SetWindowTextA(temp);
		len = 10000;
	}
	pdlgcmd->m_editresult.SetSel(len,len);
	pdlgcmd->m_editresult.ReplaceSel(str);
	pdlgcmd->m_editresult.SetFocus();
#endif

#if _ADVANCEFEATURE 
	if(pFeaturedlg!=NULL)
	{
		int len =pFeaturedlg->m_editresult.GetWindowTextLength();
		if(len >= 30000)
		{
			pFeaturedlg->m_editresult.GetWindowTextA(temp);
			temp = temp.Right(10000);
			pFeaturedlg->m_editresult.SetWindowTextA(temp);
			len = 10000;
		}
		pFeaturedlg->m_editresult.SetSel(len,len);
		pFeaturedlg->m_editresult.ReplaceSel(str);
		pFeaturedlg->m_editresult.SetFocus();
	}
#endif

#if _MASSINIT 
	FILE *fp;
	char file[256];

    GetModuleFileName(NULL,file,256); 
    //Scan a string for the last occurrence of a character.
    (strrchr(file,'\\'))[1] = 0; 
	strcat(file,"LOG\\");

	strcat(file,SSdlistItem[device].SN);
	strcat(file,".log");
	fp = fopen(file, "a+");
	if(fp==NULL)
	{
		fp = fopen(file, "a+");
		if(fp==NULL)
		{
			return ;
		}
	}
	fprintf(fp,"%s",str);
	fclose(fp);
#endif

#if	_UPDATE

	FILE *fp;
	char file[256];

    GetModuleFileName(NULL,file,256); 
    //Scan a string for the last occurrence of a character.
    (strrchr(file,'\\'))[1] = 0; 

	if(BadBlockFilename[device].Right(4) == ".bck")
		BadBlockFilename[device].TrimRight(".bck");
	
	strcat(file,BadBlockFilename[device]);
	strcat(file,"Updatelog.txt");
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

BOOL ReadResult(int device)
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
		AddInfo(device,"****ReadResult失败!!!",3);
						
		return 0;		
	}		
	strDldFile = dlgLoad.m_ofn.lpstrFile;
	BadBlockFilename[device]=strDldFile;
	SelectBadblockfile=strDldFile;
#endif

#if _MASSINIT 
	char filename[256];
	GetModuleFileName(NULL,filename,256); 
	(strrchr(filename,'\\'))[1] = 0; 
	BadBlockFilename[device]+=".bck";
	strDldFile=filename;
	strDldFile+="BadblockFile\\";
	strDldFile+=BadBlockFilename[device];
#endif

	fp = fopen(strDldFile, "rb");
	if(fp==NULL)
	{
		AfxMessageBox("Open badblock file fail!");
		return 0;
	}
	
	for(i=0;i<8;i++)
	{
		fscanf(fp,"%s ",output);
	}

	fscanf(fp,"%d",&a);

	fscanf(fp,"%s ",output);
	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&a);
	if(a!=grdie[device])
	{
		AddInfo(device,"\r\n\r\n****die设置不正确!!!\r\n\r\n",3);
		fclose(fp);
		return 0;
	}

	fscanf(fp,"%s ",output);
	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&a);
	
	int twoplaneblock;
	for(i=0;i<channel*grdie[device];i++)
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
				AddInfo(device,"\r\n\r\n****文件格式不正确!!!\r\n\r\n",3);	
				fclose(fp);
				return 0;
			}
		}

		loc=lchannel*gldie+ldie;
		fscanf(fp,"%d ",&xblock[device][loc]);
		Totalbadblock[device]+=xblock[device][loc];
		for(j=0;j<xblock[device][loc];j++)
		{
			fscanf(fp,"%d ",&badblock[device][loc][j]);
		}
	}
	fscanf(fp,"%s ",output);
	str=output;	
	if(str!="End!")
	{
		AddInfo(device,"\r\n\r\n****文件格式不正确!!!\r\n\r\n",3);
		fclose(fp);
		return 0;
	}

	fclose(fp);
	return 1;
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

	char file[256];
    GetModuleFileName(NULL,file,256); 
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
		GetModuleFileName(NULL,file,256); 
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
		fp = fopen(file, "w");
		if(fp==NULL)
		{
			AfxMessageBox("Open badblock file fail!");
			return 0;
		}
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

	str=manufacture+" ";
	capacity[device].Format("%dGB",flash_type[device].cap_in_gb);
	str+=capacity[device]+" ";
	if(flash_type[device].isMLC==0)
	{
		str+="SLC\r\n";
	}
	else
	{
		str+="MLC\r\n";
	}
	fprintf(fp,"%s",str);
	
	str.Format("CHANNEL = %d\r\n",channel);//channel number
//	str="CHANNEL = 4\r\n";//channel number
	fprintf(fp,"%s",str);

	sprintf(output,"DIE = %d\r\n",grdie[device]);//die number
	str=output;
	fprintf(fp,"%s",str);

	sprintf(output,"PLANE = %d\r\n",2);//plane number
	str=output;
	fprintf(fp,"%s",str);		
		
	for(i=0;i<gldie*channel;i++)
	{
		k=DIEMAP[device][i%gldie];
		if(k==65535)
			continue;

		sprintf(output,"DIE %d CHANNEL %d BADBLOCK NUMBER: %d\r\n",k,i/gldie,xblock[device][i+k-i%gldie]);
		str=output;
		fprintf(fp,"%s",str);
		for(j=0;j<xblock[device][i+k-i%gldie]&&j<512;j++)
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

BOOL SW80ReadBadblock(int device)
{
	unsigned int i,j,block,ret,k,addr,badblockaddr,temp;
	unsigned char buf[512*18];
	DWORD pagenum;

	for(i=0;i<5120;i++)
		buf[i]=0xFF;

	for(i=0;i<channel;i++)
		for(j=0;j<grdie[device];j++)
		{
			if(DIEMAP[device][j]==0xFFFF)
				continue;
			if( (DIEMAP[device][j]>7&&DIEMAP[device][j]<16) || (DIEMAP[device][j]>23&&DIEMAP[device][j]<32) )
				block=4096;
			else
				block=0;
	
			pagenum=flash_type[device].pages_per_block*block;
			ret = SW80PageRead(device,i,DIEMAP[device][j],pagenum,0,buf);
			if (ret == 0)
			{
				AddInfo(device,"****Read BadblockTable info failed!!!",3);	
				return 0;
			}
			addr=4096;
			for(k=0;k<128;k++)
			{
				temp=buf[addr+8*k+1]&0x80;
				if(temp==0x80)
				{
					badblockaddr=(buf[addr+8*k]|(buf[addr+8*k+1]<<8))&0x3FFF;
					SW80UpdateBadblock(device,i,DIEMAP[device][j],badblockaddr);
				}
			}
			addr=8192;
			for(k=0;k<128;k++)
			{
				temp=buf[addr+8*k+1]&0x80;
				if(temp==0x80)
				{
					badblockaddr=(buf[addr+8*k]|(buf[addr+8*k+1]<<8))&0x3FFF;
					SW80UpdateBadblock(device,i,DIEMAP[device][j],badblockaddr);
				}
			}
		}
	return 1;
}

BOOL SW80UpdateBadblock(int device,int channelnum,int dienume,int blocknum)
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
		if(xblock[device][loc]<512)
			badblock[device][loc][xblock[device][loc]] = blocknum;   //badblock的第二维对应逻辑die,所以要转换为i+m_row-i%gldie
		xblock[device][loc]++;
		Totalbadblock[device]++;
	}

	return 1;
}

BOOL SW80Twoplanebadblock(int device)
{
	int twoplaneblock;
	int lchannel,ldie;
	int loc,j;
	for(lchannel=0;lchannel<channel;lchannel++)
		for(ldie=0;ldie<gldie;ldie++)
		{
			loc=lchannel*gldie+ldie;
			for(j=0;j<xblock[device][loc];j++)
			{
				if(EnableTwoplane==1)
				{
					if(badblock[device][loc][j]%2==0)
					{
						twoplaneblock=badblock[device][loc][j]+1;	//在two plane情况下找与偶数原始坏块对应的奇数原始坏块，例如 0 对 1， 4 对 5
						SW80VerifyBadblock(device,lchannel,ldie,twoplaneblock);
					}
				}

			}
		}

	return 1;

}

BOOL SW80VerifyBadblock(int device,int channelnum,int dienume,int blocknum)
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
	if(flag==1)
	{
		Totalbadblock[device]--;
	}
	return 1;
}

/*在初始化的最后写入全局信息表*/
BOOL SW80WriteGlobalBlock(int device,unsigned char * data,int channelnum,int dienume,int blocknum)
{
	int i;
	BYTE oob[24];
	DWORD page;

	memset(oob,0,24);
	if(SW80Eraseblock(device,channelnum,dienume,blocknum)==0)
	{
		AddInfo(device,"Erase block failed!",3);
		return 0;
	}
	oob[0]=0x51;
	page=blocknum*flash_type[device].pages_per_block;
	if(SW80PageWrite(device,channelnum,dienume,page,oob,data)==0)
		return 0;

	oob[0]=0x51;
	/*写FW,FW的长度最大16个page,fw的起始位置是512×32*/
	for(i=0;i<16;i++)
	{
		page=32/flash_type[device].sectors_per_page+i+blocknum*flash_type[device].pages_per_block;		
		if(SW80PageWrite(device,channelnum,dienume,page,oob,data+512*32+flash_type[device].sectors_per_page*512*i)==0)
			return 0;
	}
	return 1;
}

/*写入访问控制块*/
BOOL SW80WriteAccessCtlBlock(int device)
{
	int i;
	BYTE oob[24],data[32*512];
	DWORD page;
	int channelnum;

	memset(oob,0,24);
	memset(data,0,32*512);
	if(AcessControlblock==0)
		channelnum=1;
	else
		channelnum=0;

	if(SW80Eraseblock(device,channelnum,0,AcessControlblock)==0)
	{
		AddInfo(device,"Erase block failed!",3);
		return 0;
	}
	oob[0]=0x56;
	memcpy(data,(void*)"WD666666",8);
	page=AcessControlblock*flash_type[device].pages_per_block;
	if(SW80PageWrite(device,channelnum,0,page,oob,data)==0)
		return 0;

	return 1;
}

/*读全局信息表*/
BOOL SW80ReadGlobalBlock(int device,unsigned char * data,int channelnum,int dienume,int blocknum)
{
	int i;
	BYTE oob[24],rbuf[512*32];
	DWORD page;
	oob[0]=0x51;
	memset(oob,0,24);
	memset(rbuf,0,512*32);	

	page=blocknum*flash_type[device].pages_per_block;
	if(SW80PageRead(device,channelnum,dienume,page,0,rbuf)==0)
		return 0;
	
	memcpy(data,rbuf+512,flash_type[device].sectors_per_page*512);

	/*写FW,FW的长度最大16个page,fw的起始位置是512×32*/
	for(i=0;i<16;i++)
	{
		page=32/flash_type[device].sectors_per_page+i+blocknum*flash_type[device].pages_per_block;	
		if(SW80PageRead(device,channelnum,dienume,page,0,rbuf)==0)
			return 0;

		memcpy(data+512*32+flash_type[device].sectors_per_page*512*i,rbuf+512,flash_type[device].sectors_per_page*512);
	}
	return 1;
}


BOOL SW80CompareGlobalBlock(int device,unsigned char * data,int channelnum,int dienum,int blocknum)
{

	PUCHAR ReadBuffer;
	int i,readsize=1024*flash_type[device].size_block;
	CString str;

	ReadBuffer=(PUCHAR)malloc(readsize);
	memset(ReadBuffer,0,readsize);
	BOOL Cmpflag=0;
	if( SW80ReadGlobalBlock(device,ReadBuffer,channelnum,dienum,blocknum))
	{

		for(i=0;i<readsize;i++)
		{
			if(ReadBuffer[i]!=data[i])
			{
				str.Format("offset: %x  w: %x   r:%x",i,data[i],ReadBuffer[i]);
				AddInfo(device,str,3);
				Cmpflag=1;
			}
		}
		if(Cmpflag)
		{
			free(ReadBuffer);
			AddInfo(device,"Compare  fw failed!",3);
			return 0;
		}
	}
	else
	{
		AddInfo(device,"****Read Fw info failed!!!",3);	
		free(ReadBuffer);
		return 0;
	}

	free(ReadBuffer);
	AddInfo(device,"****Compare Fw info successfully!!!",1);	
	return 1;
}

int SSDInitialize(int device)
{
	int ret=0;
	int m;

	CString str;
	DWORD time;
#if _INITD 
	pdlgcmd->m_progress.SetPos(1);
#endif

	ret = SWInitial(device,0);
	if(ret>0)
	{
		SW80WirteBadblockflag(device);
		AddInfo(device,"Write bad block flag successfully!\r\n",1);
		for(m=0;m<10;m++)
		{
			if(CompareBadblock(device,FWBlockNum,0,0)==0)
				FWBlockNum+=2;
			else
				break;
		}
		if(m==10)
		{
			AddInfo(device,"Can not find available block !\r\n",3);
			free(GlobelBuffer[device]);
			return -1;
		}
		if(SW80WriteGlobalBlock(device,GlobelBuffer[device],0,0,FWBlockNum)==0)
		{
			AddInfo(device,"Write Global block failed!\r\n",3);
			free(GlobelBuffer[device]);
			return -1;
		}
		AddInfo(device,"Write Global block successfully!\r\n",1);
		if(SW80CompareGlobalBlock(device,GlobelBuffer[device],0,0,FWBlockNum)==0)
		{
			free(GlobelBuffer[device]);
			return -1;
		}
		free(GlobelBuffer[device]);

		AddInfo(device,"Write Global block successfully!\r\n",1);
		if(AcessControlFlag)
		{
			if(SW80WriteAccessCtlBlock(device)==0)
			{
				AddInfo(device,"Write access control block failed!\r\n",3);
				return -1;
			}
			else
				AddInfo(device,"Write access control block successfully!\r\n",1);
		}
#if _INITD 
		pdlgcmd->m_progress.SetPos(90);
#endif
		AddInfo(device,"******Initionalize completed!!!\r\n",1);
	}

	time = GetTickCount() - StartTime;
	time /= 1000;			// seconds
	int min = time / 60;// minute
	time %= 60;			// seconds
	str.Format("Elapse time : %d : %d ", min, time);
	AddInfo(device,str,1);

	if(ret>0)
	{
#if _INITD 
		pdlgcmd->m_progress.SetPos(100);
		pdlgcmd->m_progress.SetText("Initionalize completed");
		pdlgcmd->m_progress.SetPos(0);
#endif
		return 1;
	}
	else
	{
#if _INITD 
		pdlgcmd->m_progress.SetPos(100);
		pdlgcmd->m_progress.SetText("Initionalize failed");
		pdlgcmd->m_progress.SetPos(0);
#endif
		return -1;
	}
}

/*初始化函数，在检测到新增坏块时，更新坏块表，并重新初始化，初始化次数不能大于InitialCount*/
int SWInitial(int device,int Initnum )
{
	if(Initnum >= InitialCount)
	{
		AddInfo(device,"****Find too many bad block,advice Initial from chip !\r\n",3);
		return -1;
	}
	
#if _INITD 			
	pdlgcmd->m_progress.SetText("");	
	pdlgcmd->m_progress.SetPos(0);
#endif

#if _MASSINIT 			
	time_t ltime;
	time( &ltime );
	char hostname[512];
	DWORD len=512;
	memset(hostname,0,512);
	CString str;

	str.Format( "\n\r\Initail time is %s\n", ctime( &ltime ) );
	AddInfo(device,str,1);
	GetComputerName(hostname,&len);
	str.Format( "Compute name is %s\n", hostname );
	AddInfo(device,str,1);
	str.Format( "FW FILE: %s\n", FWFilename );
	AddInfo(device,str,1);
	str.Format( "LOADER FILE: %s\n", LoadFilename );
	AddInfo(device,str,1);
#endif

	if(ReadFlashID(device)==0)
	{
		AddInfo(device,"****ReadID fail!\r\n",3);
		return -1;
	}
	
	if(SetTwoPlane(device)==0)
	{
		AddInfo(device,"****Two plane setting error!\r\n",3);
		return -1;
	}

	if(SW80SetNFC(device)==0)
	{
		AddInfo(device,"****Configure NFC Parameters fail!\r\n",3);
		return -1;
	}

	AddInfo(device,"Building BadblockTable...",1);
	if( !SW80GetDeviceInfo(device,Initnum))		// 建立坏块表
	{
		AddInfo(device,"****failed to build bad table!!!\r\n",3);
		SSdlistItem[device].BuildTableStatus=0;
		return -1;
	}
	else
	{
		SSdlistItem[device].BuildTableStatus=1;
	}
	AddInfo(device,"Build bad block table successfully!\r\n",1);

#if _INITD 
	pdlgcmd->m_progress.SetPos(80);
#endif


	if(SWBuildGlobleInfoBlock(device)==0)
	{
		AddInfo(device,"****failed to build GlobleInfoBlock!!!\r\n",3);
		free(GlobelBuffer[device]);
		return -1;
	}
	AddInfo(device,"build GlobleInfoBlock successfully!!!\r\n",3);
#if _INITD 
	pdlgcmd->m_progress.SetPos(85);
#endif
	return 1;
}

BOOL SWBuildGlobleInfoBlock(int device)
{
	int i;
	char output[256];
	int datasize=flash_type[device].size_block * 1024;
	GlobelBuffer[device]=(PUCHAR) malloc(datasize);
	
	unsigned char venderflag[16]="SOLIWARE-80";

	for(i=0;i<datasize;i++)
	{
		GlobelBuffer[device][i]=0;		
	}

	int addptr=0;
	GlobelBuffer[device][addptr++]=0x55;//globleinfoblock flag
	GlobelBuffer[device][addptr++]=0xAA;
	GlobelBuffer[device][addptr++]=0x5A;
	GlobelBuffer[device][addptr++]=0xA5;

	GlobelBuffer[device][addptr++]=0x20;//FW 起始位置（扇区为单位）
	GlobelBuffer[device][addptr++]=0x00;
	
	GlobelBuffer[device][addptr++]=0x80;//FW 长度（扇区为单位）
	GlobelBuffer[device][addptr++]=0x00;

	GlobelBuffer[device][addptr++]=0x06;//SMART DATA 起始位置（扇区为单位）
	GlobelBuffer[device][addptr++]=0x00;

	GlobelBuffer[device][addptr++]=0x07;//SMART threshold起始位置（扇区为单位）
	GlobelBuffer[device][addptr++]=0x00;

	GlobelBuffer[device][addptr++]=0x01;//IDENTIFY 0 起始位置（扇区为单位）
	GlobelBuffer[device][addptr++]=0x00;

	
	GlobelBuffer[device][addptr++]=0x16;//IDENTIFY 1 起始位置（扇区为单位）
	GlobelBuffer[device][addptr++]=0x00;

	GlobelBuffer[device][addptr++]=0x24;//IDENTIFY 2 起始位置（扇区为单位）
	GlobelBuffer[device][addptr++]=0x00;
	
	GlobelBuffer[device][addptr++]=0x02;//FW运行参数起始位置（扇区为单位）
	GlobelBuffer[device][addptr++]=0x00;
	

	SW80Twoplanebadblock(device);
	//FW INFO
	addptr=1024;		
    SW80WriteFWInfo(device,GlobelBuffer[device], addptr);	


	addptr=512;		//IDFY DATA
	SLWSetCapacity(device);

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

	if(m_descriptor=="")
	{
		descr="SoliWare";
	}
	else
		descr=m_descriptor;
	
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

	/*SATA1 支持标志*/
	if(EnableSata1)
		GlobelBuffer[device][addptr+152]=0x2;
	
	/*企业标识符，用来识别SOLIWARE80系列盘*/
	for(i=0;i<16;i++)
	{
		GlobelBuffer[device][addptr+258+i]=	venderflag[i];
	}
	/* DATA SET MANAGEMENT is supported */
	GlobelBuffer[device][addptr+338]=0x1;
	GlobelBuffer[device][addptr+339]=0x0;
	/*Bit 15 must be zero, bit 14 must be 1. Bits 13:0 indicate the logical sector offset within the first physical sector where the first logical sector is placed. 1-aligned*/
	GlobelBuffer[device][addptr+418]=0x1;
	GlobelBuffer[device][addptr+419]=0x40;
	/*Non-rotating media device*/
	GlobelBuffer[device][addptr+434]=0x1;
	GlobelBuffer[device][addptr+435]=0x0;

	GlobelBuffer[device][addptr+510]=0xa5;
	int checksum=0;
	for(i=0;i<511;i++)
	{
		checksum+=GlobelBuffer[device][addptr+i];
	}
	GlobelBuffer[device][addptr+511]=~checksum+1;//Bytes 511 为所有值的CheckSum
		
	addptr = 512*6;
	SW80WritesmartInfo(device,GlobelBuffer[device],addptr);

	long ret;
	addptr = 512*0x20;
	ret = SLWGlobalinfo(device,GlobelBuffer[device],0,addptr,0);
	if(ret ==0 )
	{				
		AddInfo(device,"Get FW code fail!\r\n",3);
		return 0;
	}
	AddInfo(device,"Get FW code successfully!\r\n",1);

	if(ret%512>0)
		GlobelBuffer[device][6]=1+ret/512;//FW 长度（扇区为单位）
	else
		GlobelBuffer[device][6]=ret/512;//FW 长度（扇区为单位）
	return 1;
}

BOOL ReadConfigureIni(CString FilePath)
{
	char str[256];
	char filename[256];
	COPini ini;

	CString mdesconf;
	CString sn;
	int i;

    GetModuleFileName(NULL,filename,256); 
    //Scan a string for the last occurrence of a character.
    (strrchr(filename,'\\'))[1] = 0; 
    strcat(filename,FilePath);	
	
	ini.ReadString("Device Setting","Model Number",str,filename);
	m_descriptor = str;
	ini.ReadString("Device Setting","Serial Number",str,filename);
	sn=str;

#if _INITD
	if(sn.IsEmpty()==0)
	{
		for(i=0;i<DeviceCount;i++)
			BadBlockFilename[i] = sn;
	}
	ini.ReadString("Device Setting","Channel Number",str,filename);
	channel=atol(str);
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
	ini.ReadString("FW Setting","Maptable Offset",str,filename);
	MaptableOffset=atoi(str);
	ini.ReadString("FW Setting","Acess Control Flag",str,filename);
	AcessControlFlag=atol(str);
	ini.ReadString("FW Setting","Purge Time",str,filename);
	Purgetime=atol(str);
	ini.ReadString("FW Setting","ECC Alarm Threshold",str,filename);
	ECCThreshold=atol(str);
	ini.ReadString("FW Setting","USB Retry Counter",str,filename);
	Retrycnt=atol(str);
	ini.ReadString("FW Setting","Restore SSD Restart Flag",str,filename);
	Restartflag=atol(str);
	ini.ReadString("FW Setting","Set Capacity",str,filename);
	SetCapacity=_atoi64(str);

#if _MASSINIT
	ini.ReadString("Update Setting","FW FILE",str,filename);
	FWFilename=str;
	ini.ReadString("Update Setting","LOAD FILE",str,filename);
	LoadFilename=str;
	ini.ReadString("Option","SSD INITIONALIZE",str,filename);
	checkinit=atoi(str);
	ini.ReadString("Option","Validation Test",str,filename);
	checkvalid=atoi(str);
	ini.ReadString("Option","Channel Number",str,filename);
	channel=atoi(str);
	ini.ReadString("Option","Input SSD ID",str,filename);
	checkinputid=atoi(str);
#endif	
	return 1;
}

/*清空全局变量，为初始化做准备*/
void ResetGlobal(int device,int Initnum)
{
	int i,j;
	/*当第一次初始化时清空原始坏块表，其他时候更新原始坏块表*/
	if(Initnum==0)
	{
		for(i=0;i<gldie*4;i++)
			for(j=0;j<512;j++)			
				badblock[device][i][j]=0;				

		for(i=0;i<gldie*4;i++)
			xblock[device][i]=0;

		for(i=0;i<gldie;i++)
			DIEMAP[device][i]=0xFFFF;

		for(i=0;i<4;i++)
			for(j=0;j<64;j++)
				flash_id[device][i][j]=0;

		Totalbadblock[device]=0;
		grdie[device]=0;
		gcs[device]=0;
		memset((void*)&flash_type[device],0,sizeof(Flash_Para));
	}		
}

void SW80WirteBadblockflag(int device)
{
	int i,j,m;	
	BYTE databuf[32*512],oob[24];
	DWORD pagenum;
	int twoplanebadblocknum[128];
	int twoplanebadblock[128][128];
	int tmpblock;
	int flag=0;

	memset(databuf,0,32*512);
	memset(oob,0,24);

	for(i=0;i<channel*gldie;i++)
	{
		twoplanebadblocknum[i]=xblock[device][i];
		for(j=0;j<xblock[device][i];j++)
		{
			twoplanebadblock[i][j]=badblock[device][i][j];
		}
	}

	for(i=0;i<channel*gldie;i++)
	{
		for(j=0;j<xblock[device][i];j++)
		{
			pagenum=flash_type[device].pages_per_block*badblock[device][i][j];
			SW80PageWrite(device,i/gldie,i%gldie,pagenum,oob,databuf);
			flag=0;
			if(EnableTwoplane==1)
			{
				if(badblock[device][i][j]%2==0)
				{
					tmpblock=badblock[device][i][j]+1;
					/*判断坏块是不是已经被标识*/
					for(m=0;m<twoplanebadblocknum[i];m++)
					{
						if(tmpblock==twoplanebadblock[i][m])
						{
							flag=1;
							break;
						}
					}
					if(flag==0)
					{
						pagenum=flash_type[device].pages_per_block*(badblock[device][i][j]+1);
						SW80PageWrite(device,i/gldie,i%gldie,pagenum,oob,databuf);
						twoplanebadblock[i][twoplanebadblocknum[i]] = tmpblock;   
						twoplanebadblocknum[i]++;
					}
				}
				else
				{
					tmpblock=badblock[device][i][j]-1;
					for(m=0;m<twoplanebadblocknum[i];m++)
					{
						if(tmpblock==twoplanebadblock[i][m])
						{
							flag=1;
							break;
						}
					}
					if(flag==0)
					{
						pagenum=flash_type[device].pages_per_block*(badblock[device][i][j]-1);
						SW80PageWrite(device,i/gldie,i%gldie,pagenum,oob,databuf);
						twoplanebadblock[i][twoplanebadblocknum[i]] = tmpblock;   //badblock的第二维对应逻辑die,所以要转换为i+m_row-i%gldie
						twoplanebadblocknum[i]++;
					}
				}
			}
		}
	}
}

BOOL SW80ReadGlobalinfoblockTxt(CString File)
{
	FILE *fp;
	int i,num;
	char output[128];
	CString str;

	CHAR FilePath[255]; 

	strcpy(FilePath,LocalPath);
    strcat(FilePath,File);

	fp = fopen(FilePath, "rb");
	if(fp==NULL)
	{
		AfxMessageBox("Open globalinfo file fail!");
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
	fscanf(fp,"%d",&FLASHNum);
	for(i=0;i<FLASHNum;i++)
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
	int addptr=512;
	ULONGLONG lcapa;
	DWORD tmp,LCN=0;
	int blockcnt;

	tmp=grdie[device]*channel*flash_type[device].blocknum*(128-GlobelBuffer[device][158+1024])/(GlobelBuffer[device][162+1024]*128);
	if(flash_type[device].blocknum%1024!=0)
	{
		blockcnt=(flash_type[device].blocknum/1024)*1024;  //取整，保证计算block数是1024的整数倍
		tmp=grdie[device]*channel*blockcnt*(128-GlobelBuffer[device][158+1024])/(GlobelBuffer[device][162+1024]*128);
	}
	lcapa=tmp*(flash_type[device].pages_per_block*GlobelBuffer[device][162+1024]-GlobelBuffer[device][160+1024])*flash_type[device].sectors_per_page;

	LCN=lcapa/(16*63);			//Number of logical cylinders
	if(LCN<0x3FFF)
	{
		IDFY32G[1]=LCN;
		IDFY32G[83]=0x7000;			//小于8G关闭48 BIT LBA
		IDFY32G[86]=0x3000;			//小于8G关闭48 BIT LBA	
	}
	else
	{
		IDFY32G[1]=0x3FFF;
		IDFY32G[83]=0x7400;		//大于8G打开48 BIT LBA
		IDFY32G[86]=0x3400;     //大于8G打开48 BIT LBA
	}
	IDFY32G[54]=IDFY32G[1];
	tmp=IDFY32G[1]*16*63;
	IDFY32G[57]=tmp;
	IDFY32G[58]=(tmp)>>16;
	IDFY32G[60]=lcapa;
	IDFY32G[61]=lcapa>>16;
	IDFY32G[100]=IDFY32G[60];
	IDFY32G[101]=IDFY32G[61];

	if(lcapa>=0xfffffff)
	{

		IDFY32G[100]=lcapa;
		IDFY32G[101]=lcapa>>16;
		IDFY32G[60]=0xffff;
		IDFY32G[61]=0xfff;
	}

	for(i=0;i<256;i++)
	{
		GlobelBuffer[device][addptr+2*i]=(unsigned char)IDFY32G[i];//WORD的低位在前
		GlobelBuffer[device][addptr+2*i+1]=(unsigned char)(IDFY32G[i]>>8);
	}

	
	
	MaxLBA = (IDFY32G[101]<<16|IDFY32G[100])-1;
}

long SLWGlobalinfo(int device,PUCHAR buf,int Initnum,long offset,BOOL CodeFlag)
{
	PUCHAR DATABuffer;	
	long datasize=64*2048*(flash_type[device].size_block/128);//new
	int i;
	FILE *fp;
	CString strDldFile;	
	int readnum;
	long line;

	if(flash_type[device].size_block==0)
		datasize=64*2048*512/128;
	DATABuffer=(PUCHAR) malloc(datasize);

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
			"(*.bin)|*.bin|AllFiles (*.*)|*.*||");
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
			LoadFilename = strDldFile;
	}
	else
		strDldFile=FWFilename;
#endif


#if _UPDATE
	char filename[256];	
	memset(filename,0,256);
	GetModuleFileName(NULL,filename,256); 
	(strrchr(filename,'\\'))[1] = 0;

	strDldFile = filename+FWFilename ;
#endif


#if _MASSINIT 
	char filename[256];
	GetModuleFileName(NULL,filename,256); 
	(strrchr(filename,'\\'))[1] = 0;
	
	if(CodeFlag==0)
		strDldFile = filename+FWFilename ;
	else
		strDldFile = filename+LoadFilename ;
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
			AddInfo(device,"打开.b文件失败!",3);
			free(DATABuffer);
			return 0;
		}
		fclose(fp);
	}
	else
	{
		AddInfo(device,"找不到.b文件!",3);
		free(DATABuffer);
		return 0 ;
	}

	line = line*32;
	long addptr=offset;			//code block
	for(i=0;i<line;i++)
	{
		buf[addptr++] = DATABuffer[i];
	}
	free(DATABuffer);
	return line;
}


void SetFlashCap(int device)
{

	if(flash_type[device].isMLC==1&&flash_type[device].cap_in_gb==4)
	{

		if(grdie[device]==4)
		{
			if(flash_type[device].ID[1]==0xD7)
			{
				flash_type[device].cap_in_gb = 8;
			}
		}

		if(grdie[device]==8)
		{
			if(flash_type[device].ID[1]==0xD7)
			{
				flash_type[device].cap_in_gb = 16;
			}
		}		
	}

	if(flash_type[device].isMLC==0 && flash_type[device].cap_in_gb==2)
	{
		if(grdie[device]==4)
		{
			if(flash_type[device].ID[1]==0xD5)
			{
				flash_type[device].cap_in_gb = 4;
			}
		}

		if(grdie[device]==8)
		{
			if(flash_type[device].ID[1]==0xD5)
			{
				flash_type[device].cap_in_gb = 8;
			}
		}	

	}

}


BOOL SW80CheckProtectStatus(int device,ULONGLONG vendorlba,BYTE *result,BYTE *partitionflag)
/*
function:读取上一条命令的执行状态,还可以鉴别磁盘分区状态
partitionflag：0 控制分区，1 用户分区
*/
{
	BYTE readbuf[512],tmp[32];
	BOOL ret;
	memset(readbuf,0,512);
	memset(tmp,0,32);
	CString str;	

	ret=ReadSectors(device,vendorlba+1,1,readbuf);
	memcpy(tmp,readbuf+498,10);
	str.Format("WS08D_SI_K:%s",tmp);
	//AfxMessageBox(str);
	if(ret==0)
		return ret;
	if(readbuf[1]=='F' && readbuf[2]=='I')
	{
		*result=readbuf[0];
		*partitionflag=readbuf[3];
	}

	if(readbuf[0]==0)
		return 1;
	else
		return 0;
}

BOOL SW80CheckRetryStatus(int device,ULONGLONG vendorlba,int *cnt)
/*
cnt 剩余的重试次数，为0时触发销毁
*/
{
	BYTE readbuf[512];
	BOOL ret;
	memset(readbuf,0,512);
	
	ret=ReadSectors(device,vendorlba+2,1,readbuf);
	if(ret==0)
		return 0;

	*cnt=(readbuf[0x33]<<8)+readbuf[0x32];

	return 1;
}

int SW80CheckGlobal(int device,ULONGLONG vendorlba)
/*
function:读取全局信息，不需要写命令，直接读取vendorlba+2
返回值： 1  支持USB访问控制
		 2  支持SSD restore
		 0  其他
*/
{
	BYTE readbuf[512];
	BOOL ret;
	memset(readbuf,0,512);
	
	ret=ReadSectors(device,vendorlba+2,1,readbuf);
	if(ret==0)
		return 0;

	unsigned int c,d,fctlword;
	fctlword=(readbuf[0x8D]<<8)+readbuf[0x8C];
	c=fctlword&0x0004;
	d=fctlword&0x0008;
	if(c>0)
		return 1;
	else if(d>0)
		return 2;
	else
		return 0;

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
	
	if(readbuf[1]=='F' && readbuf[2]=='I' )
	{
		*result=readbuf[0];
		return 1;
	}
	else
		return 0;
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

ULONGLONG SW80GetVendorLBA(int device)
{

	ULONGLONG vendorlba;
	BYTE databuf[512];
	if(IDFY_IDE(device,databuf)==0)
		return 0;
	else
	{
		vendorlba=databuf[203]<<24 | databuf[202]<<16 | databuf[201]<< 8 | databuf[200];
		vendorlba=vendorlba+0x800000;
		return vendorlba;
	}

/*	vendorlba=0xF800000;
	return vendorlba;
*/
}

BOOL SW80UpdateFWInRamdisk(int device)
{
	PUCHAR buf,ReadBuffer;
	long ret =0;
	int i,datasize=1024*flash_type[device].size_block,readsize=1024*flash_type[device].size_block;
	CString str;
	int channelnum=0,dienum=0,blocknum=0;
	DWORD pagenum=0;
	int addptr;

	if(SW80FindGCblock(device,&channelnum,&dienum,&blocknum)==0)
	{
		AddInfo(device,"****Read global info failed!!!",3);	
		return 0;
	}

	pagenum=blocknum*flash_type[device].pages_per_block;
	
	buf=(PUCHAR) malloc(datasize);
	ReadBuffer=(PUCHAR)malloc(readsize);

	for(i=0;i<datasize;i++)
	{
		buf[i]=0;
	}
	for(i=0;i<readsize;i++)
	{
		ReadBuffer[i]=0x00;
	}

	/*读全局信息*/
	if(SW80PageRead(device,channelnum,dienum,pagenum,0,buf)==0)
	{
		AddInfo(device,"****Read global info failed!!!",3);	
		free(buf);
		free(ReadBuffer);
		return 0;		
	}

	addptr = 512*3;
	//判断是否是新版本的FW 运行参数，如果是，不用更新
	if(	buf[addptr+0x50]==0xAA && buf[addptr+0x51]==0x55 && buf[addptr+0x52]==0x11 && buf[addptr+0x53]==0x22)
		;
	else
		SW80UpdateFWInfoInRAM(device,buf,addptr);

	addptr = 512*32;
	/*因为SW80PageRead读了9个扇区，第一个扇区是返回的状态，实际内容是后8个扇区，所以buf的需要后移512位*/
	ret = SLWGlobalinfo(device,buf,0,addptr+512,0);
	if(ret ==0 )
	{				
		AddInfo(device,"Get FW code fail!\r\n",3);
		free(buf);
		free(ReadBuffer);
		return 0;
	}

	if(ret%512>0)
		buf[512+6]=1+ret/512;//FW 长度（扇区为单位）
	else
		buf[512+6]=ret/512;//FW 长度（扇区为单位）
   

	if(SW80WriteGlobalBlock(device,buf+512,channelnum,dienum,blocknum)==0)
	{
		AddInfo(device,"Write FW info block fail!\r\n",3);
		free(buf);
		free(ReadBuffer);
		return 0;
	}	
	else
		AddInfo(device,"Write FW info block successfully!\r\n",1);

	BOOL Cmpflag=0;
	if( SW80ReadGlobalBlock(device,ReadBuffer,channelnum,dienum,blocknum))
	{

		for(i=0;i<datasize-512;i++)
		{
			if(ReadBuffer[i]!=buf[i+512])
			{
				str.Format("offset: %x  w: %x   r:%x",i,buf[i+512],ReadBuffer[i]);
				AddInfo(device,str,3);
				Cmpflag=1;
			}
		}
		if(Cmpflag)
		{
			free(ReadBuffer);
			free(buf);
			AddInfo(device,"Compare loader fw failed!",3);
			return 0;
		}
	}
	else
	{
		AddInfo(device,"****Read Fw info failed!!!",3);	
		free(buf);
		free(ReadBuffer);
		return 0;
	}

	free(buf);
	free(ReadBuffer);
	AddInfo(device,"****Compare Fw info successfully!!!",1);	
	return 1;
}

BOOL SW80SSDUpdateFW(int device,ULONGLONG vendorlba)
{
	PUCHAR buf,ReadBuffer;
	long ret =0;
	int i,datasize=1024*512,readsize=1024*512;
	CString str;
	long addptr;

	buf=(PUCHAR) malloc(datasize);
	ReadBuffer=(PUCHAR)malloc(readsize);

	for(i=0;i<datasize;i++)
	{
		buf[i]=0;
	}
	for(i=0;i<readsize;i++)
	{
		ReadBuffer[i]=0x00;
	}


	/*读全局信息*/
	Sleep(100);
	if(SW80SSDReadFW(device,vendorlba,buf)==0)
	{
		AddInfo(device,"****Read global info failed!!!",3);	
		free(buf);
		free(ReadBuffer);
		return 0;		
	}

	addptr = 1024;
	
	//判断是否是新版本的FW 运行参数，如果是，不用更新
	if(	buf[addptr+0x50]==0xAA && buf[addptr+0x51]==0x55 && buf[addptr+0x52]==0x11 && buf[addptr+0x53]==0x22)
		;
	else
		SW80UpdateFWInfo(device,buf,addptr);

	if(buf[addptr+0x9E]==1)
	{
		buf[addptr+0x9E]=8; //(FW版本是1042以下，要强制修改保留比例)
	}

	addptr = 512*6;
	SW80WritesmartInfo(device,buf,addptr);

	addptr = 512*32;
	ret = SLWGlobalinfo(device,buf,0,addptr,0);
	if(ret ==0 )
	{				
		AddInfo(device,"Get FW code fail!\r\n",3);
		free(buf);
		free(ReadBuffer);
		return 0;
	}

	if(ret%512>0)
		buf[6]=1+ret/512;//FW 长度（扇区为单位）
	else
		buf[6]=ret/512;//FW 长度（扇区为单位）
	Sleep(100);
	if(SW80SSDWriteFW(device,vendorlba,buf)==0)
	{
		AddInfo(device,"Write FW info block fail!\r\n",3);
		free(buf);
		free(ReadBuffer);
		return 0;
	}	
	else
		AddInfo(device,"Write FW info block successfully!\r\n",1);

	BOOL Cmpflag=0;
	Sleep(100);
	if( SW80SSDReadFW(device,vendorlba,ReadBuffer))
	{

		for(i=0;i<datasize;i++)
		{
			if(ReadBuffer[i]!=buf[i])
			{
				str.Format("offset: %x  w: %x   r:%x",i,buf[i],ReadBuffer[i]);
				AddInfo(device,str,3);
				Cmpflag=1;
			}
		}
		
		if(Cmpflag)
		{
			free(ReadBuffer);
			free(buf);
			AddInfo(device,"Compare fw failed!",3);
			return 0;
		}
	}
	else
	{
		AddInfo(device,"****Read Fw info failed!!!",3);	
		free(buf);
		free(ReadBuffer);
		return 0;
	}

	free(buf);
	free(ReadBuffer);
	AddInfo(device,"****Compare Fw info successfully!!!",1);	
	return 1;
}

BOOL SW80SSDReadFW(int device,ULONGLONG vendorlba,BYTE *buf)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=0xA1;
	buffer[16]=64*2;
	if(WriteSectors(device,vendorlba,1,buffer)==0)
		return 0;
	Sleep(50);
	ret=ReadSectors(device,vendorlba,64*2,buf);
	return ret;
}

BOOL SW80SSDWriteFW(int device,ULONGLONG vendorlba,BYTE *buf)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=0xA2;
	buffer[16]=64*2;
	if(WriteSectors(device,vendorlba,1,buffer)==0)
		return 0;
	Sleep(50);
	ret=WriteSectors(device,vendorlba,64*2,buf);
	return ret;
}


BOOL SW80UpdateLoad(int device,CString strDlgFile,int flag)
/*
在RAMDISK下写入loaderFW
flag：0  loaderFW是第一次写入，没有写入FW info，直接写入
flag: 1  loaderFW已经存在，升级FW info
*/
{
	PUCHAR DATABuffer,readbuf;
	int datasize = 64*512,i;
	DWORD oobnum=0,tmpnum=0;
	BYTE oob[24],pagebuf[512*32];
	CString str;
	CFile fl;

	memset(oob,0,24);
	memset(pagebuf,0,512*32);
	DATABuffer=(PUCHAR)malloc(datasize);
	memset(DATABuffer, 0, datasize);

	if( !fl.Open(strDlgFile, CFile::modeRead, NULL) )
	{
		AddInfo(device,"Open loadfw File Failed !",3);
		free(DATABuffer);
		return 0;		
	}

	if(fl.GetLength() > datasize )
	{
		AddInfo(device,"File size should below 32KB !",3);
		free(DATABuffer);
		return 0;	
	}

	if( !fl.Read(DATABuffer, fl.GetLength()) )
	{
		AddInfo(device,"Read loadfw file Failed !",3);
		free(DATABuffer);
		fl.Close();
		return 0;	
	}
	fl.Close();


	if(flag==1)
	{
		int channelnum=0,dienum=0,blocknum=0;
		DWORD pagenum=0;
		if(SW80FindGCblock(device,&channelnum,&dienum,&blocknum)==0)
		{
			AddInfo(device,"****Read global info failed!!!",3);	
			free(DATABuffer);
			return 0;
		}
		pagenum=blocknum*flash_type[device].pages_per_block;
			/*读全局信息*/
		if(SW80PageRead(device,channelnum,dienum,pagenum,0,pagebuf)==0)
		{
			AddInfo(device,"****Read global info failed!!!",3);	
			free(DATABuffer);
			return 0;		
		}
	}

	if(SW80SetBCH(device,1)==0)
	{
		AddInfo(device,"Set  BCH16 mode failed",3);	
		free(DATABuffer);
		return 0;
	}


	if(SW80Eraseblock(device,0,0,0)==0)
	{
		AddInfo(device,"Erase block failed!",3);
		free(DATABuffer);
		return 0;
	}

//在写入RAMDISK LOADER的时候同时写入SN
#if _ADVANCEFEATURE
	unsigned char sid[32];
	memset(sid,0,32);
	memcpy(sid,RAMDISKSN,RAMDISKSN.GetLength());
	SW80SetSNtoRamdisk(device,DATABuffer,sid);
#endif

	long addr,addptr;
	addr=512*63;
	addptr=512*3;
	if(flag==0)
		SW80WriteFWInfo(device,DATABuffer,addr);/*写入FW 运行参数到和loader中(32K的最后一个sector)*/
	else if(flag==1)
	{
		//判断是否是新版本的FW 运行参数，如果是，不用更新
		if(	pagebuf[addptr+0x50]==0xAA && pagebuf[addptr+0x51]==0x55 && pagebuf[addptr+0x52]==0x11 && pagebuf[addptr+0x53]==0x22)
			memcpy(DATABuffer+addr,pagebuf+addptr,512);
		else
		{
			memcpy(DATABuffer+addr,pagebuf+addptr,512);
			SW80UpdateFWInfoInRAM(device,DATABuffer,addr);	
		}

	}
	
	for(i=0;i<8*1024;i++)
	{
		tmpnum=DATABuffer[4*i]+(DATABuffer[4*i+1]<<8)+(DATABuffer[4*i+2]<<16)+(DATABuffer[4*i+3]<<24);
		oobnum=oobnum+tmpnum;
	}
	
	str.Format("The checknum is %x",oobnum);
	AddInfo(device,str,1);
	for(i=0;i<32;i++)
	{
		oob[0]=oobnum;
		oob[1]=oobnum>>8;
		oob[2]=oobnum>>16;
		oob[3]=oobnum>>24;
		memcpy(pagebuf,DATABuffer+i*1024,1024);
		if(SW80PageWrite(device, 0, 0, i,oob,pagebuf)==0)
		{
			AddInfo(device,"Page write failed !",3);
			free(DATABuffer);
			return 0;
		}
	}

	AddInfo(device,"Write fw loader successfully!",1);
	readbuf=(PUCHAR)malloc(datasize);
	memset(readbuf, 0, datasize);
	for(i=0;i<32;i++)
	{
		if(SW80PageRead(device, 0, 0, i,0,pagebuf)==0)
		{
			AddInfo(device,"Page read failed !",3);
			free(readbuf);
			free(DATABuffer);
			return 0;
		}
		memcpy(readbuf+i*1024,pagebuf+512,1024);
	}
	

	BOOL Cmpflag=0;
	for(i=0;i<64*512;i++)
	{
		if(readbuf[i]!=DATABuffer[i])
		{
			str.Format("offset: %x  w: %x   r:%x",i,DATABuffer[i],readbuf[i]);
			AddInfo(device,str,3);
			Cmpflag=1;
		}
	}

	if(Cmpflag)
	{
		free(readbuf);
		free(DATABuffer);
		AddInfo(device,"Compare loader fw failed!",3);
		return 0;
	}


	free(readbuf);
	free(DATABuffer);
	AddInfo(device,"Compare loader fw successfully!",1);

	if(SW80SetBCH(device,0)==0)
	{
		AddInfo(device,"Set  BCH8 mode failed",3);	
		return 0;
	}
	return 1;
}


BOOL SW80SSDReadLoad(int device,ULONGLONG vendorlba,BYTE *buf)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=0xA6;
	buffer[16]=32*2;
	if(WriteSectors(device,vendorlba,1,buffer)==0)
		return 0;
	Sleep(50);
	ret=ReadSectors(device,vendorlba,32*2,buf);
	return ret;
}

BOOL SW80SSDWriteLoad(int device,ULONGLONG vendorlba,DWORD OOB,BYTE *buf)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=0xA3;
	buffer[16]=32*2;
	buffer[20]=OOB;
	buffer[21]=OOB>>8;
	buffer[22]=OOB>>16;
	buffer[23]=OOB>>24;
	if(WriteSectors(device,vendorlba,1,buffer)==0)
		return 0;
	Sleep(50);
	ret=WriteSectors(device,vendorlba,32*2,buf);
	return ret;
}

BOOL SW80SSDUpdateLoad(int device,ULONGLONG vendorlba,CString strDlgFile)
{

	PUCHAR DATABuffer,readbuf;
	int datasize = 64*512,i;
	CString str;
	DWORD oobnum=0,tmpnum=0;
	DATABuffer=(PUCHAR)malloc(datasize);
	memset(DATABuffer, 0, datasize);

	CFile fl;
	if( !fl.Open(strDlgFile, CFile::modeRead, NULL) )
	{
		AfxMessageBox("Open loadfw File Failed !");
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
		AfxMessageBox("Read loadfw File Failed !");
		free(DATABuffer);
		fl.Close();
		return 0;	
	}
	fl.Close();

#if _ADVANCEFEATURE
	unsigned char sid[32];
	memset(sid,0,32);
	memcpy(sid,RAMDISKSN,RAMDISKSN.GetLength());
	SW80SetSNtoRamdisk(device,DATABuffer,sid);
#endif
	

	readbuf=(PUCHAR)malloc(datasize);
	memset(readbuf, 0, datasize);
	if(SW80SSDReadLoad(device,vendorlba,readbuf)==0)
	{
		AddInfo(device,"Read loader fw failed !",3);
		free(readbuf);
		free(DATABuffer);	
		return 0;
	}
			
	str.Format("offset: %x  read:%x",0x1820,readbuf[0x1820]);
	str.Format("offset: %x  read:%x",0x2025,readbuf[0x2025]);
	AddInfo(device,str,3);

	long addptr=63*512;
	//判断是否是新版本的FW 运行参数，如果是，不用更新
	if(	readbuf[addptr+0x50]==0xAA && readbuf[addptr+0x51]==0x55 && readbuf[addptr+0x52]==0x11 && readbuf[addptr+0x53]==0x22)
		memcpy(DATABuffer+addptr,readbuf+addptr,512);
	else
	{

		long addr;
		addr=512*63;
		if(SW80UpdateFWInfo(device,DATABuffer,addr)==0)
		{
			AddInfo(device,"Read fw info failed !",3);
			free(readbuf);
			free(DATABuffer);
			return 0;
		}
	}

	if(Vernum<=1042)
	{
		DATABuffer[addptr+0x9E]=8; //(FW版本是1042以下，要强制修改保留比例)
	}


	for(i=0;i<8*1024;i++)
	{
		tmpnum=DATABuffer[4*i]+(DATABuffer[4*i+1]<<8)+(DATABuffer[4*i+2]<<16)+(DATABuffer[4*i+3]<<24);
		oobnum=oobnum+tmpnum;
	}


	str.Format("The checknum is %x",oobnum);
	AddInfo(device,str,1);

	Sleep(100);
	if(SW80SSDWriteLoad(device,vendorlba,oobnum,DATABuffer)==0)
	{
		AddInfo(device,"Write load fw failed !",3);
		free(readbuf);
		free(DATABuffer);
		return 0;
	}
	AddInfo(device,"Write loader fw successfully!",1);

	memset(readbuf, 0, datasize);
	Sleep(100);
	if(SW80SSDReadLoad(device,vendorlba,readbuf)==0)
	{
		AddInfo(device,"Read loader fw failed !",3);
		free(readbuf);
		free(DATABuffer);
		return 0;
	}
	
	str.Format("offset: %x  read:%x",0x1820,readbuf[0x1820]);
	str.Format("offset: %x  read:%x",0x2025,readbuf[0x2025]);
	AddInfo(device,str,3);


	BOOL Cmpflag=0;
	for(i=0;i<64*512;i++)
	{
		if(readbuf[i]!=DATABuffer[i])
		{
			str.Format("offset: %x  w: %x   r:%x",i,DATABuffer[i],readbuf[i]);
			AddInfo(device,str,3);
			Cmpflag=1;
		}
	}

	if(Cmpflag)
	{
		free(readbuf);
		free(DATABuffer);
		AddInfo(device,"Compare loader fw failed!",3);
		return 0;
	}

	AddInfo(device,"Compare loader fw successfully!",1);
	free(readbuf);
	free(DATABuffer);
	return 1;
}

BOOL SW80SSDReadDiskinfo(int device,ULONGLONG vendorlba,BYTE *buf)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=0xA4;
	if(WriteSectors(device,vendorlba,1,buffer)==0)
		return 0;
	Sleep(50);
	ret=ReadSectors(device,vendorlba,1,buf);
	return ret;
}

BOOL SW80WriteFWInfo(int device,PUCHAR DATABuffer,long offset)	
{
	long addptr;
	unsigned int i,j;
	int respageperpb;

	j=SLWFlashIDPos(device);
	CString str;
	str.Format("ID POS : %d",j);
	AddInfo(device,str,1);


	str.Format("FLASHNum  : %d",FLASHNum);
	AddInfo(device,str,1);
//	if(j>=FLASHNum)
	if(FlashEndurance[j].pagesize==0)
		return 0;
	
	addptr=offset;
	//如果channel1存在，访问控制块的地址选择<channel 1, die 0, block 0>,如果只有channel 0，则从channel 0, die 0中挑选一个好块
	if(channel>=2)	
	{
		DATABuffer[0x20+addptr]=1;		
		DATABuffer[0x21+addptr]=0;
		DATABuffer[0x22+addptr]=0;
		DATABuffer[0x23+addptr]=0;
		AcessControlblock=0;
	}
	else 
	{
		DATABuffer[0x20+addptr]=0;		
		DATABuffer[0x21+addptr]=0;		
		AcessControlblock=FWBlockNum+2;
		for(i=0;i<128;i++)
		{
			if(VerifyBadblock(device,0,0,AcessControlblock)==0)
				break;
			else
				AcessControlblock=AcessControlblock+2;
		}
		DATABuffer[0x22+addptr]=AcessControlblock;		
		DATABuffer[0x23+addptr]=0;	
	}

	//User partition LBA offset,不支持访问控制，则此处填0
	if(AcessControlFlag==1)
	{
		DATABuffer[0x24+addptr]=PartionSize;		
		DATABuffer[0x25+addptr]=PartionSize>>8;
		DATABuffer[0x26+addptr]=PartionSize>>16;
		DATABuffer[0x27+addptr]=PartionSize>>24;
		DATABuffer[0x28+addptr]=PartionSize;		
		DATABuffer[0x29+addptr]=PartionSize>>8;
		DATABuffer[0x2A+addptr]=PartionSize>>16;
		DATABuffer[0x2B+addptr]=PartionSize>>24;
	}

	DATABuffer[0x2E+addptr]=Purgetime*1000;
	DATABuffer[0x2F+addptr]=Purgetime*1000>>8;
	unsigned int d;
	d=FunCtlWord & 0x0008;
	if(d==8)
	{
		DATABuffer[0x24+addptr]=PartionSize;		
		DATABuffer[0x25+addptr]=PartionSize>>8;
		DATABuffer[0x26+addptr]=PartionSize>>16;
		DATABuffer[0x27+addptr]=PartionSize>>24;
		DATABuffer[0x28+addptr]=MirrorSize;		
		DATABuffer[0x29+addptr]=MirrorSize>>8;
		DATABuffer[0x2A+addptr]=MirrorSize>>16;
		DATABuffer[0x2B+addptr]=MirrorSize>>24;
	}
	//密码重试次数
	DATABuffer[0x32+addptr]=Retrycnt;
	DATABuffer[0x33+addptr]=Retrycnt>>8;
	//还原盘重启生效标志
	DATABuffer[0x34+addptr]=Restartflag;
	DATABuffer[0x35+addptr]=Restartflag>>8;

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

	if(flash_type[device].isMLC==0)
		DATABuffer[0x80+addptr]=0;	/*使用MLC标志*/
	else
		DATABuffer[0x80+addptr]=1;

	DATABuffer[0x82+addptr]=EnableTwoplane;		//使用two plane标志
	DATABuffer[0x84+addptr]=1;		//使用SATA接口标志
	DATABuffer[0x86+addptr]=ECCThreshold;	//ECC Alarm Threshold
	DATABuffer[0x88+addptr]=flash_type[device].ID[0];		//Flash vender
	DATABuffer[0x8A+addptr]=Totalbadblock[device];		//原始坏块总数
	DATABuffer[0x8B+addptr]=Totalbadblock[device]>>8;
	DATABuffer[0x8C+addptr]=FunCtlWord;
	DATABuffer[0x8D+addptr]=FunCtlWord>>8;

	double n;
	int pageoffset;
	n=log((double)(flash_type[device].pages_per_block))/log(2.0);
	pageoffset=n;
	if(n-pageoffset>0)
		pageoffset++;
	DATABuffer[144+addptr]=pageoffset;  //每个block含有的page数偏移

	int sectoroffsetpage;
	n=log((double)flash_type[device].sectors_per_page)/log(2.0);
	sectoroffsetpage=n;
	if(n-sectoroffsetpage>0)
		sectoroffsetpage++;
	DATABuffer[146+addptr]=sectoroffsetpage;           //page含有的sector的个数偏移

	int blockoffsetdie;
	n=log((double)flash_type[device].blocknum)/log(2.0);
	blockoffsetdie=n;
	if(n-blockoffsetdie>0)
		blockoffsetdie++;
	DATABuffer[148+addptr]=blockoffsetdie;		//每个DIE下block的个数偏移, 12，表示4096 block每个DIE

	int pudie;

	DATABuffer[152+addptr]=flash_id[device][0][2]&0x01;			//每个CE下DIE个数偏移，1，表示每个CE下有两个DIE(以上两个值相等，DIE interleave 置1 否则置0)	
	
	int ceoffset;
	ceoffset=pow(2.0,DATABuffer[152+addptr]);
	n=log((double)(grdie[device]/ceoffset))/log(2.0);
	ceoffset=n;
	if(n-ceoffset>0)
		ceoffset++;
	
	DATABuffer[154+addptr]=ceoffset;	//每个Channel下CE个数偏移，1，表示每个Channel下有两个CE

	DATABuffer[156+addptr]=channel/2;	//Channel 数偏移，2，表示共有4个channel
	if(channel==3)
		DATABuffer[156+addptr]=2;	//Channel 数偏移，2，表示共有4个或者3个channel

	DATABuffer[158+addptr]=Resratio;	//保留比例，以x/128为单位, 1，表示每个PU共保留1/128块为free块，该参数要求做成可人工配置


	if(EnableNewrule)
	{
		respageperpb=Respage*flash_type[device].pages_per_block/64;
		MaxPU=(flash_type[device].size_page*1024-MaptableOffset)*2/(3*(flash_type[device].pages_per_block-respageperpb));
		if(MaxPU>=grdie[device]*channel)
		{
			MaxPU=grdie[device]*channel;
			Mapphyblock=(flash_type[device].size_page*1024-MaptableOffset)*2/(3*MaxPU*(flash_type[device].pages_per_block-respageperpb));
		}
		else
			Mapphyblock=1;

		DATABuffer[162+addptr]=Mapphyblock;		//每个logic block映射到的物理块的个数
		DATABuffer[160+addptr]=respageperpb*Mapphyblock; 	//每个logic block保留的page数
	}
	else
	{
		respageperpb=Respage*flash_type[device].pages_per_block/64;
		if(flash_type[device].pages_per_block==64)
			Mapphyblock=2;
		else
			Mapphyblock=1;
		
		unsigned int e;
		e=FunCtlWord & 0x0010;	

		if(EnableTwoplane==1 && e==0x10)
			MaxPU=(flash_type[device].size_page*2*1024-512)*2/(3*(flash_type[device].pages_per_block-respageperpb)*Mapphyblock);
		else
			MaxPU=(flash_type[device].size_page*1024-512)*2/(3*(flash_type[device].pages_per_block-respageperpb)*Mapphyblock);

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
		if(MaxPU>grdie[device]*channel)
			MaxPU=grdie[device]*channel;
		DATABuffer[162+addptr]=Mapphyblock;		//每个logic block映射到的物理块的个数
		DATABuffer[160+addptr]=respageperpb*Mapphyblock; 	//每个logic block保留的page数
	}



	DATABuffer[164+addptr]=Incphyblock;		//每个logic block下总共可以挂的物理块的个数，用户手动输入

	DATABuffer[166+addptr]=flash_type[device].blocknum;		//每个DIE下的block数
	DATABuffer[167+addptr]=flash_type[device].blocknum>>8;			
	
	if(grdie[device]*channel>MaxPU)
		pudie=grdie[device]*channel/MaxPU;
	else
		pudie=1;


	//每个PU下DIE的个数偏移, 0, 表示每个PU下1个DIE
	DATABuffer[150+addptr]=(pudie>>1);		

//#if _INITD 	
	str.Format("BCH : %X",FlashEndurance[j].NFCBCH);
	AddInfo(device,str,1);

	str.Format("Sparesize : %X",FlashEndurance[j].Sparesize);
	AddInfo(device,str,1);

	str.Format("Pagemark0 : %X",FlashEndurance[j].Pagemark0);
	AddInfo(device,str,1);

	str.Format("Pagemark1 : %X",FlashEndurance[j].Pagemark1);
	AddInfo(device,str,1);
		
	str.Format("Bytemark : %X",FlashEndurance[j].Bytemark);
	AddInfo(device,str,1);
	
	str.Format("Totalbadblock : %X",Totalbadblock[device]);
	AddInfo(device,str,1);
	
	str.Format("pageoffsetblock : %X",pageoffset);
	AddInfo(device,str,1);
	
	str.Format("page含有的sector的个数偏移 : %X",sectoroffsetpage);
	AddInfo(device,str,1);

	str.Format("blockoffsetdie : %X",blockoffsetdie);
	AddInfo(device,str,1);
	
	str.Format("每个CE下DIE个数偏移 : %X",DATABuffer[152+addptr]);
	AddInfo(device,str,1);	

	str.Format("每个Channel下CE个数偏移 : %X",DATABuffer[154+addptr]);
	AddInfo(device,str,1);	
	
	str.Format("Channel 数偏移 : %X",DATABuffer[156+addptr]);
	AddInfo(device,str,1);	
	
	str.Format("保留比例，以x/128为单位 : %X",DATABuffer[158+addptr]);
	AddInfo(device,str,1);	

	str.Format("每个logic block保留的page数 : %X",DATABuffer[160+addptr]);
	AddInfo(device,str,1);	
	
	str.Format("每个logic block映射到的物理块的个数 : %X",DATABuffer[162+addptr]);
	AddInfo(device,str,1);	

	str.Format("每个PU下DIE的个数偏移 : %X",DATABuffer[150+addptr]);
	AddInfo(device,str,1);	

	str.Format("功能控制字 : %X",FunCtlWord);
	AddInfo(device,str,1);	

	str.Format("two plane 支持: %X",EnableTwoplane);
	AddInfo(device,str,1);	
//#endif

	unsigned long lcapa;
	DWORD tmp;
	ULONGLONG endurblock=0;
	int blockcnt;
	blockcnt=(flash_type[device].blocknum/1024)*1024;  //取整，保证计算block数是1024的整数倍
	
	if(channel==3)
		DATABuffer[0xa8+addptr]=channel;

	DATABuffer[176+addptr]=grdie[device]*channel*blockcnt;
	DATABuffer[177+addptr]=(grdie[device]*channel*blockcnt)>>8;	//总物理块数
	DATABuffer[178+addptr]=(grdie[device]*channel*blockcnt)>>16;	//总物理块数
	DATABuffer[179+addptr]=(grdie[device]*channel*blockcnt)>>24;	//总物理块数

	/*计算block总擦除次数*/

	endurblock=grdie[device]*channel*blockcnt*FlashEndurance[j].Endurance;
	if(EnableTwoplane==1)
		endurblock=endurblock/2;		//打开two plane的情况下，介质的寿命缩小0.5 

	DATABuffer[0xDC+addptr]=endurblock;
	DATABuffer[0xDD+addptr]=endurblock>>8;	
	DATABuffer[0xDE+addptr]=endurblock>>16;	
	DATABuffer[0xDF+addptr]=endurblock>>24;	

	DATABuffer[0xE0+addptr]=endurblock>>32;
	DATABuffer[0xE1+addptr]=endurblock>>40;	
	DATABuffer[0xE2+addptr]=endurblock>>48;	
	DATABuffer[0xE3+addptr]=endurblock>>56;	

	tmp=grdie[device]*channel*blockcnt*DATABuffer[158+addptr]/128;
	DATABuffer[180+addptr]=tmp;
	DATABuffer[181+addptr]=tmp>>8;	//总保留的物理块数
	DATABuffer[182+addptr]=tmp>>16;	
	DATABuffer[183+addptr]=tmp>>24;							
	
	tmp=grdie[device]*channel*blockcnt*(128-DATABuffer[158+addptr])/(DATABuffer[162+addptr]*128);			
	DATABuffer[184+addptr]=tmp;
	DATABuffer[185+addptr]=tmp>>8;	//总逻辑块数
	DATABuffer[186+addptr]=tmp>>16;	
	DATABuffer[187+addptr]=tmp>>24;		
	
	lcapa=tmp*(flash_type[device].pages_per_block*DATABuffer[162+addptr]-DATABuffer[160+addptr])*flash_type[device].sectors_per_page;
	DATABuffer[188+addptr]=lcapa;
	DATABuffer[189+addptr]=lcapa>>8;	
	DATABuffer[190+addptr]=lcapa>>16;	
	DATABuffer[191+addptr]=lcapa>>24;		//给用户的最大LBA

	if(SetCapacity>0)
	{
		DATABuffer[0x0d8+addptr]=SetCapacity;
		DATABuffer[0x0d9+addptr]=SetCapacity>>8;	
		DATABuffer[0x0da+addptr]=SetCapacity>>16;	
		DATABuffer[0x0db+addptr]=SetCapacity>>24;		//最大磁盘LBA值
	}

	/*写DIEMAP[device]信息*/
	for(i=0;i<grdie[device];i++)
	{
		DATABuffer[addptr++] = DIEMAP[device][i];
		DATABuffer[addptr++] = DIEMAP[device][i] >> 8;
	}

	return 1;
}

BOOL SW80WritesmartInfo(int device,PUCHAR DATABuffer,long offset)	
{
	int i;
	long addr;
	addr=offset;
	int checksum=0;

	/*SMART READ DATA*/
	DATABuffer[addr]=0x01;
	DATABuffer[addr+1]=0x01;
		
	for(i=0;i<24;i++)
	{

		DATABuffer[addr+2+12*i]=SmartData[i].id;
		/*Smart Flags*/
		if(i==13)
		{
			DATABuffer[addr+3+12*i]=0x07;
			DATABuffer[addr+4+12*i]=0x0;
		}
		else
		{
			DATABuffer[addr+3+12*i]=0x06;
			DATABuffer[addr+4+12*i]=0x0;
		}
		
		DATABuffer[addr+5+12*i]=SmartData[i].currentval;
		DATABuffer[addr+6+12*i]=SmartData[i].worstval;
	}

	for(i=0;i<511;i++)
	{
		checksum+=DATABuffer[addr+i];
	}
	DATABuffer[addr+511]=~checksum+1;


	/*SMART read threshold*/
	addr=offset+512;
	DATABuffer[addr]=0x01;
	DATABuffer[addr+1]=0x01;		
	for(i=0;i<24;i++)
	{
		DATABuffer[addr+2+12*i]=SmartData[i].id;
		DATABuffer[addr+3+12*i]=SmartData[i].thresholdval;
	}
	checksum=0;
	for(i=0;i<511;i++)
	{
		checksum+=DATABuffer[addr+i];
	}
	DATABuffer[addr+511]=~checksum+1;

	return 1;
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
			AddInfo(device,"Enter VND failed in ram",3);
			return 0;
		}
		else
			AddInfo(device,"Enter VND successfully in ram",1);
	}
	else
	{
		if(SW80VenderMode(device,1)==0)	
		{
			AddInfo(device,"Enter VND failed in guc",3);
			return 0;
		}
		else
			AddInfo(device,"Enter VND successfully in guc",1);
	}
	return 1;

}

BOOL DownSRAM(int device,CString file) 
{
	CString strDlgFile,str;
	PUCHAR DATABuffer;
	int datasize = 64*512;	

	DATABuffer=(PUCHAR)malloc(datasize);
	memset(DATABuffer, 0, datasize);
#if _INITD
	CFileDialog dlgLoad(								
						TRUE, 0, 0,
						OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
						"Intel Hex(*.bin)|*.bin||");

	if(dlgLoad.DoModal() != IDOK)
	{
		free(DATABuffer);
		return 0;		
	}		
	strDlgFile = dlgLoad.GetPathName();
#endif

#if _MASSINIT 
	char filename[256];
	GetModuleFileName(NULL,filename,256); 
	(strrchr(filename,'\\'))[1] = 0;
	strDlgFile=filename;

	strDlgFile+=file;
#endif

	CFile fl;
	if( !fl.Open(strDlgFile, CFile::modeRead, NULL) )
	{
		str.Format("Open file %s failed!",strDlgFile);
		AddInfo(device,str,3);
		free(DATABuffer);
		return 0;		
	}

	if(fl.GetLength() > datasize )
	{
		AddInfo(device,"File size should below 32KB !",3);
		free(DATABuffer);
		return 0;	
	}

	if( !fl.Read(DATABuffer, fl.GetLength()) )
	{
		str.Format("Read file %s failed!",strDlgFile);
		AddInfo(device,str,3);
		free(DATABuffer);
		fl.Close();
		return 0;	
	}
	fl.Close();

	if(SSdlistItem[device].DriveStatus==1)
	{
		if(SW80WriteRam(device,FW_POS)==0)
		{
			AddInfo(device,"Down FW failed",3);
			free(DATABuffer);
			return 0;
		}
		Sleep(50);

		if(WriteSectors(device,0,64,DATABuffer)==0)
		{
			AddInfo(device,"Down FW failed",3);
			free(DATABuffer);
			return 0;
		}

		Sleep(50);
		if(SW80RunFW(device,FW_POS)==0)
		{
			AddInfo(device,"Run FW failed",3);
			free(DATABuffer);
			return 0;
		}
		else
			AddInfo(device,"Run FW success",1);
	}

	else
	{
		if(SW80RunSWFW(device, DATABuffer)==0)
		{
			AddInfo(device,"Run FW failed",3);
			free(DATABuffer);
			return 0;
		}
		else
			AddInfo(device,"Run FW success",1);
	}

	free(DATABuffer);
#if _INITD
	CloseHandle(Devicehandle[device]);
	Devicehandle[device] = INVALID_HANDLE_VALUE;
#endif
	return 1;
		
}
	
BOOL OutVND(int device) 
{
	unsigned char result;
	ULONGLONG vendorlba;

	vendorlba=SW80GetVendorLBA(device);
	if(SW80ReadCMDStatus(device,vendorlba,&result)>0)
	{
		if(SW80SSDVenderMode(device,vendorlba,1)==0)
		{
			AddInfo(device,"Out VND failed",3);
			return 0;
		}
		else
			AddInfo(device,"Out VND successfully",1);
	}
	else
	{

		if(SW80VenderMode(device,0)==0)	
		{
			AddInfo(device,"Out VND failed",3);
			return 0;
		}
		else
			AddInfo(device,"Out VND successfully",1);	
	}

	return 1;
}

BOOL ReadFlashID(int device)
{
	CString str;
	BYTE buf[512];
	int ldie;
	BYTE lchannel;

	int diepercs=1;
	int locgrdie=0;
	int locgcs=0;



	diepercs=1;
	for(lchannel = 0; lchannel < channel; lchannel++)
	{
	
		for(ldie = 0; ldie < gldie; ldie++)
		{

			if( (ldie<16&&ldie>7) || (ldie<32&&ldie>23) )
				continue;
			if(SW80ReadFlashid(device,lchannel,ldie,buf))
			{
				str.Format("Read ID: Channel:%d cs:%d = %02X %02X %02X %02X",lchannel, locgcs%8,  
								buf[0], buf[1], buf[2], buf[3]);
				AddInfo(device,str,1);
				memcpy(&(flash_id[device][lchannel * gldie + ldie ][0]), buf, 4);
				memcpy(&(flash_id[device][lchannel * gldie + ldie+8][0]), buf, 4);
				if(buf[0] == 0xEC || buf[0] == 0x89 || buf[0] == 0x2C || buf[0]==0x98 || buf[0]==0x45)
				{
					locgrdie=locgrdie+2;
				}

				if(buf[0] == 0xEC || buf[0] == 0x89 || buf[0] == 0x2C || buf[0]==0x98 || buf[0]==0x45)
				{
					diepercs=flash_id[device][0][2]&0x01;
				}
				locgcs++;
			}
			else
			{
				str.Format("Read ID: Channel:%d cs:%d failed",lchannel, locgcs%8);
				AddInfo(device,str,3);
				return 0;
			}
				
		}
	}

	locgrdie=locgrdie/channel;

	if(diepercs==0)
	{
		locgrdie=locgrdie/2;
	}
	grdie[device]=locgrdie;

	if(ReadflashPara(device)==0)
	{
		AddInfo(device,"FLASH ID is error",3);
		return 0;
	}

	str.Format("FLASH ID: %02X %02X %02X %02X", flash_type[device].ID[0], flash_type[device].ID[1], flash_type[device].ID[2], flash_type[device].ID[3]);
	AddInfo(device,str,1);

	str.Format("Page size: %d K \r\nBlock size: %d K", flash_type[device].size_page, flash_type[device].size_block);
	AddInfo(device,str,1);

	str.Format("Pages Per Block: %d ", flash_type[device].pages_per_block);
	AddInfo(device,str,1);

	str.Format("Sectors Per Page: %d ", flash_type[device].sectors_per_page);
	AddInfo(device,str,1);

	str.Format("Total Block: %d ", flash_type[device].total_blocks);
	AddInfo(device,str,1);

	str.Format("TYPE MLC: %d ", flash_type[device].isMLC );
	AddInfo(device,str,1);

	str.Format("Chip Capactiy: %dG ", flash_type[device].cap_in_gb );
	AddInfo(device,str,1);

	str.Format("Die per channel: %d ", locgrdie );
	AddInfo(device,str,1);

	/*	判断是Intel 还是三星的flash*/
	if(flash_type[device].ID[0]==0x89)
		manufacture="INTEL";
	else if(flash_type[device].ID[0]==0xEC)
		manufacture="SAMSUNG";
	else if(flash_type[device].ID[0]==0x2C)
		manufacture="Micron";
	else if(flash_type[device].ID[0]==0x98)
		manufacture="TOSHIBA";
	else if(flash_type[device].ID[0]==0x45)
		manufacture="SanDisk";

	if(diepercs==0)
		gcs[device]=grdie[device];
	else
		gcs[device]=grdie[device]/2;
	str.Format("CS per channel: %d ", gcs[device] );
	AddInfo(device,str,1);	
	
	return true;
}

BOOL SetTwoPlane(int device)
/*
two plane: 需要做成可兼容用户配置和自动配置,如果EnableTwoplane读入值是2，就自动配置

先根据当前配置算出可以支持的最大容量
  8*256 最大512G
  8*128/4*256/4*128  最大256G
  4*64  最大128G
  其它配置暂不支持
*/
{
	int GCap=0;
	DWORD RGCap=0;


	if(flash_type[device].size_page==8 && flash_type[device].pages_per_block==256)
		GCap=512;
	else if(flash_type[device].size_page==4 && flash_type[device].pages_per_block==128)
		GCap=256;
	else if(flash_type[device].size_page==4 && flash_type[device].pages_per_block==256)
		GCap=256;
	else if(flash_type[device].size_page==8 && flash_type[device].pages_per_block==128)
		GCap=256;
	else if(flash_type[device].size_page==4 && flash_type[device].pages_per_block==64)
		GCap=128;

	int blocksize=flash_type[device].size_block/128;
	RGCap=channel*grdie[device]*plane*blocksize*flash_type[device].blocknum/(4*4096);

	CapcityGB=RGCap;
	// 设置large mode，如果容量>=64G则enable，否则disable
	if(RGCap>=64)
		FunCtlWord=FunCtlWord | 0x10;


	if(EnableTwoplane!=2)
		return 1;

	if( flash_type[device].plane_per_die==1 )
	{
		if( RGCap>=GCap )
			return 0;
		else 
			EnableTwoplane=0;
	} 
      
	else
    {
       	if ( RGCap==GCap )
			EnableTwoplane=1;
           
         else
         {
			 if( flash_type[device].size_page == 8 || flash_type[device].ID[0]==0xEC || flash_type[device].isMLC==0 )
            {
               if( RGCap<=32)
                  EnableTwoplane=1;
               else
                  EnableTwoplane=0;
             }
             else // IM 4K page MLC
                  EnableTwoplane=1;
          }
    }
	
	return 1;

}

BOOL ReadflashPara(int device)
{
	int diepercs=1;
	int i=16;
	int ldie;
	int dienum=0;
	int idflag[4];

	i=SLWFlashIDPos(device);
	if(FlashEndurance[i].pagesize==0)
		return 0;

	if(FlashEndurance[i].Planeperdie==1 && EnableTwoplane==1)
	{
		AddInfo(device,"The chip is not supported two plane!",3);
		return 0;
	}
	flash_type[device].plane_per_die=FlashEndurance[i].Planeperdie;
	if(flash_id[device][0][0] == 0xEC)
	{
		memcpy(flash_type[device].ID, &(flash_id[device][0][0]),4);

		flash_type[device].size_page = FlashEndurance[i].pagesize;
		flash_type[device].size_block = FlashEndurance[i].blocksize;
		flash_type[device].blocknum = FlashEndurance[i].blocknum;
		flash_type[device].isMLC =  FlashEndurance[i].Flashtype;

		//判断一个片选有几个DIE
		diepercs=flash_type[device].ID[2]&0x01;

		flash_type[device].cap_in_gb = FlashEndurance[i].blocknum*FlashEndurance[i].blocksize*SW80GetDIEnumPerChip(device)/(1024*1024);
		if(diepercs==0)
			flash_type[device].cap_in_gb=flash_type[device].cap_in_gb/2;
		
		flash_type[device].pages_per_block = flash_type[device].size_block / flash_type[device].size_page;
		flash_type[device].sectors_per_page = (flash_type[device].size_page * 1024) / 512;
		flash_type[device].sectors_per_block = flash_type[device].sectors_per_page * flash_type[device].pages_per_block;
		flash_type[device].total_blocks = (flash_type[device].cap_in_gb * 1024 * 1024) / flash_type[device].size_block;		 
	

		//判断4个channel的id是否相同,并得到DIEMAP[device]
		for(ldie=0;ldie<gldie;ldie++)
		{

			//当每个片选只有一个die的时候，按照现行结构，一个channel只有8个die
			if(diepercs==0&&ldie>=8)
				continue;

			idflag[0]=memcmp(flash_id[device][ldie],flash_id[device][gldie+ldie],4);
			idflag[1]=memcmp(flash_id[device][ldie],flash_id[device][2*gldie+ldie],4);
			idflag[2]=memcmp(flash_id[device][ldie],flash_id[device][3*gldie+ldie],4);
			for(i=0;i<channel-1;i++)
			{
				if(idflag[i])
				{
					AddInfo(device,"4个channel的FLASH ID不对称，不能初始化！",3);
					return 0;
				}
			}
			if(flash_id[device][ldie][0]==0xEC)
			{
				DIEMAP[device][dienum]=ldie;
				dienum++;
			}			
		}
		

		return 1;
	}

	else if(flash_id[device][0][0] == 0x89 || flash_id[device][0][0] == 0x2C || flash_id[device][0][0] == 0x98 || flash_id[device][0][0] == 0x45)
	{
		memcpy(flash_type[device].ID, &(flash_id[device][0][0]),4);		
		diepercs=flash_type[device].ID[2]&0x01;

		flash_type[device].cap_in_gb = FlashEndurance[i].blocknum*FlashEndurance[i].blocksize*SW80GetDIEnumPerChip(device)/(1024*1024);
		if(diepercs==0)
			flash_type[device].cap_in_gb=flash_type[device].cap_in_gb/2;

		flash_type[device].isMLC =  FlashEndurance[i].Flashtype;
		flash_type[device].size_page = FlashEndurance[i].pagesize;
		flash_type[device].size_block = FlashEndurance[i].blocksize;
		flash_type[device].blocknum = FlashEndurance[i].blocknum;

		flash_type[device].pages_per_block = flash_type[device].size_block / flash_type[device].size_page;
		flash_type[device].sectors_per_page = (flash_type[device].size_page * 1024) / 512;
		flash_type[device].sectors_per_block = flash_type[device].sectors_per_page * flash_type[device].pages_per_block;
		flash_type[device].total_blocks = (flash_type[device].cap_in_gb * 1024 * 1024) / flash_type[device].size_block;		 
		//判断4个channel的id是否相同,并得到DIEMAP[device]
		for(ldie=0;ldie<gldie;ldie++)
		{

			//当每个片选只有一个die的时候，按照现行结构，一个channel只有8个die
			if(diepercs==0&&ldie>=8)
				continue;
			idflag[0]=memcmp(flash_id[device][ldie],flash_id[device][gldie+ldie],4);
			idflag[1]=memcmp(flash_id[device][ldie],flash_id[device][2*gldie+ldie],4);
			idflag[2]=memcmp(flash_id[device][ldie],flash_id[device][3*gldie+ldie],4);
			for(i=0;i<channel-1;i++)
			{
				if(idflag[i])
				{
					AddInfo(device,"4个channel的FLASH ID不对称，不能初始化！",3);
#if _MASSINIT 
					return 0;
#endif								
				}
			}

			if(flash_id[device][ldie][0]==0x89 || flash_id[device][ldie][0] == 0x2C || flash_id[device][ldie][0] == 0x98 || flash_id[device][ldie][0] == 0x45)
			{
				DIEMAP[device][dienum]=ldie;
				dienum++;
			}			
		}

		return 1;
	}
	else
	{
		AddInfo(device,"ID is error",3);
		return false;	
	}
}


BOOL Updateloader(int device,int flag) 
{
	unsigned char result;
	ULONGLONG vendorlba;
	BOOL ret=0;
	CString strDlgFile;

	PIOFlag[device]=0;
#if _INITD 
	CFileDialog dlgLoad(								
						TRUE, 0, 0,
						OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
						"Intel Hex(*.bin)|*.bin||");

	if(dlgLoad.DoModal() != IDOK)
	{
		return 0;		
	}		
	strDlgFile = dlgLoad.GetPathName();
#endif

#if _UPDATE 
	char filename[256];
	GetModuleFileName(NULL,filename,256); 
	(strrchr(filename,'\\'))[1] = 0;
	strDlgFile=filename;
	strDlgFile+=LoadFilename;
#endif

#if _MASSINIT 
	char filename[256];
	GetModuleFileName(NULL,filename,256); 
	(strrchr(filename,'\\'))[1] = 0;
	strDlgFile=filename;
	strDlgFile+=LoadFilename;
#endif

	vendorlba=SW80GetVendorLBA(device);
	if(SW80ReadCMDStatus(device,vendorlba,&result)>0)
	{
		ret=SW80SSDUpdateLoad(device,vendorlba,strDlgFile);
		if(ret==0)
		{
			AddInfo(device,"Retry update loader fw !",1);
			PIOFlag[device]=1;
			ret=SW80SSDUpdateLoad(device,vendorlba,strDlgFile);
		}
	}
	else
	{
		ret=SW80UpdateLoad(device,strDlgFile,flag);
		if(ret==0)
		{
			AddInfo(device,"Retry update loader fw !",1);
			PIOFlag[device]=1;
			SW80UpdateLoad(device,strDlgFile,flag);
		}
	}

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
    sptwb.Spt.DataTransferLength = IDENTIFY_BUFFER_SIZE;
    sptwb.Spt.TimeOutValue = 3;
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


BOOL ata_via_scsi_write_dma(int device, BYTE target,LONGLONG dwStartSector, WORD wSectors, BYTE *data)
{
	BOOL	bRet;
	DWORD	dwReturned;
	DWORD	length,errorcode;

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

	sptwb.Spt.CdbLength = 16;
	sptwb.Spt.Cdb[0] = 0x85;
	sptwb.Spt.Cdb[1] = (0x6 << 1) | 1; //MULTIPLE_COUNT=0,PIO OUT=5,Reserved
	sptwb.Spt.Cdb[2] = (0 << 3) | (1 << 2) | 2;//OFF_LINE=0,CK_COND=0,Reserved=0,T_DIR=1(ToDevice),BYTE_BLOCK=1,T_LENGTH=2
	sptwb.Spt.Cdb[4] = 0;//FEATURES (7:0)
	sptwb.Spt.Cdb[6] = wSectors;//SECTOR_COUNT (7:0)
	sptwb.Spt.Cdb[8] = dwStartSector;//LBA_LOW (7:0)
	sptwb.Spt.Cdb[10] = dwStartSector>>8;//LBA_MID (7:0)
	sptwb.Spt.Cdb[12] = dwStartSector>>16;//LBA_HIGH (7:0)
	sptwb.Spt.Cdb[13] =0x40 ;//DriveHeadReg (4:0)
	sptwb.Spt.Cdb[14] = 0x35;//COMMAND
	sptwb.Spt.Cdb[15] = 0;//COMMAND

	sptwb.Spt.Cdb[3] = 0;//FEATURES 
	sptwb.Spt.Cdb[5] = 0;//SECTOR_COUNT 
	sptwb.Spt.Cdb[7] = dwStartSector>>24;//LBA_LOW 
	sptwb.Spt.Cdb[9] = dwStartSector>>32;//LBA_MID 
	sptwb.Spt.Cdb[11] = dwStartSector>>40;//LBA_HIGH 

	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);

	bRet = ::DeviceIoControl(Devicehandle[device], IOCTL_SCSI_PASS_THROUGH_DIRECT, 
		&sptwb, length,
		&sptwb, length,	&dwReturned, FALSE);
	
	if(bRet == FALSE)
	{
		errorcode=GetLastError();
		AddInfo(device,"Write error code 121",3);
//		return	FALSE;
	}
	return	TRUE;
}

BOOL ata_via_scsi_write(int device, BYTE target,LONGLONG dwStartSector, WORD wSectors, BYTE *data)
{
	BOOL	bRet;
	DWORD	dwReturned;
	DWORD	length,errorcode;

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

	//sptwb.Spt.CdbLength = 12;
	//sptwb.Spt.Cdb[0] = 0xA1;
	//sptwb.Spt.Cdb[1] = (0x5 << 1) | 0; //MULTIPLE_COUNT=0,PIO OUT=5,Reserved
	//sptwb.Spt.Cdb[2] = (0 << 3) | (1 << 2) | 2;//OFF_LINE=0,CK_COND=0,Reserved=0,T_DIR=1(ToDevice),BYTE_BLOCK=1,T_LENGTH=2

	//sptwb.Spt.Cdb[3] = 0;//FEATURES (7:0)
	//sptwb.Spt.Cdb[4] = wSectors;//SECTOR_COUNT (7:0)
	//sptwb.Spt.Cdb[5] = dwStartSector;//LBA_LOW (7:0)
	//sptwb.Spt.Cdb[6] = dwStartSector>>8;//LBA_MID (7:0)
	//sptwb.Spt.Cdb[7] = dwStartSector>>16;//LBA_HIGH (7:0)
	//sptwb.Spt.Cdb[8] =(0x4<<4)  | (dwStartSector>>24);//LBA_HIGH (7:0)
	//sptwb.Spt.Cdb[9] = 0x30;//COMMAND

	sptwb.Spt.CdbLength = 16;
	sptwb.Spt.Cdb[0] = 0x85;
	sptwb.Spt.Cdb[1] = (0x5 << 1) | 1; //MULTIPLE_COUNT=0,PIO OUT=5,Reserved
	sptwb.Spt.Cdb[2] = (0 << 3) | (1 << 2) | 2;//OFF_LINE=0,CK_COND=0,Reserved=0,T_DIR=1(ToDevice),BYTE_BLOCK=1,T_LENGTH=2
	sptwb.Spt.Cdb[4] = 0;//FEATURES (7:0)
	sptwb.Spt.Cdb[6] = wSectors;//SECTOR_COUNT (7:0)
	sptwb.Spt.Cdb[8] = dwStartSector;//LBA_LOW (7:0)
	sptwb.Spt.Cdb[10] = dwStartSector>>8;//LBA_MID (7:0)
	sptwb.Spt.Cdb[12] = dwStartSector>>16;//LBA_HIGH (7:0)
	sptwb.Spt.Cdb[13] =0x40 ;//DriveHeadReg (4:0)
	sptwb.Spt.Cdb[14] = 0x34;//COMMAND
	sptwb.Spt.Cdb[15] = 0;//COMMAND

	sptwb.Spt.Cdb[3] = 0;//FEATURES 
	sptwb.Spt.Cdb[5] = 0;//SECTOR_COUNT 
	sptwb.Spt.Cdb[7] = dwStartSector>>24;//LBA_LOW 
	sptwb.Spt.Cdb[9] = dwStartSector>>32;//LBA_MID 
	sptwb.Spt.Cdb[11] = dwStartSector>>40;//LBA_HIGH 

	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);

	bRet = ::DeviceIoControl(Devicehandle[device], IOCTL_SCSI_PASS_THROUGH_DIRECT, 
		&sptwb, length,
		&sptwb, length,	&dwReturned, FALSE);
	
	if(bRet == FALSE)
	{
		errorcode=GetLastError();
		AddInfo(device,"Write error code 121",3);
//		return	FALSE;
	}
	return	TRUE;
}


BOOL ata_via_scsi_read_dma(int device, BYTE target,LONGLONG dwStartSector, WORD wSectors, BYTE *data)
{
	BOOL	bRet;
	DWORD	dwReturned;
	DWORD	length,errorcode;


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

	sptwb.Spt.CdbLength = 16;
	sptwb.Spt.Cdb[0] = 0x85;
	sptwb.Spt.Cdb[1] = (6 << 1) | 1; //MULTIPLE_COUNT=0,PROTOCOL=4(PIO Data-In),Reserved
	sptwb.Spt.Cdb[2] = (1 << 3) | (1 << 2) | 2;//OFF_LINE=0,CK_COND=0,Reserved=0,T_DIR=1(ToDevice),BYTE_BLOCK=1,T_LENGTH=2
	
	sptwb.Spt.Cdb[4] = 0;//FEATURES (7:0)
	sptwb.Spt.Cdb[6] = wSectors;//SECTOR_COUNT (7:0)
	sptwb.Spt.Cdb[8] = dwStartSector;//LBA_LOW (7:0)
	sptwb.Spt.Cdb[10] = dwStartSector>>8;//LBA_MID (7:0)
	sptwb.Spt.Cdb[12] = dwStartSector>>16;//LBA_HIGH (7:0)
	sptwb.Spt.Cdb[13] =0x40 ;//DriveHeadReg (4:0)
	sptwb.Spt.Cdb[14] = 0x25;//COMMAND
	sptwb.Spt.Cdb[15] = 0;//COMMAND

	sptwb.Spt.Cdb[3] = 0;//FEATURES 
	sptwb.Spt.Cdb[5] = 0;//SECTOR_COUNT 
	sptwb.Spt.Cdb[7] = dwStartSector>>24;//LBA_LOW 
	sptwb.Spt.Cdb[9] = dwStartSector>>32;//LBA_MID 
	sptwb.Spt.Cdb[11] = dwStartSector>>40;//LBA_HIGH 
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	bRet = ::DeviceIoControl(Devicehandle[device], IOCTL_SCSI_PASS_THROUGH_DIRECT, 
		&sptwb, length,
		&sptwb, length,	&dwReturned, NULL);
	
	if(bRet == FALSE)
	{
		errorcode=GetLastError();
		AddInfo(device,"Read error code 121",3);
	//	return	FALSE;
	}

	return	TRUE;
}

BOOL ata_via_scsi_read(int device, BYTE target,LONGLONG dwStartSector, WORD wSectors, BYTE *data)
{
	BOOL	bRet;
	DWORD	dwReturned;
	DWORD	length,errorcode;

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
	
	//sptwb.Spt.CdbLength = 12;
	//sptwb.Spt.Cdb[0] = 0xA1;//ATA PASS THROUGH(12) OPERATION CODE(A1h)
	//sptwb.Spt.Cdb[1] = (4 << 1) | 0; //MULTIPLE_COUNT=0,PROTOCOL=4(PIO Data-In),Reserved
	//sptwb.Spt.Cdb[2] = (1 << 3) | (1 << 2) | 2;//OFF_LINE=0,CK_COND=0,Reserved=0,T_DIR=1(ToDevice),BYTE_BLOCK=1,T_LENGTH=2
	//sptwb.Spt.Cdb[3] = 0;//FEATURES (7:0)
	//sptwb.Spt.Cdb[4] = wSectors;//SECTOR_COUNT (7:0)
	//sptwb.Spt.Cdb[5] = dwStartSector;//LBA_LOW (7:0)
	//sptwb.Spt.Cdb[6] = dwStartSector>>8;//LBA_MID (7:0)
	//sptwb.Spt.Cdb[7] = dwStartSector>>16;//LBA_HIGH (7:0)

	//sptwb.Spt.Cdb[8] = (0x4<<4) | dwStartSector>>24;//LBA_HIGH (7:0)
	//sptwb.Spt.Cdb[9] = 0x20;//COMMAND

	sptwb.Spt.CdbLength = 16;
	sptwb.Spt.Cdb[0] = 0x85;
	sptwb.Spt.Cdb[1] = (4 << 1) | 1; //MULTIPLE_COUNT=0,PROTOCOL=4(PIO Data-In),Reserved
	sptwb.Spt.Cdb[2] = (1 << 3) | (1 << 2) | 2;//OFF_LINE=0,CK_COND=0,Reserved=0,T_DIR=1(ToDevice),BYTE_BLOCK=1,T_LENGTH=2
	
	sptwb.Spt.Cdb[4] = 0;//FEATURES (7:0)
	sptwb.Spt.Cdb[6] = wSectors;//SECTOR_COUNT (7:0)
	sptwb.Spt.Cdb[8] = dwStartSector;//LBA_LOW (7:0)
	sptwb.Spt.Cdb[10] = dwStartSector>>8;//LBA_MID (7:0)
	sptwb.Spt.Cdb[12] = dwStartSector>>16;//LBA_HIGH (7:0)
	sptwb.Spt.Cdb[13] =0x40 ;//DriveHeadReg (4:0)
	sptwb.Spt.Cdb[14] = 0x24;//COMMAND
	sptwb.Spt.Cdb[15] = 0;//COMMAND

	sptwb.Spt.Cdb[3] = 0;//FEATURES 
	sptwb.Spt.Cdb[5] = 0;//SECTOR_COUNT 
	sptwb.Spt.Cdb[7] = dwStartSector>>24;//LBA_LOW 
	sptwb.Spt.Cdb[9] = dwStartSector>>32;//LBA_MID 
	sptwb.Spt.Cdb[11] = dwStartSector>>40;//LBA_HIGH 

	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	bRet = ::DeviceIoControl(Devicehandle[device], IOCTL_SCSI_PASS_THROUGH_DIRECT, 
		&sptwb, length,
		&sptwb, length,	&dwReturned, NULL);
	
	if(bRet == FALSE)
	{
		errorcode=GetLastError();
		AddInfo(device,"Read error code 121",3);
	//	return	FALSE;
	}

	return	TRUE;
}

BOOL SW80UpdateFWInfo(int device,PUCHAR DATABuffer,long offset)	
/*将旧版本的FW 运行参数更新为新版本*/
{
	BYTE buf[512];
	ULONGLONG vendorlba;
	
	vendorlba=SW80GetVendorLBA(GlobalDevice);
	if(SW80SSDReadDiskinfo(device,vendorlba,buf))
	{
		memcpy((void*)(DATABuffer+offset),buf,512);

		DATABuffer[offset+0x50]=0xAA;
		DATABuffer[offset+0x51]=0x55;
		DATABuffer[offset+0x52]=0x11;
		DATABuffer[offset+0x53]=0x22;
		DATABuffer[offset+0x60]=0x77;
		DATABuffer[offset+0x61]=0x49;
		DATABuffer[offset+0x62]=0xFF;
		DATABuffer[offset+0x63]=0xFF;
		DATABuffer[offset+0x64]=0x1F;
		DATABuffer[offset+0x65]=0x0B;
		DATABuffer[offset+0x66]=0x0E;
		DATABuffer[offset+0x67]=0x0B;
		DATABuffer[offset+0x68]=0x00;
		DATABuffer[offset+0x69]=0x00;
		DATABuffer[offset+0x6A]=0x0B;
		DATABuffer[offset+0x6B]=0x1F;
		if(buf[0x88]==0xEC)
		{
			DATABuffer[offset+0x6C]=0x00;
			DATABuffer[offset+0x6D]=0x20;
			DATABuffer[offset+0x6E]=0x32;
			DATABuffer[offset+0x6F]=0x42;
			DATABuffer[offset+0x70]=0x7F;
			DATABuffer[offset+0x71]=0x5C;
			DATABuffer[offset+0x72]=0x08;
			DATABuffer[offset+0x73]=0x40;
		}
		else
		{
			DATABuffer[offset+0x6C]=0x00;
			DATABuffer[offset+0x6D]=0x20;
			DATABuffer[offset+0x6E]=0x20;
			DATABuffer[offset+0x6F]=0x42;
			DATABuffer[offset+0x70]=0x7F;
			DATABuffer[offset+0x71]=0x26;
			DATABuffer[offset+0x72]=0x0E;
			DATABuffer[offset+0x73]=0x40;
		}
		DATABuffer[offset+0x9E]=buf[0x9E]*8;
		return 1;
	}
	else
		return 0;
}

BOOL SW80UpdateFWInfoInRAM(int device,PUCHAR DATABuffer,long offset)	
/*在RAMDISK下将旧版本的FW 运行参数更新为新版本*/
{
	DATABuffer[offset+0x50]=0xAA;
	DATABuffer[offset+0x51]=0x55;
	DATABuffer[offset+0x52]=0x11;
	DATABuffer[offset+0x53]=0x22;
	DATABuffer[offset+0x60]=0x77;
	DATABuffer[offset+0x61]=0x49;
	DATABuffer[offset+0x62]=0xFF;
	DATABuffer[offset+0x63]=0xFF;
	DATABuffer[offset+0x64]=0x1F;
	DATABuffer[offset+0x65]=0x0B;
	DATABuffer[offset+0x66]=0x0E;
	DATABuffer[offset+0x67]=0x0B;
	DATABuffer[offset+0x68]=0x00;
	DATABuffer[offset+0x69]=0x00;
	DATABuffer[offset+0x6A]=0x0B;
	DATABuffer[offset+0x6B]=0x1F;
	if(DATABuffer[0x88]==0xEC)
	{
		DATABuffer[offset+0x6C]=0x00;
		DATABuffer[offset+0x6D]=0x20;
		DATABuffer[offset+0x6E]=0x32;
		DATABuffer[offset+0x6F]=0x42;
		DATABuffer[offset+0x70]=0x7F;
		DATABuffer[offset+0x71]=0x5C;
		DATABuffer[offset+0x72]=0x08;
		DATABuffer[offset+0x73]=0x40;
	}
	else
	{
		DATABuffer[offset+0x6C]=0x00;
		DATABuffer[offset+0x6D]=0x20;
		DATABuffer[offset+0x6E]=0x20;
		DATABuffer[offset+0x6F]=0x42;
		DATABuffer[offset+0x70]=0x7F;
		DATABuffer[offset+0x71]=0x26;
		DATABuffer[offset+0x72]=0x0E;
		DATABuffer[offset+0x73]=0x40;
	}
	DATABuffer[offset+0x9E]=DATABuffer[0x9E]*8;
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

BOOL SW80FindGCblock(int device,int *channelnum,int *dienum,int *blocknum)
{
	int i=0;
	BYTE rbuf[512*32];
	int page,gcblock=1;

	memset(rbuf,0,512*32);	

	if(SW80PageRead(device,1,0,0,0,rbuf)!=0)
	{
		if(rbuf[23]==0x51 || rbuf[23]==0x52 )
		{
			*channelnum=1;
			*dienum=0;
			*blocknum=0;
			return 1;
		}
	}
	

	for(i=0;i<10;i++)
	{
		page=flash_type[device].pages_per_block*gcblock;
		if(SW80PageRead(device,0,0,page,0,rbuf)!=0)
		{
			if(rbuf[23]==0x51 || rbuf[23]==0x52 )
			{
				*channelnum=0;
				*dienum=0;
				*blocknum=gcblock;
				return 1;
			}
			else
				gcblock=gcblock+2;
		}
	}
	return 0;
}


int SW80GetDIEnumPerChip(int device)
{
	int i;
	int dieperchip=0;
	int diepos[8];//记录每个chip中的8个die是否为空
	int tmp;

	memset(diepos,0,8*sizeof(int));
	for(i=0;i<64;i++)
	{
		if(flash_id[device][i][0]==0xEC ||flash_id[device][i][0]==0x89 || flash_id[device][i][0] == 0x2C || flash_id[device][i][0] == 0x98 || flash_id[device][i][0] == 0x45)
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

BOOL ConvertData(char *date)
{
	char *token;
	char sep=' ';
	char c[6][32],tmp[8];
	int i,j,day;

	memset(tmp,0,8);
	for(i=0;i<6;i++)
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

BOOL SW80Readchanelnum(int device)
/*
从RAM DISK中读出channel 数
*/
{
	BYTE oob[24],pagebuf[512*32],varbuf[512];
	CString str;
	
	memset(oob,0,24);
	memset(pagebuf,0x5a,512*32);
	memset(varbuf,0x5a,512);
	if(SW80SetBCH(device,1)==0)
	{
		AddInfo(device,"Set  BCH16 mode failed",3);	
		return 0;
	}
	
	if(SW80PageRead(device, 0, 0, 31,0,pagebuf)==0)
	{
		AddInfo(device,"Page read failed !",3);
		return 0;
	}
	memcpy(varbuf,pagebuf+1024,512);

	if(varbuf[0x50]==0xAA && varbuf[0x51]==0x55 && varbuf[0x52]==0x11 && varbuf[0x53]==0x22)
		channel=pow(2.0,(double)varbuf[156]);	//Channel数
	else 
		return 0;


	if(SW80SetBCH(device,0)==0)
	{
		AddInfo(device,"Set  BCH8 mode failed",3);	
		return 0;
	}
	return 1;
}


BOOL SW80ReadRuleTxt(CString File)
{
	FILE *fp;
	int i,num;
	char output[128];
	CString str;

	CHAR FilePath[255]; 

	strcpy(FilePath,LocalPath);
    strcat(FilePath,File);

	fp = fopen(FilePath, "rb");
	if(fp==NULL)
	{
		AfxMessageBox("Open rule file fail!");
		AfxMessageBox(FilePath);
		return 0;
	}

	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&num);
	for(i=0;i<num;i++)
	{
		fscanf(fp,"%X,",&ProduceRule[i].enable_two_plane);	
		fscanf(fp,"%X,",&ProduceRule[i].ECC_alarm_threshold);	
		fscanf(fp,"%X,",&ProduceRule[i].res_ratio);	
		fscanf(fp,"%X,",&ProduceRule[i].FCW);
		fscanf(fp,"%X,",&ProduceRule[i].respage_per_64Pages);
		fscanf(fp,"%X,",&ProduceRule[i].lblock_include_pblock);
	}


	fclose(fp);
	RuleNum = num;
	return 1;
}

BOOL SW80SetSNtoRamdisk(int device,unsigned char* DATABuffer,unsigned char* sid)
{
	int i=0,j,pos=1;

	while(i<63*1024)
	{
		pos=memcmp(DATABuffer+i,"WS0810325476",12);
		if(pos==0)
			break;
		else
		{	
			i++;
		}
	}
	if(pos==0)
	{
		for(j=0;j<12;j++)
		{
			DATABuffer[i+j]=sid[j];		
		}
		
		for(j=0;j<6;j++)
		{
			sid[20]=DATABuffer[i+2*j];
			DATABuffer[i+2*j]=DATABuffer[i+2*j+1];
			DATABuffer[i+2*j+1]=sid[20];
		}
	}
	else
	{
		AddInfo(device,"查找 SSD 序列号失败!",3);	
		return 0;
	}
	return 1;
}

BOOL SW80SaveInterpara(int device, char* file)
{
	BYTE buf[512];
	CString str,str1;
	ULONGLONG vendorlba;
	int ret,ret1;
	unsigned char result;

	vendorlba=SW80GetVendorLBA(device);
	if(SW80ReadCMDStatus(GlobalDevice,vendorlba,&result)<=0)
		return 0;

	if(SW80SSDReadDiskinfo(GlobalDevice,vendorlba,buf))
	{
		ret=SW80SaveNV(file,buf);
		ret1=SW80SaveGC(file,buf);
		if(ret&&ret1)
			return 1;
		else
			return 0;
	}
	else
		return 0;

	
}

BOOL SW80SaveNV(char* file,unsigned char* buf)
{	
	CString m_features[64];
	FILE *fp;
	int i;
	DWORD tmp;
	CString str,str1;

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
	
	fp = fopen(file, "a+");
	if(fp==NULL)
	{
		AfxMessageBox("Open Interpara file fail!");
		return 0;
	}

	for(i=0;i<8;i++)
	{
		tmp=(buf[0x180+4*i+3]<<24)|(buf[0x180+4*i+2]<<16)|(buf[0x180+4*i+1]<<8)|(buf[0x180+4*i]);
		str.Format("%s:0x%X",m_features[i],tmp);
		fprintf(fp,"%s\r\n",str);
	}
	for(i=8;i<12;i++)
	{
		tmp=(buf[0x1A0+2*(i-8)+1]<<8)|(buf[0x1A0+2*(i-8)]);
		str.Format("%s:0x%X",m_features[i],tmp);
		fprintf(fp,"%s\r\n",str);
	}
	
	for(i=0;i<4;i++)
	{
		tmp=(buf[0x1E8+4*i+3]<<24)|(buf[0x1E8+4*i+2]<<16)|(buf[0x1E8+4*i+1]<<8)|(buf[0x1E8+4*i]);
		str.Format("%s:0x%X",m_features[i+12],tmp);
		fprintf(fp,"%s\r\n",str);
	}

	for(i=0;i<16;i++)
	{
		tmp=(buf[0x1A8+4*i+3]<<24)|(buf[0x1A8+4*i+2]<<16)|(buf[0x1A8+4*i+1]<<8)|(buf[0x1A8+4*i]);
		str.Format("0x%X ",tmp);
		str1+=str;
	}
	fprintf(fp,"%s:%s\r\n",m_features[16],str1);
	
	fclose(fp);

	return 1;

}

BOOL SW80SaveGC(char* file,unsigned char* buf)
{
	CString m_features[64];
	FILE *fp;
	int i;
	CString str,str1;
	DWORD tmp;
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

	fp = fopen(file, "a+");
	if(fp==NULL)
	{
		AfxMessageBox("Open Interpara file fail!");
		return 0;
	}

	for(i=0;i<8;i++)
	{
		tmp=(buf[2*i+1]<<8)|(buf[2*i]);
		if(tmp>=0 && tmp!=0xFFFF)
		{
			str.Format("%d ",tmp);
			str1+=str;
		}
	}
	str1+=str;
	fprintf(fp,"%s:%s\r\n",m_features[0],str1);

	for(i=0;i<6;i++)
	{
		tmp=(buf[0x080+2*i+1]<<8)|(buf[0x080+2*i]);
		str.Format("0x%X",tmp);
		fprintf(fp,"%s:%s\r\n",m_features[i+2],str);
	}

	for(i=0;i<11;i++)
	{
		tmp=(buf[0x090+2*i+1]<<8)|(buf[0x090+2*i]);
		str.Format("0x%X",tmp);
		fprintf(fp,"%s:%s\r\n",m_features[i+8],str);
	}

	for(i=0;i<13;i++)
	{

		tmp=(buf[0x0b0+4*i+3]<<24)|(buf[0x0b0+4*i+2]<<16)|(buf[0x0b0+4*i+1]<<8)|(buf[0x0b0+4*i]);
		str.Format("0x%X ",tmp);
		fprintf(fp,"%s:%s\r\n",m_features[i+19],str);
	}

	
	tmp=(buf[0x153]<<8)|(buf[0x152]);
	str.Format("0x%X ",tmp);
	fprintf(fp,"%s:%s\r\n",m_features[32],str);

	for(i=0;i<3;i++)
	{

		tmp=(buf[0x154+4*i+3]<<24)|(buf[0x154+4*i+2]<<16)|(buf[0x154+4*i+1]<<8)|(buf[0x154+4*i]);
		str.Format("0x%X ",tmp);
		fprintf(fp,"%s:%s\r\n",m_features[i+33],str);
	}

	tmp=(buf[0x050+3]<<24)|(buf[0x050+2]<<16)|(buf[0x050+1]<<8)|(buf[0x050]);
	str.Format("0x%X ",tmp);
	fprintf(fp,"%s:%s\r\n",m_features[36],str);

	tmp=(buf[0x074+3]<<24)|(buf[0x074+2]<<16)|(buf[0x074+1]<<8)|(buf[0x074]);
	str.Format("0x%X ",tmp);
	fprintf(fp,"%s:%s\r\n",m_features[37],str);

	tmp=(buf[0x8D]<<8)|(buf[0x8C]);
	str.Format("0x%X ",tmp);
	fprintf(fp,"%s:%s\r\n",m_features[38],str);

	//最后附上没解析的整个sector
	fprintf(fp,"\r\nFW INFO SECTOR\r\n");
	for(i=0;i<512;i++)
	{	
		if(i%16==0)
			fprintf(fp,"\r\n");	
		fprintf(fp,"%02x",buf[i]);
		
	}
	fprintf(fp,"\r\n\r\n");	
	fclose(fp);
	return 1;
}

BOOL SW80VerifyPassword(int device,ULONGLONG vendorlba,BYTE *buf,int length,BOOL Swithflag)
{
	BYTE buffer[512];
	BOOL ret;
	int i;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");
	
	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=0xA9;
	for(i=0;i<length;i++)
		buffer[i+16]=buf[i];
	
//分区切换标志，如果此标志打开，则在密码校验正确后会同时将分区切换到用户分区，如果此标志末打开，则只进行密码校验，返回校验结果，并不切换分区
	buffer[32]=Swithflag;	
	ret=WriteSectors(device,vendorlba,1,buffer);
	return ret;
}

BOOL SW80ChangePassword(int device,ULONGLONG vendorlba,BYTE *oldps,int len1,BYTE *newps,int len2,int bypass)
{
	BYTE buffer[512];
	BOOL ret;
	int i;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");
	
	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=0xAA;
	for(i=0;i<len1;i++)
		buffer[i+16]=oldps[i];
	for(i=0;i<len2;i++)
		buffer[i+32]=newps[i];

	buffer[48]=bypass;
	ret=WriteSectors(device,vendorlba,1,buffer);
	return ret;
}


int  InitDisk(int device)
{
	BOOL bResult = FALSE; // generic results flag   
	HANDLE hDevice; // handle to the drive to be examined   
	DWORD junk = 0; // discard results  
	DWORD ret=0;
	hDevice = CreateFile(TEXT("\\\\.\\PhysicalDrive1"), //"\\\\.\\PhysicalDrive1", // drive to open  
	GENERIC_READ | GENERIC_WRITE, // no access to the drive  
	FILE_SHARE_READ | // share mode  
	FILE_SHARE_WRITE,   
	NULL, // default security attributes  
	OPEN_EXISTING, // disposition  
	0, // file attributes  
	NULL); // do not copy file attributes  
	if (hDevice == INVALID_HANDLE_VALUE) // cannot open the drive  
	{  
		return GetLastError();
	}

	CREATE_DISK dsk;  
	dsk.PartitionStyle = PARTITION_STYLE_MBR;   
	dsk.Mbr.Signature = 9999;

	  // 初始化磁盘
	bResult = DeviceIoControl(hDevice, // device to be queried  
	IOCTL_DISK_CREATE_DISK, // operation to perform  
	&dsk, sizeof(dsk), //sizeof(pdg), // output buffer  
	NULL, 0, // no output buffer  
	&junk, // # bytes returned  
	NULL);  
	if (!bResult)
	{  
		return GetLastError();  
	}
	bResult = DeviceIoControl(hDevice,  
	IOCTL_DISK_UPDATE_PROPERTIES,  
	NULL, 0, NULL, 0, &junk, NULL);
	if (! bResult)
	{  
		return GetLastError();  
	}

	LARGE_INTEGER lgPartitionSize;
	lgPartitionSize.QuadPart = (1024 * 1024 * 1024);
	DWORD dwDriverLayoutInfoExLen = sizeof (DRIVE_LAYOUT_INFORMATION_EX) + 3 * sizeof(PARTITION_INFORMATION_EX);
	DRIVE_LAYOUT_INFORMATION_EX *pdg = (DRIVE_LAYOUT_INFORMATION_EX *)new BYTE[dwDriverLayoutInfoExLen];
	if (pdg == NULL)
	{
		return -1;
	}
	SecureZeroMemory(pdg, dwDriverLayoutInfoExLen);
	// set RewritePartition=true in every partition to force rewrite.   
	// for (int item = 0; item < 4; item++){
	// pdg->PartitionEntry[item].RewritePartition = 1;
	// pdg->PartitionEntry[item].PartitionNumber = 0;
	// }

	pdg->PartitionStyle = PARTITION_STYLE_MBR;  
	pdg->PartitionCount = 1;
	pdg->Mbr.Signature = 99999;

	pdg->PartitionEntry[0].PartitionStyle = PARTITION_STYLE_MBR;   
	pdg->PartitionEntry[0].StartingOffset.QuadPart = 32256; // 63扇区
	pdg->PartitionEntry[0].PartitionLength.QuadPart = lgPartitionSize.QuadPart * 36;   
	pdg->PartitionEntry[0].PartitionNumber = 1;   
	pdg->PartitionEntry[0].RewritePartition = TRUE;   
	pdg->PartitionEntry[0].Mbr.PartitionType = PARTITION_FAT32; // PARTITION_IFS (NTFS partition or logical drive)   
	pdg->PartitionEntry[0].Mbr.BootIndicator = TRUE;
	pdg->PartitionEntry[0].Mbr.RecognizedPartition = 1;   
	pdg->PartitionEntry[0].Mbr.HiddenSectors = 32256 / 512;   

	  // 对磁盘分区
	bResult = DeviceIoControl(hDevice, // device to be queried  
	IOCTL_DISK_SET_DRIVE_LAYOUT_EX, // operation to perform  
	pdg, sizeof DRIVE_LAYOUT_INFORMATION_EX, //sizeof(pdg), // output buffer  
	NULL, 0, // no output buffer  
	&junk, // # bytes returned  
	NULL);  
	if (!bResult)
	{  
		return GetLastError();  
	}
	bResult = DeviceIoControl(hDevice,  
	IOCTL_DISK_UPDATE_PROPERTIES,  
	NULL, 0, NULL, 0, &junk, NULL);
	if (!bResult)
	{  
		return GetLastError();  
	}

	return 0;
}

BOOL ata_reconnect(int device,BYTE *buf)
{
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER     sptdwb;
	BOOL	bRet;
	DWORD	dwReturned;
	DWORD	length,errorcode;

	ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
	sptdwb.Spt.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptdwb.Spt.PathId = 0;
	sptdwb.Spt.TargetId = 1;
	sptdwb.Spt.Lun = 0;
	sptdwb.Spt.CdbLength = 7;
	sptdwb.Spt.DataIn = 0;  //%%%%%%%%%%%%%%%%%%read 1 write 0
	sptdwb.Spt.SenseInfoLength = 24;
	sptdwb.Spt.DataTransferLength = 0;
	sptdwb.Spt.TimeOutValue = 2;//time s
	sptdwb.Spt.DataBuffer = buf;
	sptdwb.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);

	sptdwb.Spt.Cdb[0] = 0xE6;
	sptdwb.Spt.Cdb[1] = 0x00;
	sptdwb.Spt.Cdb[2] = 0x00;
	sptdwb.Spt.Cdb[3] = 0x00;
	sptdwb.Spt.Cdb[4] = 0x00;
	sptdwb.Spt.Cdb[5] = 0x00;
	sptdwb.Spt.Cdb[6] = 0x09;

	
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);

	bRet = ::DeviceIoControl(Devicehandle[device], IOCTL_SCSI_PASS_THROUGH_DIRECT, 
		&sptdwb, length,
		&sptdwb, length,	&dwReturned, FALSE);
	
	if(bRet == FALSE)
	{
		CString str;
		errorcode=GetLastError();
		str.Format("Write error code %d",errorcode);
		AddInfo(device,str,3);
		return 0;
	}
	return	1;
}

BOOL SW80_DataClear(int device)
{
	int i,ldie;
	for(i=0;i<channel;i++)
		for(ldie=0;ldie<grdie[device];ldie++)
		{
			if(DIEMAP[device][ldie]>7 && DIEMAP[device][ldie]<16)
				SW80Eraseblock(device,i,DIEMAP[device][ldie],flash_type[device].blocknum);
			else
				SW80Eraseblock(device,i,DIEMAP[device][ldie],0);
		}
	return 1;
}


BOOL SW80SmartReset(int device,ULONGLONG vendorlba)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=SLW_CMD_VND_SMART_RESET;
	if(WriteSectors(device,vendorlba,1,buffer)==0)
		return 0;

	return 1;
}

BOOL SW80SmartRestore(int device,ULONGLONG vendorlba)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");

	buffer[14]=SW80_CMD_FEATURE;
	buffer[15]=SLW_CMD_VND_SMART_RESTORE;
	if(WriteSectors(device,vendorlba,1,buffer)==0)
		return 0;

	return 1;
}