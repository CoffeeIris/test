#include "stdafx.h"
#include "SW300API.h"

#include <stdio.h> 
#include <stdlib.h> 
#include <stddef.h>
#include <windows.h>  
#include <objbase.h>
#include <initguid.h>   // Guid definition
#include <devguid.h>    // Device guids
#include <setupapi.h>   // for SetupDiXxx functions.

#include <cfgmgr32.h>   // for SetupDiXxx functions.
#include <devioctl.h>  
#include <ntddstor.h> 
#include <ntddscsi.h>
#include "OPini.h"	
#include <math.h>

#if _INITD 
#include "..\\SW300 INITOOLS\\DlgCmd.h"
extern CDlgCmd *pdlgcmd;
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


typedef struct _STORAGE_ADAPTER_DESCRIPTOR {

    ULONG Version;

    ULONG Size;

    ULONG MaximumTransferLength;

    ULONG MaximumPhysicalPages;

    ULONG AlignmentMask;

    BOOLEAN AdapterUsesPio;

    BOOLEAN AdapterScansDown;

    BOOLEAN CommandQueueing;

    BOOLEAN AcceleratedTransfer;

    UCHAR BusType;

    USHORT BusMajorVersion;

    USHORT BusMinorVersion;

} STORAGE_ADAPTER_DESCRIPTOR, *PSTORAGE_ADAPTER_DESCRIPTOR;

typedef enum _STORAGE_QUERY_TYPE {
    PropertyStandardQuery = 0,          // Retrieves the descriptor
    PropertyExistsQuery,                // Used to test whether the descriptor is supported
    PropertyMaskQuery,                  // Used to retrieve a mask of writeable fields in the descriptor
    PropertyQueryMaxDefined     // use to validate the value
} STORAGE_QUERY_TYPE, *PSTORAGE_QUERY_TYPE;

//
// define some initial property id's
//

typedef enum _STORAGE_PROPERTY_ID {
    StorageDeviceProperty = 0,
    StorageAdapterProperty,
    StorageDeviceIdProperty
} STORAGE_PROPERTY_ID, *PSTORAGE_PROPERTY_ID;

typedef struct _STORAGE_PROPERTY_QUERY {

    //
    // ID of the property being retrieved
    //

    STORAGE_PROPERTY_ID PropertyId;

    //
    // Flags indicating the type of query being performed
    //

    STORAGE_QUERY_TYPE QueryType;

    //
    // Space for additional parameters if necessary
    //

    UCHAR AdditionalParameters[1];

} STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;


HANDLE Devicehandle[DeviceCount];
int SSDID;
DWORD MaxLBA;
INT8U DeviceID;
int GlobalDevice;
char LocalPath[128];
Flash_Para flash_type;
BOOL gucflag;
BYTE flash_id[64][4];		// 	最多64 die, 每个die 有2 plane
int channel = 4;
int plane = 2;
int gldie = 16; //逻辑die,约定一个channel有16个逻辑die,允许为空
int grdie;   //每个channel实际存在的物理die数目，可以根据DIEMAP取
int gcs = 8; //片选数目，一般一个片选有2个die(当前使用的坏块表文件是4个channel,8个cs)
int step =2;	
int page4k ;		// 1: 2K		2: 4K	4: 8K
int blocksize; 	// 1: 128K	2: 256k	4: 512k
int sectorperblock;		//block包含sector的数目，MLC 1024 SLC 512 
int DIEMAP[16];
int badblock[DeviceCount][128][512];		//坏块表,最多128 die,
int xblock[DeviceCount][128];			//坏块个数,最多128 die,
DWORD Totalbadblock[DeviceCount];		//原始坏块统计值
int IniMode;
WORD IDFY32G[256];
int Resratio;
unsigned int FLASHNum=0;

BOOL checkinit;
BOOL checkvalid;
CString SelectBadblockfile;
CString BadBlockFilename[DeviceCount];
CString capacity[DeviceCount];
CString	m_fw[DeviceCount];
CString	m_descriptor[DeviceCount];
PUCHAR GlobelBuffer[DeviceCount];
CString FWFilename;
CString	LoadFilename;
CString	RamdiskFilename;
CString manufacture;
CString RAMDISKSN;
long Vernum=0;

TEST_STRU IOtest[DeviceCount];
int InterfaceType[DeviceCount];
int ECCThreshold=0;
FLASHENDURANCE FlashEndurance[64];

BOOL FindSW300Disk()
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

BOOL CloseSW300SSDHandle()
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
				FILE_FLAG_WRITE_THROUGH,	//	FILE_FLAG_OVERLAPPED|FILE_FLAG_WRITE_THROUGH,                                  // dwFlagsAndAttributes
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


   ULONGLONG vendorlba;
   vendorlba=SW300GetVendorLBA(SSDID);

   if(strstr(interfaceDetailData->DevicePath,"gucssd")==NULL && SW300DistSSD(SSDID)==0 /*&& SW300SSDVenderMode(SSDID,vendorlba,1)==0 */)
   {
       free (interfaceDetailData);
	   CloseHandle(Devicehandle[SSDID]);
	   Devicehandle[SSDID] = INVALID_HANDLE_VALUE;
	   return 1;
   }	
	//如果是SW300 SSD，进入VENDER 模式
	if(SW300DistSSD(SSDID)==2)
	{
		if(SW300SSDVenderMode(SSDID,vendorlba,1)==0)
		{
			AddInfo(SSDID,"Enter VND failed",3);
			return 0;
		}
		else
			AddInfo(SSDID,"Enter VND successfully",1);
	}

	if(strstr(interfaceDetailData->DevicePath,"gucssd"))
		gucflag=1;
	else
		gucflag=0;

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
	pSCIP -> irDriveRegs.bDriveHeadReg = 0xE0| DeviceID;//0xA0 | ((bDriveNum & 1) << 4);
	
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
	CString str="(0MB)";
	
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

	bRet = ReadFile(Devicehandle[device], lpSectBuff, 512 * wSectors, &dwCB, NULL);

	return bRet;
}

BOOL SW300RWRAM(int device,int flag,INT16U sectoroffset,BYTE* sctbuf)
{
	BYTE buffer[512*32];
	BOOL ret;
	memset(buffer,0,32*512);

	buffer[0]='W';
	buffer[1]='S';
	if(flag==0)
		buffer[2]=5;         //read
	else if(flag==1)
		buffer[2]=6;		//write
	buffer[3]=0;  //目标Bank num
	if(flag==0)
		buffer[4]=0x24;		//read
	else if(flag==1)
		buffer[4]=0x25;		//write
	buffer[5]=0x10;
	buffer[6]=sectoroffset;
	buffer[7]=BYTE(sectoroffset >> 8);

	if(flag==0)
	{
		ret=WriteSectors(device,0x5000000,1,buffer);
		if(ret>0)
		{
			ret=ReadSectors(device,0x5000000,1+16,buffer);
			memcpy(sctbuf,buffer+1,16*512);
		}			
	}
	else if(flag==1)
	{
		memcpy(buffer+512,sctbuf,16*512);
		ret=WriteSectors(device,0x5000000,1+16,buffer);
	}

	return ret;
}

