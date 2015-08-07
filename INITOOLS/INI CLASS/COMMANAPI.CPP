#include "stdafx.h"
#include "COMMANAPI.h"

#include "OPini.h"
#define _WIN32_DCOM
#include <iostream>
using namespace std;
#include <comdef.h>
#include <Wbemidl.h>

# pragma comment(lib, "wbemuuid.lib")

SSDList SSdlistItem[DeviceCount];

BOOL InitSSDListItem()
{
	int i;
	for(i=0;i<DeviceCount;i++)
	{
		SSdlistItem[i].BuildTableStatus=2;
		SSdlistItem[i].DeviceID="";
		SSdlistItem[i].IniStatus=2;
		SSdlistItem[i].LoadStatus=2;
		SSdlistItem[i].Model="";
		SSdlistItem[i].PCBStatus=2;
		SSdlistItem[i].PNPDeviceID="";
		SSdlistItem[i].RunStatus=3;
		SSdlistItem[i].ValidStatus=2;
		SSdlistItem[i].SN="";
		SSdlistItem[i].device=i;
		SSdlistItem[i].DriveStatus=0;
	}	
	return 1;
}

BOOL ReadSSDListIni(CString file)
{
	char str[256];
	char filename[256];
	COPini ini;

    GetModuleFileName(NULL,filename,256); 
    //Scan a string for the last occurrence of a character.
    (strrchr(filename,'\\'))[1] = 0; 
    strcat(filename,file);

	ini.ReadString("A0","Slot",str,filename);
	SSdlistItem[0].Slot=strtol(str,NULL,16);	
	ini.ReadString("A0","SCSIBus",str,filename);
	SSdlistItem[0].SCSIBus=atoi(str);		
	ini.ReadString("A0","SCSILogicalUnit",str,filename);
	SSdlistItem[0].SCSILogicalUnit=atoi(str);	
	ini.ReadString("A0","SCSIPort",str,filename);
	SSdlistItem[0].SCSIPort=atoi(str);	
	ini.ReadString("A0","SCSITargetId",str,filename);
	SSdlistItem[0].SCSITargetId=atoi(str);	

	ini.ReadString("A1","Slot",str,filename);
	SSdlistItem[1].Slot=strtol(str,NULL,16);	
	ini.ReadString("A1","SCSIBus",str,filename);
	SSdlistItem[1].SCSIBus=atoi(str);		
	ini.ReadString("A1","SCSILogicalUnit",str,filename);
	SSdlistItem[1].SCSILogicalUnit=atoi(str);	
	ini.ReadString("A1","SCSIPort",str,filename);
	SSdlistItem[1].SCSIPort=atoi(str);	
	ini.ReadString("A1","SCSITargetId",str,filename);
	SSdlistItem[1].SCSITargetId=atoi(str);	

	ini.ReadString("A2","Slot",str,filename);
	SSdlistItem[2].Slot=strtol(str,NULL,16);	
	ini.ReadString("A2","SCSIBus",str,filename);
	SSdlistItem[2].SCSIBus=atoi(str);		
	ini.ReadString("A2","SCSILogicalUnit",str,filename);
	SSdlistItem[2].SCSILogicalUnit=atoi(str);	
	ini.ReadString("A2","SCSIPort",str,filename);
	SSdlistItem[2].SCSIPort=atoi(str);	
	ini.ReadString("A2","SCSITargetId",str,filename);
	SSdlistItem[2].SCSITargetId=atoi(str);	

	ini.ReadString("A3","Slot",str,filename);
	SSdlistItem[3].Slot=strtol(str,NULL,16);	
	ini.ReadString("A3","SCSIBus",str,filename);
	SSdlistItem[3].SCSIBus=atoi(str);		
	ini.ReadString("A3","SCSILogicalUnit",str,filename);
	SSdlistItem[3].SCSILogicalUnit=atoi(str);	
	ini.ReadString("A3","SCSIPort",str,filename);
	SSdlistItem[3].SCSIPort=atoi(str);	
	ini.ReadString("A3","SCSITargetId",str,filename);
	SSdlistItem[3].SCSITargetId=atoi(str);	

	ini.ReadString("A4","Slot",str,filename);
	SSdlistItem[4].Slot=strtol(str,NULL,16);	
	ini.ReadString("A4","SCSIBus",str,filename);
	SSdlistItem[4].SCSIBus=atoi(str);		
	ini.ReadString("A4","SCSILogicalUnit",str,filename);
	SSdlistItem[4].SCSILogicalUnit=atoi(str);	
	ini.ReadString("A4","SCSIPort",str,filename);
	SSdlistItem[4].SCSIPort=atoi(str);	
	ini.ReadString("A4","SCSITargetId",str,filename);
	SSdlistItem[4].SCSITargetId=atoi(str);	

	ini.ReadString("A5","Slot",str,filename);
	SSdlistItem[5].Slot=strtol(str,NULL,16);	
	ini.ReadString("A5","SCSIBus",str,filename);
	SSdlistItem[5].SCSIBus=atoi(str);		
	ini.ReadString("A5","SCSILogicalUnit",str,filename);
	SSdlistItem[5].SCSILogicalUnit=atoi(str);	
	ini.ReadString("A5","SCSIPort",str,filename);
	SSdlistItem[5].SCSIPort=atoi(str);	
	ini.ReadString("A5","SCSITargetId",str,filename);
	SSdlistItem[5].SCSITargetId=atoi(str);	

	ini.ReadString("A6","Slot",str,filename);
	SSdlistItem[6].Slot=strtol(str,NULL,16);	
	ini.ReadString("A6","SCSIBus",str,filename);
	SSdlistItem[6].SCSIBus=atoi(str);		
	ini.ReadString("A6","SCSILogicalUnit",str,filename);
	SSdlistItem[6].SCSILogicalUnit=atoi(str);	
	ini.ReadString("A6","SCSIPort",str,filename);
	SSdlistItem[6].SCSIPort=atoi(str);	
	ini.ReadString("A6","SCSITargetId",str,filename);
	SSdlistItem[6].SCSITargetId=atoi(str);	

	ini.ReadString("A7","Slot",str,filename);
	SSdlistItem[7].Slot=strtol(str,NULL,16);	
	ini.ReadString("A7","SCSIBus",str,filename);
	SSdlistItem[7].SCSIBus=atoi(str);		
	ini.ReadString("A7","SCSILogicalUnit",str,filename);
	SSdlistItem[7].SCSILogicalUnit=atoi(str);	
	ini.ReadString("A7","SCSIPort",str,filename);
	SSdlistItem[7].SCSIPort=atoi(str);	
	ini.ReadString("A7","SCSITargetId",str,filename);
	SSdlistItem[7].SCSITargetId=atoi(str);	

	ini.ReadString("B0","Slot",str,filename);
	SSdlistItem[8].Slot=strtol(str,NULL,16);	
	ini.ReadString("B0","SCSIBus",str,filename);
	SSdlistItem[8].SCSIBus=atoi(str);		
	ini.ReadString("B0","SCSILogicalUnit",str,filename);
	SSdlistItem[8].SCSILogicalUnit=atoi(str);	
	ini.ReadString("B0","SCSIPort",str,filename);
	SSdlistItem[8].SCSIPort=atoi(str);	
	ini.ReadString("B0","SCSITargetId",str,filename);
	SSdlistItem[8].SCSITargetId=atoi(str);	

	ini.ReadString("B1","Slot",str,filename);
	SSdlistItem[9].Slot=strtol(str,NULL,16);	
	ini.ReadString("B1","SCSIBus",str,filename);
	SSdlistItem[9].SCSIBus=atoi(str);		
	ini.ReadString("B1","SCSILogicalUnit",str,filename);
	SSdlistItem[9].SCSILogicalUnit=atoi(str);	
	ini.ReadString("B1","SCSIPort",str,filename);
	SSdlistItem[9].SCSIPort=atoi(str);	
	ini.ReadString("B1","SCSITargetId",str,filename);
	SSdlistItem[9].SCSITargetId=atoi(str);	

	ini.ReadString("B2","Slot",str,filename);
	SSdlistItem[10].Slot=strtol(str,NULL,16);	
	ini.ReadString("B2","SCSIBus",str,filename);
	SSdlistItem[10].SCSIBus=atoi(str);		
	ini.ReadString("B2","SCSILogicalUnit",str,filename);
	SSdlistItem[10].SCSILogicalUnit=atoi(str);	
	ini.ReadString("B2","SCSIPort",str,filename);
	SSdlistItem[10].SCSIPort=atoi(str);	
	ini.ReadString("B2","SCSITargetId",str,filename);
	SSdlistItem[10].SCSITargetId=atoi(str);	

	ini.ReadString("B3","Slot",str,filename);
	SSdlistItem[11].Slot=strtol(str,NULL,16);	
	ini.ReadString("B3","SCSIBus",str,filename);
	SSdlistItem[11].SCSIBus=atoi(str);		
	ini.ReadString("B3","SCSILogicalUnit",str,filename);
	SSdlistItem[11].SCSILogicalUnit=atoi(str);	
	ini.ReadString("B3","SCSIPort",str,filename);
	SSdlistItem[11].SCSIPort=atoi(str);	
	ini.ReadString("B3","SCSITargetId",str,filename);
	SSdlistItem[11].SCSITargetId=atoi(str);	

	ini.ReadString("B4","Slot",str,filename);
	SSdlistItem[12].Slot=strtol(str,NULL,16);	
	ini.ReadString("B4","SCSIBus",str,filename);
	SSdlistItem[12].SCSIBus=atoi(str);		
	ini.ReadString("B4","SCSILogicalUnit",str,filename);
	SSdlistItem[12].SCSILogicalUnit=atoi(str);	
	ini.ReadString("B4","SCSIPort",str,filename);
	SSdlistItem[12].SCSIPort=atoi(str);	
	ini.ReadString("B4","SCSITargetId",str,filename);
	SSdlistItem[12].SCSITargetId=atoi(str);	

	ini.ReadString("B5","Slot",str,filename);
	SSdlistItem[13].Slot=strtol(str,NULL,16);	
	ini.ReadString("B5","SCSIBus",str,filename);
	SSdlistItem[13].SCSIBus=atoi(str);		
	ini.ReadString("B5","SCSILogicalUnit",str,filename);
	SSdlistItem[13].SCSILogicalUnit=atoi(str);	
	ini.ReadString("B5","SCSIPort",str,filename);
	SSdlistItem[13].SCSIPort=atoi(str);	
	ini.ReadString("B5","SCSITargetId",str,filename);
	SSdlistItem[13].SCSITargetId=atoi(str);	

	ini.ReadString("B6","Slot",str,filename);
	SSdlistItem[14].Slot=strtol(str,NULL,16);	
	ini.ReadString("B6","SCSIBus",str,filename);
	SSdlistItem[14].SCSIBus=atoi(str);		
	ini.ReadString("B6","SCSILogicalUnit",str,filename);
	SSdlistItem[14].SCSILogicalUnit=atoi(str);	
	ini.ReadString("B6","SCSIPort",str,filename);
	SSdlistItem[14].SCSIPort=atoi(str);	
	ini.ReadString("B6","SCSITargetId",str,filename);
	SSdlistItem[14].SCSITargetId=atoi(str);	

	ini.ReadString("B7","Slot",str,filename);
	SSdlistItem[15].Slot=strtol(str,NULL,16);	
	ini.ReadString("B7","SCSIBus",str,filename);
	SSdlistItem[15].SCSIBus=atoi(str);		
	ini.ReadString("B7","SCSILogicalUnit",str,filename);
	SSdlistItem[15].SCSILogicalUnit=atoi(str);	
	ini.ReadString("B7","SCSIPort",str,filename);
	SSdlistItem[15].SCSIPort=atoi(str);	
	ini.ReadString("B7","SCSITargetId",str,filename);
	SSdlistItem[15].SCSITargetId=atoi(str);	
	return 1;
}

