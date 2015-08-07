#ifndef _COMMANAPI_H_
#define _COMMANAPI_H_

#define  SWSSDPORTINI          "SSDPort_Configure.ini";
#define  DeviceCount 16

typedef struct _SSDList
{
	CString Model;
	CString PNPDeviceID ; 
	CString  DeviceID ;
	int SCSIBus ;
	int SCSILogicalUnit; 
	int SCSIPort;  
	int SCSITargetId; 
	int Slot;
	CString SN;
	int RunStatus;  // 0 SSD就绪  1 量产进行中 2 量产完成 3 SSD移除
	int PCBStatus;  //0 PCB测试失败 1 PCB测试成功
	int BuildTableStatus;
	int IniStatus;
	int LoadStatus;
	int ValidStatus;
	int device;
	int DriveStatus; // 0 不是SW80系列   1  GUCSSD  2 RAMDISK  3 SW80 SSD
}SSDList;

extern SSDList SSdlistItem[DeviceCount];;
BOOL InitSSDListItem();
BOOL ReadSSDListIni(CString file);
int GetSSDPort();
int SetSSDList(SSDList* Sourcelist,int SourcelistLen,SSDList* Devlist,int DevlistLen);
#endif