BOOL SW300RWDDR(int device,int flag,INT16U sectoroffset,BYTE* sctbuf)
{
	BYTE buffer[512*32];
	BOOL ret;
	memset(buffer,0,32*512);

	buffer[0]='W';
	buffer[1]='S';
	if(flag==0)
		buffer[2]=5;         //read
	else if(flag==1)
		buffer[2]=6;		//write
	buffer[3]=0;  //目标Bank num
	if(flag==0)
		buffer[4]=0x26;		//read
	else if(flag==1)
		buffer[4]=0x27;		//write	
	buffer[5]=0x10;
	buffer[6]=sectoroffset;
	buffer[7]=BYTE(sectoroffset >> 8);
	buffer[8]=BYTE(sectoroffset >> 16);
	if(flag=0)
	{
		ret=WriteSectors(device,0x5000000,1,buffer);
		if(ret>0)
		{
			ret=ReadSectors(device,0x5000000,1+16,buffer);
			memcpy(sctbuf,buffer+1,16*512);
		}			
	}
	else if(flag==1)
	{
		memcpy(buffer+512,sctbuf,16*512);
		ret=WriteSectors(device,0x5000000,1+16,buffer);
	}
	return ret;
}

BOOL SW300ReadFlashid(int device,int channel,int CE,BYTE *ID)
{
	BYTE buffer[512],readbuf[512];
	BOOL ret;
	memset(buffer,0,512);
	buffer[0]='W'; 
	buffer[1]='S';
	buffer[2]=5;
	buffer[3]=0;  //目标Bank num
	buffer[4]=0x32;
	buffer[5]=1;
	buffer[6]=BYTE(channel);
	buffer[7]=BYTE(CE);
	buffer[11]=0;		//without ECC

	ret=WriteSectors(device,0x5000000,1,buffer);

	if(ret==0)
		return ret;
	ret=ReadSectors(device,0x5000000,2,readbuf);
	if(ret==0)
		return ret;

	if(readbuf[0]=='S'&&readbuf[1]=='W')
	{
		ID[0]=readbuf[512+0];
		ID[1]=readbuf[512+2];
		ID[2]=readbuf[512+4];
		ID[3]=readbuf[512+8];
	}
	else
		return 0;

	return 1;
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
			if(SW300ReadFlashUID(device,lchannel,ldie,buf))
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
			AfxMessageBox("当前目录下存在同名的FLASHUID文件，请重新选择文件名保存原始坏块文件!");
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
		str=manufacture+" ";
		if(flash_type.isMLC==0)
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

BOOL SW300ReadFlashUID(int device,int channel,int CE,BYTE *UID)
{
	BYTE buffer[512],readbuf[512];
	BOOL ret;
	int i;
	memset(buffer,0,512);
	buffer[0]='W'; 
	buffer[1]='S';
	buffer[2]=5;
	buffer[3]=0;  //目标Bank num
	buffer[4]=0x33;
	buffer[5]=1;
	buffer[6]=BYTE(channel);
	buffer[7]=BYTE(CE);
	
	ret=WriteSectors(device,0x5000000,1,buffer);

	if(ret==0)
		return ret;
	ret=ReadSectors(device,0x5000000,2,readbuf);
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


BOOL SW300Findbadblk(int device,int CE,int blkcnt,int sblk)
/*
Function:同时查找所有个channel中某个CE的原始坏块
返回值：1代表能执行查找动作，0表示不能支持，这意味不能继续查找
输入：dienum，物理die

*/
{
	BYTE buffer[512],readbuf[512*12];
	BOOL ret;
	int i,j,badblocknum=0,badblockaddr=0;
	CString str;

	memset(buffer,0,512);
	memset(readbuf,0,512*5);
	buffer[0]='W'; 
	buffer[1]='S';
	buffer[2]=5;
	buffer[3]=0;  //目标Bank num
	buffer[4]=0x34;
	buffer[5]=0xA;
	buffer[6]=CE;
	buffer[7]=BYTE(blkcnt);
	buffer[8]=BYTE(sblk);
	buffer[9]=BYTE(sblk>>8);

	ret=WriteSectors(device,0x5000000,1,buffer);
	if(ret==0)
		return ret;

	ret=ReadSectors(device,0x5000000,5,readbuf);
	if(ret==0)
		return ret;

	if(readbuf[0]=='S'&&readbuf[1]=='W')
	{
		for(i=0;i<channel;i++)
		{
			badblocknum=readbuf[15+i];
			str.Format("channel %d die %d 坏块个数 %d\r\n", i,CE, badblocknum);
			AddInfo(device,str,1);	
			for(j=0;j<badblocknum;j++)
			{
				if(j>=255)
					break;
				badblockaddr=readbuf[512*i+512+2*j] | readbuf[512*i+512+2*j+1]<<8;
				SW300UpdateBadblock(device,i,CE,badblockaddr);
			}
		}

		
	}
	else
		return 0;

	return 1;
}

BOOL SW300PageRW(int device,int channel,int CE,DWORD page,BYTE *databuf,int flag)
{
	BYTE buffer[512*32];
	BOOL ret;
	memset(buffer,0,32*512);

	buffer[0]='W'; 
	buffer[1]='S';
	if(flag==0)
		buffer[2]=5;         //read
	else if(flag==1)
		buffer[2]=6;		//write
	buffer[3]=0;  //目标Bank num
	if(flag==0)
		buffer[4]=0x31;		//read 
	else if(flag==1)
		buffer[4]=0x38;		//write
	buffer[5]=channel;
	buffer[6]=BYTE(CE);
	buffer[7]=BYTE(page);
	buffer[8]=BYTE(page>>8);
	buffer[9]=BYTE(page>>16);

	if(flag=0)
	{
		ret=WriteSectors(device,0x5000000,1,buffer);
		if(ret>0)
		{
			ret=ReadSectors(device,0x5000000,1+16,buffer);
			memcpy(databuf,buffer+1,16*512);
		}			
	}
	else if(flag==1)
	{
		memcpy(buffer+512,databuf,16*512);
		ret=WriteSectors(device,0x5000000,1+16,buffer);
	}
	return ret;
}

BOOL SW300IDFYSSD(int device,ULONGLONG vendorlba)
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

int SW300DistSSD(int device)
/*
根据IDFY的值判断是不是SW300 SSD
返回值
1: SW300 RAM DISK
2: SW300 SSD(量产完成后的盘)
0: 非SW300 系列
*/
{	unsigned char SSDflag[32]="SOLIWARE-300",RAMflag[32]="SW300_RAM_DISK",tmp[32];
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
	if(memcmp(model,RAMflag,14)==0)
		return 1;

	for(i=0;i<16;i++)
		tmp[i]=databuf[258+i];

	if(memcmp(tmp,SSDflag,12)==0)
		return 2;

	return 0;
}

BOOL SW300SetBCH(int device,int BCHmode)
/*
Byte 0
0x00: BCH8, 4 K + 128 Byte
0x01: BCH14, 4 K + 218 Byte
0x02: BCH16, 4 K + 218 Byte
*/
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	buffer[0]='W';
	buffer[1]='S';
	buffer[2]=0;
	buffer[3]=BCHmode;  	
	ret=WriteSectors(device,0x5000000,1,buffer);
	return ret;
}

BOOL SW300ReadNORFLASH(int device,int sctcount,int sctoffset,BYTE *SectBuff)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	buffer[0]='W';
	buffer[1]='S';
	buffer[2]=1;
	buffer[3]=sctoffset; 
	ret=WriteSectors(device,0x5000000,1,buffer);
	if(ret)
	{
		ret=ReadSectors(device,0x5000000,sctcount,SectBuff);
	}
	return ret;
}

BOOL SW300WriteNORFLASH(int device,int sctcount,int sctoffset,BYTE *SectBuff)
{

	BOOL ret;

	SectBuff[0]='W';
	SectBuff[1]='S';
	SectBuff[2]=2;
	SectBuff[3]=sctoffset; 
	ret=WriteSectors(device,0x5000000,sctcount,SectBuff);
	return ret;
}