int GetSSDPort()
{
    HRESULT hres;
	SSDList list[20];
    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    hres =  CoInitializeEx(0, COINIT_MULTITHREADED); 
    if (FAILED(hres))
    {
        cout << "Failed to initialize COM library. Error code = 0x" 
            << hex << hres << endl;
        return 1;                  // Program has failed.
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------
    // Note: If you are using Windows 2000, you need to specify -
    // the default authentication credentials for a user by using
    // a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
    // parameter of CoInitializeSecurity ------------------------

    //hres =  CoInitializeSecurity(
    //    NULL, 
    //    -1,                          // COM authentication
    //    NULL,                        // Authentication services
    //    NULL,                        // Reserved
    //    RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
    //    RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
    //    NULL,                        // Authentication info
    //    EOAC_NONE,                   // Additional capabilities 
    //    NULL                         // Reserved
    //    );

    //                  
    //if (FAILED(hres))
    //{
    //    cout << "Failed to initialize security. Error code = 0x" 
    //        << hex << hres << endl;
    //    CoUninitialize();
    //    return 1;                    // Program has failed.
    //}
    
    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    IWbemLocator *pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,             
        0, 
        CLSCTX_INPROC_SERVER, 
        IID_IWbemLocator, (LPVOID *) &pLoc);
 
    if (FAILED(hres))
    {
        cout << "Failed to create IWbemLocator object."
            << " Err code = 0x"
            << hex << hres << endl;
        CoUninitialize();
        return 1;                 // Program has failed.
    }

    // Step 4: -----------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

    IWbemServices *pSvc = NULL;
	
    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer pSvc
    // to make IWbemServices calls.
    hres = pLoc->ConnectServer(
         _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
         NULL,                    // User name. NULL = current user
         NULL,                    // User password. NULL = current
         0,                       // Locale. NULL indicates current
         NULL,                    // Security flags.
         0,                       // Authority (e.g. Kerberos)
         0,                       // Context object 
         &pSvc                    // pointer to IWbemServices proxy
         );
    
    if (FAILED(hres))
    {
        cout << "Could not connect. Error code = 0x" 
             << hex << hres << endl;
        pLoc->Release();     
        CoUninitialize();
        return 1;                // Program has failed.
    }

    cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------

    hres = CoSetProxyBlanket(
       pSvc,                        // Indicates the proxy to set
       RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
       RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
       NULL,                        // Server principal name 
       RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
       RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
       NULL,                        // client identity
       EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres))
    {
        cout << "Could not set proxy blanket. Error code = 0x" 
            << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();     
        CoUninitialize();
        return 1;               // Program has failed.
    }

    // Step 6: --------------------------------------------------
    // Use the IWbemServices pointer to make requests of WMI ----

    // For example, get the name of the operating system
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        (L"WQL"), 
   //     bstr_t("SELECT * FROM Win32_PnPSignedDriver "),
        (L"SELECT * FROM Win32_DiskDrive"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
        NULL,
        &pEnumerator);
    
    if (FAILED(hres))
    {
        cout << "Query for operating system name failed."
            << " Error code = 0x" 
            << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 1;               // Program has failed.
    }

    // Step 7: -------------------------------------------------
    // Get the data from the query in step 6 -------------------
 
    IWbemClassObject *pclsObj;
    ULONG uReturn = 0;
	int i=0;
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
            &pclsObj, &uReturn);

        if(0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;


		 // Get the value of the Name property
        hr = pclsObj->Get(L"PNPDeviceID", 0, &vtProp, 0, 0);
		list[i].PNPDeviceID=vtProp.bstrVal;
        VariantClear(&vtProp);

        // Get the value of the Name property
        hr = pclsObj->Get(L"DeviceID", 0, &vtProp, 0, 0);
		list[i].DeviceID=vtProp.bstrVal;
        VariantClear(&vtProp);

		 // Get the value of the Name property
        hr = pclsObj->Get(L"Model", 0, &vtProp, 0, 0);
        list[i].Model=vtProp.bstrVal;
        VariantClear(&vtProp);

		 // Get the value of the Name property
        hr = pclsObj->Get(L"SCSIBus", 0, &vtProp, 0, 0);
		list[i].SCSIBus=vtProp.lVal;
        VariantClear(&vtProp);

        // Get the value of the Name property
        hr = pclsObj->Get(L"SCSILogicalUnit", 0, &vtProp, 0, 0);
		list[i].SCSILogicalUnit=vtProp.lVal;
        VariantClear(&vtProp);

        // Get the value of the Name property
        hr = pclsObj->Get(L"SCSIPort", 0, &vtProp, 0, 0);
		list[i].SCSIPort=vtProp.lVal;
        VariantClear(&vtProp);

        // Get the value of the Name property
        hr = pclsObj->Get(L"SCSITargetId", 0, &vtProp, 0, 0);
		list[i].SCSITargetId=vtProp.lVal;
        VariantClear(&vtProp);
		
        pclsObj->Release();
		i++;
    }
	SetSSDList(list,i,SSdlistItem,DeviceCount);


    // Cleanup
    // ========
    
    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();

    return 0;   // Program successfully completed.
	
}


