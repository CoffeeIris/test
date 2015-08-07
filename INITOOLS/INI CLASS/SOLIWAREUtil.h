
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the SOLIWAREUTIL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// SOLIWAREUTIL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef SOLIWAREUTIL_EXPORTS
#define SOLIWAREUTIL_API extern "C" __declspec(dllexport)
#else
#define SOLIWAREUTIL_API extern "C" __declspec(dllimport)
#endif

#define SOLI_ERR_NO_USBKEY					            10
#define SOLI_ERR_WRONG_USBKEY							20
#define SOLI_ERR_NO_FILE					            30
#define SOLI_ERR_WRONG_FORMAT							40
#define SOLI_ERR_WRONG_PARAMETER						50
#define SOLI_ERR_NO_CERT					            60
#define SOLI_ERR_NO_KEYPAIR								70
#define SOLI_ERR_CMD_WRONG					            80
#define SOLI_ERR_WRONG_CERT								90
#define SOLI_ERR_RSA_PUB_ENC_FAILED						100
#define SOLI_ERR_RSA_PRI_DEC_FAILED						110
#define SOLI_ERR_LIB_FAILED								120
#define SOLI_ERR_USER_CERT_INVALIED						130
#define SOLI_ERR_USER_DATA_OVERLOAD						140



//查找目标USBKEY
SOLIWAREUTIL_API int FindToken(void);
//向保护区写入数据，最大1k字节
SOLIWAREUTIL_API int WriteData(char *pin, unsigned char *pDataIn,   long dataLen, long offset);
//从保护区读取数据
SOLIWAREUTIL_API int ReadData  (char *pin, unsigned char *pDataOut, long dataLen, long offset);
//获取指定长度的随机数
SOLIWAREUTIL_API int GetRandom(unsigned char *pBuf, long len);
//修改用户密码或者管理员密码
SOLIWAREUTIL_API int ChangePin(char *oldPin, char *newPin, bool isSo=false);
//修改管理员密码
SOLIWAREUTIL_API int UnlockPin(char *soPin, char *newPin);
//初始化, counter <14
SOLIWAREUTIL_API int InitUsbkey(char *userPin, char *soPin, long counter);
//得到用户密码或者管理员密码剩余尝试次数
SOLIWAREUTIL_API int GetRemains(long *pCounter, bool isSo=false);
//导入公钥,pModulus为公钥的模，必须为128字节
SOLIWAREUTIL_API int ImportPubKey(char *pin, unsigned char *pModulus);
//公钥加密,明文长度必须小于110字节，输出为固定128字节
SOLIWAREUTIL_API int KeyRSAPubKeyEncrypt(unsigned char * bpMsg, long dwMsgLen, unsigned char *  bpOut);


typedef int (*fpFindToken) (void);
typedef int (*fpWriteData)(char* pin,unsigned char* pDataIn,long dataLen,long offset);
typedef int (*fpReadData)(char* pin,unsigned char* pDataOut,long dataLen,long offset);
typedef int (*fpChangePin)(char* oldPin,char* newPin,bool isSo);
typedef int (*fpUnlockPin)(char* soPin,char* newPin);
typedef int (*fpInitUsbkey)(char *userPin, char *soPin,long counter);
typedef int (*fpGetRemains)(long *pCounter, bool isSo);