int SLWFlashIDPos()
{
	int i,j;

	for(i=0;i<64;i++)
	{
		for(j=0;j<32;j++)
		{
			if(flash_id[i][0]==FlashEndurance[j].ID[0] && flash_id[i][1]==FlashEndurance[j].ID[1] && flash_id[i][2]==FlashEndurance[j].ID[2] && flash_id[i][3]==FlashEndurance[j].ID[3]) 
				return j;		
		}
	}
}


BOOL IDFY_IDE(int device,BYTE *idfy)
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
		
		if( !ata_pass_through_ioctl_pio(Devicehandle[device], &regs, idfy, 512, false) )
		{
			return FALSE;
		}

		if ( regs.bCommandReg!= 0x50 )//success
		{
			return FALSE;
		}
		
		return TRUE;
	}	
}

BOOL ata_pass_through_ioctl_pio_48b(HANDLE hdevice, IDEREGS * regs,IDEREGS * regspre, unsigned char * data, unsigned long datasize, bool IsWR)
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
			ab.apt.AtaFlags = ATA_FLAGS_DATA_IN | ATA_FLAGS_48BIT_COMMAND;
		}
		else
		{
			ab.apt.AtaFlags = ATA_FLAGS_DATA_OUT | ATA_FLAGS_48BIT_COMMAND;
			memcpy(ab.ucDataBuf, data, datasize);
		}
		ab.apt.AtaFlags |= ATA_FLAGS_DRDY_REQUIRED;
	}
	else
		ab.apt.AtaFlags = ATA_FLAGS_DRDY_REQUIRED;

	CString str;
	ASSERT(sizeof(ab.apt.CurrentTaskFile) == sizeof(IDEREGS));
	ASSERT(sizeof(ab.apt.PreviousTaskFile) == sizeof(IDEREGS));
	IDEREGS * ctfregs = (IDEREGS *)ab.apt.CurrentTaskFile;
	*ctfregs = *regs;
	IDEREGS * ctfregs1 = (IDEREGS *)ab.apt.PreviousTaskFile;
	*ctfregs1 = *regspre;
	
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

		return FALSE;
	}
	
	// Check and copy data
	if (!IsWR && datasize) 
	{
		memcpy(data, ab.ucDataBuf, datasize);
	}
	
	return TRUE;
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

		return FALSE;
	}
	
	// Check and copy data
	if (!IsWR && datasize) 
	{
		memcpy(data, ab.ucDataBuf, datasize);
	}
	
	return TRUE;
}

