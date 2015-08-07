#include "StdAfx.h"
#include "SW80API.h"
//word1 length mask offset word2 length mask offset nameCN nameEN
IDFY_DATA IdfyData[256]={
{0,1,0xFFFF,0,0,0,0,0,false,true,"","General Config"},
{1,1,0xFFFF,0,0,0,0,0,false,true,"","Number of logical cylinders"},
{2,1,0xFFFF,0,0,0,0,0,false,true,"","Specific Config"},
{3,1,0xFFFF,0,0,0,0,0,false,true,"","Number of logical heads"},
//{4,2,0xFFFF,0,0,0,0,0,true,true,"","Retired"},
{6,1,0xFFFF,0,0,0,0,0,false,true,"","Number of logical Sectors Per logical track"},
//{7,2,0xFFFF,0,0,0,0,0,false,true,"","Reserved"},
//{9,1,0xFFFF,0,0,0,0,0,true,true,"","Retired"},

{10,10,0xFFFF,0,0,0,0,0,false,true,"","Serial number",true},
{23,4,0xFFFF,0,0,0,0,0,false,true,"","Firmware revision",true},
{27,20,0xFFFF,0,0,0,0,0,false,true,"","Model number",true},
{47,1,0xFF,0,0,0,0,0,false,true,"","Max number of sectors transfer per INT"},
{51,1,0xFF00,8,0,0,0,0,false,true,"","PIO data transfer number"},
//----------------cylinders
{54,1,0xFFFF,0,0,0,0,0,false,true,"","Number of logical cylinders"},
{55,1,0xFFFF,0,0,0,0,0,false,true,"","Number of logical heads"},
{56,1,0xFFFF,0,0,0,0,0,false,true,"","Number of logical Sectors Per logical track"},
{57,2,0xFFFF,0,0,0,0,0,false,true,"","Current capacity in sectors"},

{59,1,0xFF,0,0,0,0,0,false,true,"","Current number of sectors  transfer per INT"},
{59,1,0x100,8,0,0,0,0,false,true,"","Multiple sector setting is valid"},

{60,2,0xFFFF,0,0,0,0,0,false,true,"","Total number of user addressable sectors"},

//-----82 Command support
{82,1,0x4000,14,85,1,0x4000,14,false,true,"","NOP"},
{82,1,0x2000,13,85,1,0x2000,13,false,true,"","READ BUFFER"},
{82,1,0x1000,12,85,1,0x1000,12,false,true,"","WRITE BUFFER"},
{82,1,0x400,10,85,1,0x400,10,false,true,"","Host Protected Area feature set"},
{82,1,0x200,9,85,1,0x200,9,false,true,"","DEVICE RESET"},
{82,1,0x100,8,85,1,0x100,8,false,true,"","SERVICE interrupt"},
{82,1,0x80,7,85,1,0x80,7,false,true,"","release interrupt "},
{82,1,0x40,6,85,1,0x40,6,false,true,"","look-ahead"},
{82,1,0x20,5,85,1,0x20,5,false,true,"","write cache"},
{82,1,0x10,4,85,1,0x10,4,false,true,"","PACKET Command feature set "},
{82,1,0x08,3,85,1,0x08,3,false,true,"","mandatory Power Management feature set"},
{82,1,0x04,2,85,1,0x04,2,false,true,"","Removable Media feature set supported"},
{82,1,0x02,1,85,1,0x02,1,false,true,"","Security Mode feature set supported"},
{82,1,0x01,0,85,1,0x01,0,false,true,"","SMART feature set"},

//-----83 Command support
{83,1,0x2000,13,86,1,0x2000,13,false,true,"","FLUSH CACHE EXT"},
{83,1,0x1000,12,86,1,0x1000,12,false,true,"","mandatory FLUSH CACHE"},
{83,1,0x800,11,86,1,0x800,11,false,true,"","Device Configuration Overlay feature set"},

{83,1,0x400,10,86,1,0x400,10,false,true,"","48-bit Address feature set"},
{83,1,0x200,9,86,1,0x200,9,false,true,"","Automatic Acoustic Management feature set"},
{83,1,0x100,8,86,1,0x100,8,false,true,"","SET MAX security extension"},

{83,1,0x40,6,86,1,0x40,6,false,true,"","SET FEATURES subcommand required to spinup after power-up"},
{83,1,0x20,5,86,1,0x20,5,false,true,"","Power-Up In Standby feature set"},
{83,1,0x10,4,86,1,0x10,4,false,true,"","Removable Media Status Notification feature set  "},
{83,1,0x08,3,86,1,0x08,3,false,true,"","Advanced Power Management feature set"},
{83,1,0x04,2,86,1,0x04,2,false,true,"","CFA feature set "},
{83,1,0x02,1,86,1,0x02,1,false,true,"","READ/WRITE DMA QUEUED "},
{83,1,0x01,0,86,1,0x01,0,false,true,"","DOWNLOAD MICROCODE"},

//-----84 Command support
{84,1,0x2000,13,87,1,0x2000,13,false,true,"","IDLE IMMEDIATE with UNLOAD FEATURE"},

{84,1,0x400,10,87,1,0x400,10,false,true,"","URG bit supported for WRITE STREAM DMA EXT and WRITE STREAM EXT"},
{84,1,0x200,9,87,1,0x200,9,false,true,"","URG bit supported for READ STREAM DMA EXT and READ STREAM EXT"},
{84,1,0x100,8,87,1,0x100,8,false,true,"","64-bit World wide name"},

{84,1,0x80,7,87,1,0x80,7,false,true,"","WRITE DMA QUEUED FUA EXT"},
{84,1,0x40,6,87,1,0x40,6,false,true,"","WRITE DMA FUA EXT and WRITE MULTIPLE FUA EXT"},
{84,1,0x20,5,87,1,0x20,5,false,true,"","General Purpose Logging feature set "},
{84,1,0x10,4,87,1,0x10,4,false,true,"","Streaming feature set"},

{84,1,0x08,3,87,1,0x08,3,false,true,"","Media Card Pass Through Command feature set"},
{84,1,0x04,2,87,1,0x04,2,false,true,"","Media serial number "},
{84,1,0x02,1,87,1,0x02,1,false,true,"","SMART self-test"},
{84,1,0x01,0,87,1,0x01,0,false,true,"","SMART error logging"},

//----88 UDMA
{88,1,0x4000,14,88,1,0x4000,14,false,true,"","Ultra DMA mode 6"},

//78 SATA
{78,1,0x40,6,79,1,0x40,6,false,true,"","Software Settings Preservation"},
{78,1,0x10,4,79,1,0x10,4,false,true,"","In-order data delivery "},
{78,1,0x08,3,79,1,0x08,3,false,true,"","Device initiated power managment"},
{78,1,0x04,2,79,1,0x04,2,false,true,"","DMA Setup auto-activation"},
{78,1,0x02,1,79,1,0x02,1,false,true,"","Non-zero buffer offsets"},
//

{88,1,0x7F00,8,0,0,0,0,false,true,"","Ultra DMA mode"},
{88,1,0x7F,0,0,0,0,0,false,true,"","Ultra DMA mode supported"},
//-----91
{91,1,0xFFFF,0,0,0,0,0,false,true,"","Current advanced power management value"},
{92,1,0xFFFF,0,0,0,0,0,false,true,"","Master Password Revision Code"},
//-------93
{93,1,0x2000,13,0,0,0,0,false,true,"","device detected CBLID- above ViH"},
{93,1,0x0800,11,0,0,0,0,false,true,"","Device 1 asserted PDIAG"},
{93,1,0x0600,9,0,0,0,0,false,true,"","Device 1 Determine Way"},

{93,1,0x40,6,0,0,0,0,false,true,"","Device 0 responds when Device 1 is selected"},
{93,1,0x20,5,0,0,0,0,false,true,"","Device 0 detected the assertion of DASP"},
{93,1,0x10,4,0,0,0,0,false,true,"","Device 0 detected the assertion of PDIAG"},
{93,1,0x08,3,0,0,0,0,false,true,"","Device 0 passed diagnostics"},

{93,1,0x06,1,0,0,0,0,false,true,"","Device 0 Determine Way"},
//---------security
{128,1,0x100,8,0,0,0,0,false,true,"","Security level"},
{128,1,0x20,5,0,0,0,0,false,true,"","Enhanced security erase supported"},

{128,1,0x10,4,0,0,0,0,false,true,"","Security count expired"},
{128,1,0x08,3,0,0,0,0,false,true,"","Security frozen"},

{128,1,0x04,2,0,0,0,0,false,true,"","Security locked"},
{128,1,0x02,1,0,0,0,0,false,true,"","Security enabled"},
{128,1,0x01,0,0,0,0,0,false,true,"","Security supported"},

//SATA
{76,1,0x400,10,0,0,0,0,false,true,"","Supports Phy Event Counters"},
{76,1,0x200,9,0,0,0,0,false,true,"","Supports receipt of host initiated power management requests"},
{76,1,0x100,8,0,0,0,0,false,true,"","Supports native Command Queuing"},
{76,1,0x04,2,0,0,0,0,false,true,"","Supports SATA Gen2 Signaling speed(3.0Gb/s)"},
{76,1,0x02,1,0,0,0,0,false,true,"","Supports SATA Gen1 Signaling speed(1.5Gb/s)"},
//

{255,1,0xFF00,8,0,0,0,0,false,true,"","Checksum"},
{255,1,0xFF,0,0,0,0,0,false,true,"","Signature"}
};