int SetSSDList(SSDList* Sourcelist,int SourcelistLen,SSDList* Devlist,int DevlistLen)
{
	int i,j;
	for(i=0;i<DevlistLen;i++)
	{
		for(j=0;j<SourcelistLen;j++)
		{
			//找到新设备
			if(Devlist[i].SCSIBus==Sourcelist[j].SCSIBus && Devlist[i].SCSILogicalUnit==Sourcelist[j].SCSILogicalUnit &&
				Devlist[i].SCSIPort==Sourcelist[j].SCSIPort && Devlist[i].SCSITargetId==Sourcelist[j].SCSITargetId && Sourcelist[j].SCSITargetId!=0)
			{
				if(Devlist[i].RunStatus==3)
				{
					Devlist[i].Model=Sourcelist[j].Model;
					Devlist[i].PNPDeviceID=Sourcelist[j].PNPDeviceID;
					Devlist[i].DeviceID=Sourcelist[j].DeviceID;
					Devlist[i].RunStatus=0;
				}
				break;

			}
		}

		//在原表中没有找到设备，删除目标表中设备,并把变量置为初始值
		if(j==SourcelistLen && Devlist[i].RunStatus!=3)
		{
			Devlist[i].Model.Empty();
			Devlist[i].PNPDeviceID.Empty();
			Devlist[i].DeviceID.Empty();
			Devlist[i].SN.Empty();
			Devlist[i].RunStatus=3;
			Devlist[i].BuildTableStatus=2;
			Devlist[i].IniStatus=2;
			Devlist[i].LoadStatus=2;
			Devlist[i].PCBStatus=2;
			Devlist[i].ValidStatus=2;
		}
	}
	return 1;
}