BOOL SW300FindBadblockAll(int device)
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

	for(i=0;i<grdie;i++)
	{
		// SW300Findbadblock(int device,int CE,int blkcnt,int sblk)
		//	return 0;
		;
	}

	int errorflag=0;	
	for(i=0;i<2;i++)
	{
		for(j=0;j<xblock[device][i];j++)
		{
			if(badblock[device][i][j]==0)
			{
				errorflag=1;				
				str.Format("Channel %d die %d  block 0 块为坏块\r\n",(i/gldie),DIEMAP[i%gldie]);
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
			if(a>(flash_type.blocknum*Resratio/(2*128*flash_type.plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d plane 0 坏块>50\r\n",(i/gldie),DIEMAP[i%gldie]);
				AddInfo(device,str,3);
					
			}
			if(b>(flash_type.blocknum*Resratio/(2*128*flash_type.plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d plane 1 坏块>50\r\n",(i/gldie),DIEMAP[i%gldie]);
				AddInfo(device,str,3);
					
			}
			if(c>(flash_type.blocknum*Resratio/(2*128*flash_type.plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d  plane 0 坏块>50\r\n",(i/gldie),DIEMAP[i%gldie]);
				AddInfo(device,str,3);
					
			}
			if(d>(flash_type.blocknum*Resratio/(2*128*flash_type.plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d  plane 1 坏块>50\r\n",(i/gldie),DIEMAP[i%gldie]);
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
			if(a>(flash_type.blocknum*Resratio/(2*128*flash_type.plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d plane 0 坏块>50\r\n",(i/gldie),DIEMAP[i%gldie]);
				AddInfo(device,str,3);
					
			}
			if(b>(flash_type.blocknum*Resratio/(2*128*flash_type.plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d plane 1 坏块>50\r\n",(i/gldie),DIEMAP[i%gldie]);
				AddInfo(device,str,3);
					
			}
			if(c>(flash_type.blocknum*Resratio/(2*128*flash_type.plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d  plane 0 坏块>50\r\n",(i/gldie),DIEMAP[i%gldie]);
				AddInfo(device,str,3);
					
			}
			if(d>(flash_type.blocknum*Resratio/(2*128*flash_type.plane_per_die)))
			{
				errorflag=1;
				str.Format("Channel %d die %d  plane 1 坏块>50\r\n",(i/gldie),DIEMAP[i%gldie]);
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


BOOL SW300GetDeviceInfo(int device,int Initnum)
{
	int i=0;
	int j=0;

	CString str;
	if(Initnum==0)
	{
		if(IniMode==1)
		{
			if(SW300FindBadblockAll(device)==0)
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
#if _MASSINIT
		pSW300MPToolDlg->m_GridCtrl.SetItemText(device+1,2," 100%");
		pSW300MPToolDlg->m_GridCtrl.Refresh();
#endif
					
		}
		else if(IniMode==2)	
		{
			if(SW300ReadBadblock(device)==0)
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
			SW300EraseAll(device);
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
			SW300EraseAll(device);/*擦除所有块*/
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
说明：检查当前BLOCK是否是坏块，如果是，则不用擦除。
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
		if(VerifyBadblock(device,i,die,block))
			blockaddr[4*i]=0xff;
		else
			blockaddr[4*i]=i;

		blockaddr[4*i+1]=die;
		blockaddr[4*i+2]=block;
		blockaddr[4*i+3]=block>>8;
	}
}

void SW300EraseAll(int device)
{
	int idie;
	DWORD dblock = 4096,num;
	DWORDLONG LBAADD = 0;
	CString str;
	int loc=0;
	BYTE blockaddr[16],status[8];

	for (idie = 0; idie <gldie ;idie ++)
	{
#if _INITD 
		pdlgcmd->m_progress.SetPos(idie*80/gldie);

#endif
		if(DIEMAP[idie]==65535)
			continue;
		for(dblock = 0;dblock <flash_type.blocknum ; dblock++)
		{

			if( (DIEMAP[idie]>7&&DIEMAP[idie]<16) || (DIEMAP[idie]>23&&DIEMAP[idie]<32) )
			{
				num=dblock+flash_type.blocknum;
				if(flash_type.blocknum%1024!=0)
					num=(flash_type.blocknum/1024)*2048+dblock;
			}
			else
				num=dblock;
			
			memset(blockaddr,0,16);
			memset(status,0,8);
			Setblockaddr(device,DIEMAP[idie],num,blockaddr);
		}
	}
	
	AddInfo(device,"Erase all block and find new bad block successfully !\r\n",1);
}

BOOL SW300Eraseblk(int device,int channelmask,int CE,int blk)
{
	BYTE buffer[512];
	BOOL ret;
	int i,j,badblocknum=0,badblockaddr=0;
	CString str;

	memset(buffer,0,512);
	buffer[0]='W';
	buffer[1]='S';
	buffer[2]=6;		//write
	buffer[3]=0;  //目标Bank num
	buffer[4]=0x31;
	buffer[5]=0;
	buffer[6]=channelmask;
	buffer[7]=channelmask>>8;
	buffer[8]=CE;
	buffer[9]=blk;
	buffer[10]=BYTE(blk>>8);
	ret=WriteSectors(device,0x5000002,1,buffer);
	return ret;


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


#if _MASSINIT 
	FILE *fp;
	char file[128];

    GetModuleFileName(NULL,file,128); 
    //Scan a string for the last occurrence of a character.
    (strrchr(file,'\\'))[1] = 0; 
	strcat(file,"LOG\\");

	if(BadBlockFilename[device].Right(4) == ".bck")
		BadBlockFilename[device].TrimRight(".bck");
	
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

#if	_UPDATE

	FILE *fp;
	char file[128];

    GetModuleFileName(NULL,file,128); 
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
	char filename[128];
	GetModuleFileName(NULL,filename,128); 
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
	if(a!=grdie)
	{
		AddInfo(device,"\r\n\r\n****die设置不正确!!!\r\n\r\n",3);
		fclose(fp);
		return 0;
	}

	fscanf(fp,"%s ",output);
	fscanf(fp,"%s ",output);
	fscanf(fp,"%d",&a);
	

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

	str=manufacture+" ";
	capacity[device].Format("%dGB",flash_type.cap_in_gb);
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
	
	str.Format("CHANNEL = %d\r\n",channel);//channel number
//	str="CHANNEL = 4\r\n";//channel number
	fprintf(fp,"%s",str);

	sprintf(output,"DIE = %d\r\n",grdie);//die number
	str=output;
	fprintf(fp,"%s",str);

	sprintf(output,"PLANE = %d\r\n",2);//plane number
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

BOOL SW300ReadBadblock(int device)
{
	unsigned int i,j,block,ret,k,addr,badblockaddr,temp;
	unsigned char buf[512*18];
	DWORD pagenum;

	for(i=0;i<5120;i++)
		buf[i]=0xFF;

	for(i=0;i<channel;i++)
		for(j=0;j<grdie;j++)
		{
			if(DIEMAP[j]==0xFFFF)
				continue;
			if( (DIEMAP[j]>7&&DIEMAP[j]<16) || (DIEMAP[j]>23&&DIEMAP[j]<32) )
				block=4096;
			else
				block=0;
	
			pagenum=flash_type.pages_per_block*block;
/*			ret = SW300PageRead(device,i,DIEMAP[j],pagenum,0,buf);
			if (ret == 0)
			{
				AddInfo(device,"****Read BadblockTable info failed!!!",3);	
				return 0;
			}*/
			addr=4096;
			for(k=0;k<128;k++)
			{
				temp=buf[addr+8*k+1]&0x80;
				if(temp==0x80)
				{
					badblockaddr=(buf[addr+8*k]|(buf[addr+8*k+1]<<8))&0x3FFF;
					SW300UpdateBadblock(device,i,DIEMAP[j],badblockaddr);
				}
			}
			addr=8192;
			for(k=0;k<128;k++)
			{
				temp=buf[addr+8*k+1]&0x80;
				if(temp==0x80)
				{
					badblockaddr=(buf[addr+8*k]|(buf[addr+8*k+1]<<8))&0x3FFF;
					SW300UpdateBadblock(device,i,DIEMAP[j],badblockaddr);
				}
			}
		}
	return 1;
}

BOOL SW300UpdateBadblock(int device,int channelnum,int dienume,int blocknum)
{
	int loc,j;
	BOOL flag=0;
	loc=channelnum*gldie+dienume;
	int twoplaneblock;

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

BOOL SW300VerifyBadblock(int device,int channelnum,int dienume,int blocknum)
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
BOOL SW300WriteGlobalBlock(int device,unsigned char * data,int channelnum,int dienume,int blocknum)
{
	return 1;
}

/*读全局信息表*/
BOOL SW300ReadGlobalBlock(int device,unsigned char * data,int channelnum,int dienume,int blocknum)
{
	return 1;
}

int SSDInitialize(int device)
{
	int ret=0;
	int m;
	int gcblock=1;
	CString str;
	DWORD time = GetTickCount();
#if _INITD 
	pdlgcmd->m_progress.SetPos(1);
#endif

	ret = SWInitial(device,0);
	if(ret>0)
	{
#if _INITD 
		pdlgcmd->m_progress.SetPos(90);
#endif
		AddInfo(device,"******Initionalize completed!!!\r\n",1);
	}

	time = GetTickCount() - time;
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

	ResetGlobal(device,Initnum);

#if _INITD 			
	pdlgcmd->m_progress.SetText("");	
	pdlgcmd->m_progress.SetPos(0);
#endif

	CString file; 
	file=SLWCONFINI;
	if(Initnum==0)
		ReadConfigureIni(file);
	
	if(ReadFlashID(device)==0)
	{
		AddInfo(device,"****ReadID fail!\r\n",3);
		return -1;
	}

	AddInfo(device,"Build bad block table successfully!\r\n",1);

#if _INITD 
	pdlgcmd->m_progress.SetPos(80);
#endif


#if _INITD 
	pdlgcmd->m_progress.SetPos(85);
#endif
	return 1;
}

BOOL SWBuildGlobleInfoBlock(int device)
{
	return 1;
}

BOOL ReadConfigureIni(CString FilePath)
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
	
	ini.ReadString("Device Setting","Model Number",str,filename);
	for(i=0;i<DeviceCount;i++)
		m_descriptor[i] = str;
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
	
	ini.ReadString("FW Setting","ECC Alarm Threshold",str,filename);
	ECCThreshold=atol(str);

#if _MASSINIT
	ini.ReadString("Update Setting","FW FILE",str,filename);
	FWFilename=str;
	ini.ReadString("Update Setting","LOAD FILE",str,filename);
	LoadFilename=str;
	ini.ReadString("Option","SSD INITIONALIZE",str,filename);
	checkinit=atoi(str);
	ini.ReadString("Option","Validation Test",str,filename);
	checkvalid=atoi(str);
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
			{
				badblock[device][i][j]=0;	
			}

		for(i=0;i<gldie*4;i++)
			xblock[device][i]=0;

		Totalbadblock[device]=0;
	}		
}

void SW300WirteBadblockflag(int device,int channel,int CE,int blk)
{
	BYTE buffer[512];
	BOOL ret;
	int i,j,badblocknum=0,badblockaddr=0;
	CString str;

	memset(buffer,0,512);
	buffer[0]='W';
	buffer[1]='S';
	buffer[2]=6;		//write
	buffer[3]=0;  //目标Bank num
	buffer[4]=0x35;
	buffer[5]=0;
	buffer[6]=channel;
	buffer[7]=CE;
	buffer[8]=blk;
	buffer[9]=BYTE(blk>>8);
	ret=WriteSectors(device,0x5000000,1,buffer);
}

BOOL SW300ReadGlobalinfoblockTxt(CString File)
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
	DWORD tmp;
	int blockcnt;

	tmp=grdie*channel*flash_type.blocknum*(128-GlobelBuffer[device][158+1024])/(GlobelBuffer[device][162+1024]*128);
	if(flash_type.blocknum%1024!=0)
	{
		blockcnt=(flash_type.blocknum/1024)*1024;  //取整，保证计算block数是1024的整数倍
		tmp=grdie*channel*blockcnt*(128-GlobelBuffer[device][158+1024])/(GlobelBuffer[device][162+1024]*128);
	}
	lcapa=tmp*(flash_type.pages_per_block*GlobelBuffer[device][162+1024]-GlobelBuffer[device][160+1024])*flash_type.sectors_per_page;

	if(lcapa<0xF00000)
	{
		IDFY32G[1]=lcapa/(16*63);
		IDFY32G[54]=IDFY32G[1];
		tmp=IDFY32G[1]*16*63;
		IDFY32G[57]=tmp;
		IDFY32G[58]=(tmp)>>16;

		IDFY32G[60]=lcapa;
		IDFY32G[61]=lcapa>>16;
		IDFY32G[83]=0x7000;			//小于8G关闭48 BIT LBA
		IDFY32G[86]=0x3000;			//小于8G关闭48 BIT LBA	
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

		IDFY32G[83]=0x7400;		//大于8G打开48 BIT LBA
		IDFY32G[86]=0x3400;     //大于8G打开48 BIT LBA

	}

	if((channel*grdie*plane*blocksize*flash_type.blocknum/(4*4096))==8)//new 8GB
	{	

		IDFY32G[31]=0x3030;
		IDFY32G[32]=0x3847;
		IDFY32G[60]=lcapa;
		IDFY32G[61]=lcapa>>16;


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
		IDFY32G[100]=lcapa;
		IDFY32G[101]=lcapa>>16;
		for(i=0;i<256;i++)
		{
			GlobelBuffer[device][addptr+2*i]=(unsigned char)IDFY32G[i];
			GlobelBuffer[device][addptr+2*i+1]=(unsigned char)(IDFY32G[i]>>8);
		}
	}
	
	MaxLBA = (IDFY32G[101]<<16|IDFY32G[100])-1;
}

long SLWGlobalinfo(int device,PUCHAR buf,int Initnum,long offset,BOOL CodeFlag)
{
	PUCHAR DATABuffer;	
	long datasize=64*2048*(blocksize);//new
	int i;
	FILE *fp;
	CString strDldFile;	
	int readnum;
	long line;

	if(blocksize==0)
		datasize=64*2048*4;
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
	char filename[128];	
	memset(filename,0,128);
	GetModuleFileName(NULL,filename,128); 
	(strrchr(filename,'\\'))[1] = 0;

	strDldFile = filename+FWFilename ;
#endif


#if _MASSINIT 
	char filename[128];
	GetModuleFileName(NULL,filename,128); 
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

BOOL SW300SSDVenderMode(int device,ULONGLONG vendorlba,BOOL venderflag)
{
	IDEREGS regs;
	if(venderflag)
		regs.bFeaturesReg		= SW300_CMD_VND_SUB_ENDBG;
	else
		regs.bFeaturesReg		= SW300_CMD_VND_SUB_OUTBG;
			
	regs.bSectorCountReg	= 0;
	regs.bSectorNumberReg	= 0;
	regs.bCylLowReg			= 0;
	regs.bCylHighReg		= 0;	
	regs.bDriveHeadReg		= 0;	
	regs.bCommandReg		= SW300_CMD_VND;

	if( !ata_pass_through_ioctl_pio(Devicehandle[device], &regs, NULL, NULL, true) )
	{
	//	AddInfo(device,"ata_pass_through_ioctl_pio Failed !",2);
		return 0;
	}
	return 1;
}

ULONGLONG SW300GetVendorLBA(int device)
{
	ULONGLONG vendorlba;
	BYTE databuf[512];
	if(IDFY_IDE(device,databuf)==0)
		return 0;
	else
	{
		vendorlba=databuf[203]<<24 | databuf[202]<<16 | databuf[201]<< 8 | databuf[200];
		return vendorlba;
	}
}

BOOL SW300UpdateFWInRamdisk(int device)
{
	PUCHAR DATABuffer,ReadBuffer;
	long ret =0;
	int i,datasize=1024*32,readsize=1024*32;
	unsigned char pagebuf[18*512];
	CString str;
	CFile fl;
	CString strDlgFile;

#if _INITD 

	CFileDialog dlgLoad(
		TRUE, 0, 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"(*.bin)|*.bin|AllFiles (*.*)|*.*||");

	dlgLoad.m_ofn.lpstrTitle = "Select FW code";
	if(dlgLoad.DoModal() != IDOK)
	{
		return 0;		
	}
	
	strDlgFile = dlgLoad.GetPathName();	
#endif

	DATABuffer=(PUCHAR) malloc(datasize);
	ReadBuffer=(PUCHAR)malloc(readsize);
	memset(pagebuf,0,18*512);
	for(i=0;i<datasize;i++)
	{
		DATABuffer[i]=0;
	}
	for(i=0;i<readsize;i++)
	{
		ReadBuffer[i]=0;
	}

	if( !fl.Open(strDlgFile, CFile::modeRead, NULL) )
	{
		AfxMessageBox("Open File Failed !");
		free(DATABuffer);
		free(ReadBuffer);
		return 0;		
	}

	if(fl.GetLength() > datasize )
	{
		AfxMessageBox("File size should below 32KB !");
		free(DATABuffer);
		free(ReadBuffer);
		return 0;	
	}

	if( !fl.Read(DATABuffer, fl.GetLength()) )
	{
		AfxMessageBox("Read fw file Failed !");
		free(DATABuffer);
		free(ReadBuffer);
		fl.Close();
		return 0;	
	}
	fl.Close();

	for(i=0;i<8;i++)
	{
		memcpy(pagebuf+512,DATABuffer+i*4096,1024*4);
		if(SW300WriteNORFLASH(device,9,i+1,pagebuf)==0)
		{
			AddInfo(device,"Write fw failed !",3);
			free(DATABuffer);
			free(ReadBuffer);
			return 0;
		}		
	}

	AddInfo(device,"Write fw successfully!",1);

	for(i=0;i<8;i++)
	{
		if(SW300ReadNORFLASH(device,9,i+1,pagebuf)==0)
		{
			AddInfo(device,"Page read failed !",3);
			free(ReadBuffer);
			free(DATABuffer);
			return 0;
		}
		memcpy(ReadBuffer+i*4096,pagebuf+512,1024*4);
	}

	int Cmpflag=0;
	for(i=0;i<datasize;i++)
	{
		if(ReadBuffer[i]!=DATABuffer[i])
		{
			str.Format("offset: %x  w: %x   r:%x",i,DATABuffer[i],ReadBuffer[i]);
			AddInfo(device,str,3);
			Cmpflag=1;
		}
	}
	if(Cmpflag)
	{
		free(ReadBuffer);
		free(DATABuffer);
		AddInfo(device,"Compare loader fw failed!",3);
		return 0;
	}
	
	free(DATABuffer);
	free(ReadBuffer);
	AddInfo(device,"****Compare Fw info successfully!!!",1);	
	return 1;
}

BOOL SW300SSDUpdateFW(int device,ULONGLONG vendorlba)
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
	if(SW300SSDReadFW(device,vendorlba,buf)==0)
	{
		AddInfo(device,"****Read global info failed!!!",3);	
		free(buf);
		free(ReadBuffer);
		return 0;		
	}

	addptr = 1024;
	

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

	if(SW300SSDWriteFW(device,vendorlba,buf)==0)
	{
		AddInfo(device,"Write FW info block fail!\r\n",3);
		free(buf);
		free(ReadBuffer);
		return 0;
	}	
	else
		AddInfo(device,"Write FW info block successfully!\r\n",1);

	BOOL Cmpflag=0;
	if( SW300SSDReadFW(device,vendorlba,ReadBuffer))
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

BOOL SW300SSDReadFW(int device,ULONGLONG vendorlba,BYTE *buf)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);

	buffer[15]=0xA1;
	buffer[16]=64*2;
	if(WriteSectors(device,vendorlba,1,buffer)==0)
		return 0;
	Sleep(50);
	ret=ReadSectors(device,vendorlba,64*2,buf);
	return ret;
}

BOOL SW300SSDWriteFW(int device,ULONGLONG vendorlba,BYTE *buf)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");


	buffer[15]=0xA2;
	buffer[16]=64*2;
	if(WriteSectors(device,vendorlba,1,buffer)==0)
		return 0;
	Sleep(50);
	ret=WriteSectors(device,vendorlba,64*2,buf);
	return ret;
}


BOOL SW300UpdateLoad(int device,CString strDlgFile,int flag)
/*
在RAMDISK下写入loaderFW
flag：0  loaderFW是第一次写入，没有写入FW info，直接写入
flag: 1  loaderFW已经存在，升级FW info
*/
{
	PUCHAR DATABuffer,readbuf;
	int datasize = 32*512,i;
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
		AfxMessageBox("Open File Failed !");
		free(DATABuffer);
		return 0;		
	}

	if(fl.GetLength() > datasize )
	{
		AfxMessageBox("File size should below 16KB !");
		free(DATABuffer);
		return 0;	
	}

	if( !fl.Read(DATABuffer, fl.GetLength()) )
	{
		AfxMessageBox("Read loadfw file Failed !");
		free(DATABuffer);
		fl.Close();
		return 0;	
	}
	fl.Close();

	memcpy(pagebuf+512,DATABuffer,1024*4);
	if(SW300WriteNORFLASH(device,9,0,pagebuf)==0)
	{
		AddInfo(device,"Page write failed !",3);
		free(DATABuffer);
		return 0;
	}
		

	AddInfo(device,"Write fw loader successfully!",1);
	readbuf=(PUCHAR)malloc(datasize);
	memset(readbuf, 0, datasize);

	if(SW300ReadNORFLASH(device,9,0,pagebuf)==0)
	{
		AddInfo(device,"Page read failed !",3);
		free(readbuf);
		free(DATABuffer);
		return 0;
	}
	memcpy(readbuf,pagebuf+512,1024*4);

	BOOL Cmpflag=0;
	for(i=0;i<8*512;i++)
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

	return 1;
}


BOOL SW300SSDReadLoad(int device,ULONGLONG vendorlba,BYTE *buf)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");


	buffer[15]=0xA6;
	buffer[16]=32*2;
	if(WriteSectors(device,vendorlba,1,buffer)==0)
		return 0;
	Sleep(50);
	ret=ReadSectors(device,vendorlba,32*2,buf);
	return ret;
}

BOOL SW300SSDWriteLoad(int device,ULONGLONG vendorlba,DWORD OOB,BYTE *buf)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");

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

BOOL SW300SSDUpdateLoad(int device,ULONGLONG vendorlba,CString strDlgFile)
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
		AfxMessageBox("Read loadfw File Failed !");
		free(DATABuffer);
		fl.Close();
		return 0;	
	}
	fl.Close();

	readbuf=(PUCHAR)malloc(datasize);
	memset(readbuf, 0, datasize);
	if(SW300SSDReadLoad(device,vendorlba,readbuf)==0)
	{
		AddInfo(device,"Read loader fw failed !",3);
		free(readbuf);
		free(DATABuffer);	
		return 0;
	}
	

	for(i=0;i<8*1024;i++)
	{
		tmpnum=DATABuffer[4*i]+(DATABuffer[4*i+1]<<8)+(DATABuffer[4*i+2]<<16)+(DATABuffer[4*i+3]<<24);
		oobnum=oobnum+tmpnum;
	}


	str.Format("The checknum is %x",oobnum);
	AddInfo(device,str,1);

	if(SW300SSDWriteLoad(device,vendorlba,oobnum,DATABuffer)==0)
	{
		AddInfo(device,"Write load fw failed !",3);
		free(readbuf);
		free(DATABuffer);
		return 0;
	}
	AddInfo(device,"Write loader fw successfully!",1);

	memset(readbuf, 0, datasize);
	if(SW300SSDReadLoad(device,vendorlba,readbuf)==0)
	{
		AddInfo(device,"Read loader fw failed !",3);
		free(readbuf);
		free(DATABuffer);
		return 0;
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

	AddInfo(device,"Compare loader fw successfully!",1);
	free(readbuf);
	free(DATABuffer);
	return 1;
}

BOOL SW300SSDReadDiskinfo(int device,ULONGLONG vendorlba,BYTE *buf)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"WS08D_SI_K");

	buffer[15]=0xA4;
	if(WriteSectors(device,vendorlba,1,buffer)==0)
		return 0;
	Sleep(50);
	ret=ReadSectors(device,vendorlba,1,buf);
	return ret;
}



BOOL EnterVND(int device) 
{
	ULONGLONG vendorlba;

	AddInfo(device,"Enter VND mode",1);	
	if(gucflag==1)
	{				
		AddInfo(device,"Enter VND mode",1);	
		if(SW300VenderMode(device,1)==0)
		{
			AddInfo(device,"Enter VND failed",3);
			return 0;
		}
		else
			AddInfo(device,"Enter VND successfully",1);
	}
	else
	{
		vendorlba=SW300GetVendorLBA(device);
		if(SW300SSDVenderMode(device,vendorlba,1)==0)
		{
			AddInfo(device,"Enter VND failed",3);
			return 0;
		}
		else
			AddInfo(device,"Enter VND successfully",1);
	}
	return 1;
}

BOOL DownSRAM(int device,int fwflag) 
{
	CString strDlgFile;
	PUCHAR DATABuffer;
	int datasize = 0;	
	long tmp;
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

#if _MASSINIT 
	char filename[128];
	GetModuleFileName(NULL,filename,128); 
	(strrchr(filename,'\\'))[1] = 0;
	strDlgFile=filename;
	strDlgFile+=LoadFilename;
#endif

	CFile fl;
	if( !fl.Open(strDlgFile, CFile::modeRead, NULL) )
	{
		AfxMessageBox("Open File Failed !");
		return 0;		
	}

	if(fwflag)
	{
		datasize=fl.GetLength();
		if(  datasize%(8*1024) >0)
			datasize=(fl.GetLength()/(8*1024)+1)*8*1024;//取整，必须是8K的整数倍
	}	
	else 
		datasize=64*1024;
	
	DATABuffer=(PUCHAR)malloc(datasize);
	memset(DATABuffer, 0, datasize);

	if( !fl.Read(DATABuffer, fl.GetLength()) )
	{
		AfxMessageBox("Read loadfw file Failed !");
		free(DATABuffer);
		fl.Close();
		return 0;	
	}
	fl.Close();

	if(fwflag)
	{
		if(SW300RunFW(device,DATABuffer,fl.GetLength())==0)
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
		if(SW300RunRam80(device,DATABuffer)==0)
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
	ULONGLONG vendorlba;

	vendorlba=SW300GetVendorLBA(device);
	if(SW300SSDVenderMode(device,vendorlba,0)==0)
	{
		AddInfo(device,"Out VND failed",3);
		return 0;
	}
	else
		AddInfo(device,"Out VND successfully",1);

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
			if(SW300ReadFlashid(device,lchannel,ldie,buf))
			{
				str.Format("Read ID: Channel:%d cs:%d = %02X %02X %02X %02X",lchannel, locgcs%8,  
								buf[0], buf[1], buf[2], buf[3]);
				AddInfo(device,str,1);
				memcpy(&(flash_id[lchannel * gldie + ldie ][0]), buf, 4);
				memcpy(&(flash_id[lchannel * gldie + ldie+8][0]), buf, 4);
				if(buf[0] == 0xEC || buf[0] == 0x89 || buf[0] == 0x2C || buf[0]==0x98)
				{
					locgrdie=locgrdie+2;
				}

				if(buf[0] == 0xEC || buf[0] == 0x89 || buf[0] == 0x2C || buf[0]==0x98)
				{
					diepercs=flash_id[0][2]&0x01;
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
	grdie=locgrdie;

	if(ReadflashPara(device)==0)
	{
		AddInfo(device,"FLASH ID is error",3);
		return 0;
	}

	str.Format("FLASH ID: %02X %02X %02X %02X", flash_type.ID[0], flash_type.ID[1], flash_type.ID[2], flash_type.ID[3]);
	AddInfo(device,str,1);

	str.Format("Page size: %d K \r\nBlock size: %d K", flash_type.size_page, flash_type.size_block);
	AddInfo(device,str,1);

	str.Format("Pages Per Block: %d ", flash_type.pages_per_block);
	AddInfo(device,str,1);

	str.Format("Sectors Per Page: %d ", flash_type.sectors_per_page);
	AddInfo(device,str,1);

	str.Format("Total Block: %d ", flash_type.total_blocks);
	AddInfo(device,str,1);

	str.Format("TYPE MLC: %d ", flash_type.isMLC );
	AddInfo(device,str,1);

	str.Format("Chip Capactiy: %dG ", flash_type.cap_in_gb );
	AddInfo(device,str,1);

	str.Format("Die per channel: %d ", locgrdie );
	AddInfo(device,str,1);

	/*	判断是Intel 还是三星的flash*/
	if(flash_type.ID[0]==0x89)
		manufacture="INTEL";
	else if(flash_type.ID[0]==0xEC)
		manufacture="SAMSUNG";
	else if(flash_type.ID[0]==0x2C)
		manufacture="Micron";
	else if(flash_type.ID[0]==0x98)
		manufacture="TOSHIBA";

	if(diepercs==0)
		gcs=grdie;
	else
		gcs=grdie/2;
	str.Format("CS per channel: %d ", gcs );
	AddInfo(device,str,1);	
	
	return true;
}

BOOL ReadflashPara(int device)
{
	int diepercs=1;
	int i=16;
	int ldie;
	int dienum=0;
	int idflag[4];

	i=SLWFlashIDPos();
	if(FlashEndurance[i].pagesize==0)
		return 0;

	flash_type.plane_per_die=FlashEndurance[i].Planeperdie;
	if(flash_id[0][0] == 0xEC)
	{
		memcpy(flash_type.ID, &(flash_id[0][0]),4);

		flash_type.size_page = FlashEndurance[i].pagesize;
		flash_type.size_block = FlashEndurance[i].blocksize;
		flash_type.blocknum = FlashEndurance[i].blocknum;
		sectorperblock = flash_type.size_block*2;

		flash_type.isMLC =  FlashEndurance[i].Flashtype;

		//判断一个片选有几个DIE
		diepercs=flash_type.ID[2]&0x01;

		flash_type.cap_in_gb = FlashEndurance[i].blocknum*FlashEndurance[i].blocksize*SW300GetDIEnumPerChip()/(1024*1024);
		if(diepercs==0)
			flash_type.cap_in_gb=flash_type.cap_in_gb/2;
		
		flash_type.pages_per_block = flash_type.size_block / flash_type.size_page;
		flash_type.sectors_per_page = (flash_type.size_page * 1024) / 512;
		flash_type.sectors_per_block = flash_type.sectors_per_page * flash_type.pages_per_block;
		flash_type.total_blocks = (flash_type.cap_in_gb * 1024 * 1024) / flash_type.size_block;		 
		blocksize=flash_type.size_block/128;
		page4k=flash_type.size_page/2;

		//判断4个channel的id是否相同,并得到DIEMAP
		for(ldie=0;ldie<gldie;ldie++)
		{

			//当每个片选只有一个die的时候，按照现行结构，一个channel只有4个die
			if(diepercs==0&&ldie>=4)
				continue;

			idflag[0]=memcmp(flash_id[ldie],flash_id[gldie+ldie],4);
			idflag[1]=memcmp(flash_id[ldie],flash_id[2*gldie+ldie],4);
			idflag[2]=memcmp(flash_id[ldie],flash_id[3*gldie+ldie],4);
			for(i=0;i<channel-1;i++)
			{
				if(idflag[i])
				{
					AddInfo(device,"4个channel的FLASH ID不对称，不能初始化！",3);
					return 0;
				}
			}
			if(flash_id[ldie][0]==0xEC)
			{
				DIEMAP[dienum]=ldie;
				dienum++;
			}			
		}
		
		return 1;
	}

	else if(flash_id[0][0] == 0x89 || flash_id[0][0] == 0x2C || flash_id[0][0] == 0x98)
	{
		memcpy(flash_type.ID, &(flash_id[0][0]),4);		
		diepercs=flash_type.ID[2]&0x01;

		flash_type.cap_in_gb = FlashEndurance[i].blocknum*FlashEndurance[i].blocksize*SW300GetDIEnumPerChip()/(1024*1024);
		if(diepercs==0)
			flash_type.cap_in_gb=flash_type.cap_in_gb/2;

		flash_type.isMLC =  FlashEndurance[i].Flashtype;
		sectorperblock = flash_type.size_block*2;
		flash_type.size_page = FlashEndurance[i].pagesize;
		flash_type.size_block = FlashEndurance[i].blocksize;
		flash_type.blocknum = FlashEndurance[i].blocknum;

		flash_type.pages_per_block = flash_type.size_block / flash_type.size_page;
		flash_type.sectors_per_page = (flash_type.size_page * 1024) / 512;
		flash_type.sectors_per_block = flash_type.sectors_per_page * flash_type.pages_per_block;
		flash_type.total_blocks = (flash_type.cap_in_gb * 1024 * 1024) / flash_type.size_block;		 
		blocksize=flash_type.size_block/128;
		page4k=flash_type.size_page/2;
		//判断4个channel的id是否相同,并得到DIEMAP
		for(ldie=0;ldie<gldie;ldie++)
		{

			//当每个片选只有一个die的时候，按照现行结构，一个channel只有4个die
			if(diepercs==0&&ldie>=4)
				continue;
			idflag[0]=memcmp(flash_id[ldie],flash_id[gldie+ldie],4);
			idflag[1]=memcmp(flash_id[ldie],flash_id[2*gldie+ldie],4);
			idflag[2]=memcmp(flash_id[ldie],flash_id[3*gldie+ldie],4);
			for(i=0;i<channel-1;i++)
			{
				if(idflag[i])
				{
					AddInfo(device,"4个channel的FLASH ID不对称，不能初始化！",3);
					return 0;
				}
			}

			if(flash_id[ldie][0]==0x89 || flash_id[ldie][0] == 0x2C || flash_id[ldie][0] == 0x98)
			{
				DIEMAP[dienum]=ldie;
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
	ULONGLONG vendorlba;
	BOOL ret=0;
	CString strDlgFile;

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
	char filename[128];
	GetModuleFileName(NULL,filename,128); 
	(strrchr(filename,'\\'))[1] = 0;
	strDlgFile=filename;
	strDlgFile+=LoadFilename;
#endif

#if _MASSINIT 
	char filename[128];
	GetModuleFileName(NULL,filename,128); 
	(strrchr(filename,'\\'))[1] = 0;
	strDlgFile=filename;
	strDlgFile+=LoadFilename;
#endif

	vendorlba=SW300GetVendorLBA(device);
	if(SW300DistSSD(device)==1)
		ret=SW300UpdateLoad(device,strDlgFile,flag);
	else if(SW300DistSSD(device)==2)
		ret=SW300SSDUpdateLoad(device,vendorlba,strDlgFile);

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

int SW300GetDIEnumPerChip()
{
	int i;
	int dieperchip=0;
	int diepos[8];//记录每个chip中的8个die是否为空
	int tmp;

	memset(diepos,0,8*sizeof(int));
	for(i=0;i<64;i++)
	{
		if(flash_id[i][0]==0xEC ||flash_id[i][0]==0x89 || flash_id[i][0] == 0x2C || flash_id[i][0] == 0x98)
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


BOOL SW300SetSNtoRamdisk(int device,unsigned char* DATABuffer,unsigned char* sid)
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

BOOL SW300ATARWNORFLASH(int device,INT16U sctoffset,BOOL venderflag,BYTE * buf)
{
	IDEREGS regs;
	if(venderflag)
		regs.bFeaturesReg		= SW300_CMD_VND_SUB_ENDBG;
	else
		regs.bFeaturesReg		= SW300_CMD_VND_SUB_OUTBG;
			
	regs.bSectorCountReg	= 8;
	regs.bSectorNumberReg	= sctoffset;
	regs.bCylLowReg			= sctoffset>>8;
	regs.bCylHighReg		= 0;	
	regs.bDriveHeadReg		= 0;	
	regs.bCommandReg		= SW300_CMD_RWNFLASH;

	if( !ata_pass_through_ioctl_pio(Devicehandle[device], &regs, buf, 4096, venderflag) )
	{
		AddInfo(device,"ata_pass_through_ioctl_pio Failed !",2);
		return 0;
	}
	return 1;
}

BOOL SW300ATARWSPI(int device,SW300_Para VAR,BOOL IsWR,BYTE * buf,int datasize)
{
	IDEREGS regs,regspre;

	regs.bFeaturesReg		= 0x00;
	regs.bSectorCountReg	= VAR.par1;
	regs.bSectorNumberReg	= VAR.par0 ;
	regs.bCylLowReg			= VAR.par2 ;
	regs.bCylHighReg		= VAR.par3 ;	
	regs.bDriveHeadReg		= 0x40;									
	regs.bCommandReg		= SW300_CMD_RWSPI;									
	regs.bReserved			= 0x00;

	regspre.bFeaturesReg		= 0x00;
	regspre.bSectorCountReg		= 0x00;
	regspre.bSectorNumberReg	= VAR.par4 ;
	regspre.bCylLowReg			= VAR.par5 ;
	regspre.bCylHighReg			= VAR.par6 ;	
	regspre.bDriveHeadReg		= 0x40;									
	regspre.bCommandReg			= SW300_CMD_RWSPI;									
	regspre.bReserved			= 0x00;


	if( !ata_pass_through_ioctl_pio_48b(Devicehandle[device], &regs,&regspre, buf, datasize, IsWR) )
	{
		AddInfo(device,"ata_pass_through_ioctl_pio Failed !",2);
		return 0;
	}
	return 1;
}

BOOL SW300VenderMode(int device,BOOL venderflag)
{
	BYTE buffer[512];
	BOOL ret;
	memset(buffer,0,512);
	strcpy((char*)buffer,"GUCSSDTOOLING.");

	buffer[14]=0xE0;
	if(venderflag)
		buffer[15]=0xAD;
	else
		buffer[15]=0xDA;

	buffer[16]=0x55;
	buffer[17]=0xAA;
	buffer[18]=0x55;
	buffer[19]=0;
	buffer[20]=0xEF;
	ret=WriteSectors(device,0,1,buffer);
	return ret;
}


BOOL SW300RunFW(int device,BYTE* Databuf,DWORD length)
{

	BOOL ret;
	int i,cycle,tmplen;
	SW300_Para VAR;
	VAR.par0=0x38;
	VAR.par1=0x10;
	if(length%(8*1024) > 0)
		cycle=length/(8*1024)+1;
	else
		cycle=length/(8*1024);

	for(i=0;i<cycle;i++)
	{
		VAR.par3=i*16;
		
		if(i==cycle-1)
		{
			VAR.par4=1;
			tmplen=length-i*8*1024;
			if(tmplen>4*1024)
				tmplen=8*1024;
			else
				tmplen=4*1024;
		}
		else
		{
			VAR.par4=0;
			tmplen=8*1024;
		}

		ret=SW300ATARWSPI(device,VAR,1,Databuf+8*1024,tmplen);

		if(ret==0)
			break;
	}
	return ret;
}


BOOL SW300RWSPI(int device,int flag,INT16U sectoroffset,BYTE* sctbuf)
{
	BYTE buffer[512*32];
	BOOL ret;
	memset(buffer,0,32*512);

	buffer[0]='W';
	buffer[1]='S';
	if(flag==0)
		buffer[2]=5;         //read
	else if(flag==1)
		buffer[2]=6;		//write
	buffer[3]=0;  //目标Bank num
	if(flag==0)
		buffer[4]=0x28;		//read
	else if(flag==1)
		buffer[4]=0x29;		//write	
	buffer[5]=0x10;
	buffer[6]=sectoroffset;
	buffer[7]=BYTE(sectoroffset >> 8);
	
	if(flag==0)
	{
		ret=WriteSectors(device,0x5000000,1,buffer);
		if(ret>0)
		{
			ret=ReadSectors(device,0x5000000,1+16,buffer);
			memcpy(sctbuf,buffer+1,16*512);
		}			
	}
	else if(flag==1)
	{
		memcpy(buffer+512,sctbuf,16*512);
		ret=WriteSectors(device,0x5000000,1+16,buffer);
	}
	return ret;
}

BOOL SW300RunRam80(int device,BYTE* Databuf)
{
	BYTE buffer[512*64];
	BOOL ret;
	memset(buffer,0,512*64);
	buffer[0]='W';
	buffer[1]='S';
	buffer[2]=7;
	memcpy(buffer+512,Databuf,62*512);
	ret=WriteSectors(device,0x5000000,62+1,buffer);
	return ret;
}

BOOL SW300RWFSIData(int device,int flag,int ECCflag,int bank,int bus,INT16U sectoroffset,BYTE* sctbuf)
{
	BYTE buffer[512*32];
	BOOL ret;
	memset(buffer,0,32*512);

	buffer[0]='W';
	buffer[1]='S';
	if(flag==0)
		buffer[2]=3;     //read
	else if(flag==1)
		buffer[2]=4;	 //write
	buffer[3]=bank;      //目标Bank num
	buffer[4]=bus;		 //目标bus num	
	buffer[5]=sectoroffset;
	buffer[6]=BYTE(sectoroffset >> 8);
	buffer[7]=ECCflag;
	if(flag==0)
	{
		ret=WriteSectors(device,0x5000000,1,buffer);
		if(ret>0)
		{
			ret=ReadSectors(device,0x5000000,1+16,buffer);
			memcpy(sctbuf,buffer,17*512);
		}			
	}
	else if(flag==1)
	{
		memcpy(buffer+512,sctbuf,16*512);
		ret=WriteSectors(device,0x5000000,1+16,buffer);
	}
	return ret;
}