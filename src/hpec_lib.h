#ifndef __HPEC_LIB_H__
#define __HPEC_LIB_H__
#include "hl_lib.h"

#include <Windows.h>
#include <time.h>

#define PRODUCT_HPEC 1

#if PRODUCT_HPEC
#define NETWORK_HPEC 0
#else
#define NETWORK_HPEC 1
#endif

/*CPU板状态*/
#define UNINIT_DEVICE 0x00000FF7 // 未初始化
#define TIME_OUT_FUNC 0xFFFFFFFF // 函数等待超时
#define RECONFIGING 0x00000013   // 正在重构或者烧写FLASH
#define TRANSFERRING 0x00000014  // 正在进行数据传输

#define WRITEREGSUCCESS 0x00001021        // 写寄存器命令发送成功
#define WRITEREGFAILED 0x00001022         // 写寄存器命令格式有误
#define READREGCTRLSUCCESS 0x00001033     // 读寄存器命令发送成功
#define READREGCTRLFAILED 0x00001034      // 读寄存器命令格式有误
#define READREGSUCCESS 0x00001031         // 读寄存器成功
#define READREGFAILED 0x00001032          // 读寄存器格式有误
#define RECONFIGSUCCESS 0x00001041        // 重构/链路扫描命令发送成功
#define RECONFIGFAILED 0x00001042         // 重构/链路扫描命令格式有误
#define WRITEBINTOFLASHFAILED 0x00001061  // 上传flash命令发送成功
#define WRITEBINTOFLASHSUCCESS 0x00001062 // 上传flash命令格式有误

#define TRANSFERCTRLSUCCESS 0x00001071     // 传输命令发送成功
#define TRANSFERCTRLFAILED 0x00001072      // 传输命令格式有误
#define NONETRANSFERLENGTH 0x00001073      // 传输长度超出范围
#define STOPTRANSFERCTRLSUCCESS 0x00001081 // 结束传输命令发送成功
#define STOPTRANSFERCTRLFAILED 0x00001082  // 结束传输命令格式有误
#define SENDCHANGESAMPLESUCCESS 0x00001091 // 修改采样量命令发送成功
#define SAMPLEERROR 0x00001093             // 采样量错误
#define CHANNELERROR 0x00001094            // 通道号错误
#define CHANGETRANSSPEEDSUCCESS 0x00001101 // 修改传输速度成功
#define CHANGETRANSSPEEDFAILED 0x00001102  // 修改传输速度失败

#define STARTWRITETRANSFERCTRLSUCCESS 0x00001111 // 下行传输通道打开成功
#define STARTWRITETRANSFERCTRLFAILED 0x00001112  // 下行传输通道打开失败
#define STOPWRITETRANSFERCTRLSUCCESS 0x00001121  // 下行传输通道关闭成功
#define STOPWRITETRANSFERCTRLFAILED 0x00001122   // 下行传输通道关闭失败

#define STARTREADDDRSUCCESS 0x00001131 // fpga开始读取DDR成功
#define STARTREADDDRFAILED 0x00001132  // fpga开始读取DDR失败
#define STOPREADDDRSUCCESS 0x00001141  // fpga结束读取DDR成功
#define STOPREADDDRFAILED 0x00001142   // fpga结束读取DDR失败
#define BLOCKWRITESUCCESS 0x00001151   // 块写寄存器成功
#define BLOCKWRITEFAILED 0x00001152    // 块写寄存器失败
#define BLOCKREADSUCCESS 0x00001161    // 块读寄存器成功
#define BLOCKREADFAILED 0x00001162     // 块读寄存器失败
#define WRITESPIDATASUCCESS 0x00001171 // 写SPI数据成功
#define WRITESPIDATAFAILED 0x00001172  // 写SPI数据失败
#define READSPIDATASUCCESS 0x00001181  // 读SPI数据成功
#define READSPIDATAFAILED 0x00001182   // 读SPI数据失败

#define RESETPPCSUCCESS 0x00001211 // 重置PPC成功
#define RESETPPCFAIL 0x00001212    // 重置PPC失败

#define CREATEFILECTRLSUCCESS 0x00002011       // 创建文件命令发送成功
#define CREATEFILECTRLFAIL 0x00002012          // 创建文件命令失败
#define FPGANUMBERERROR 0x00002013             // 115号错误
#define FPGACHANNELERROR 0x00002014            // 115通道错误
#define SIZEERROR 0x00002015                   // 文件大小错误
#define STOP_FILL_FILE_SUCCESS 0x00002021      // 停止填充文件成功
#define STOP_FILL_FILE_FAIL 0x00002022         // 停止填充文件失败
#define FILETRANSFERCTRLSUCCESS 0x00002031     // 文件传输命令发送成功
#define FILETRANSFERCTRLFAIL 0x00002032        // 文件传输命令发送失败
#define STOPFILETRANSFERCTRLSUCCESS 0x00002041 // 文件结束传输命令发送成功
#define STOPFILETRANSFERCTRLFAIL 0x00002042    // 文件结束传输命令发送失败
#define GETDIRSUCCESS 0x00002051               // 获取目录成功
#define GETDIRFAIL 0x00002052                  // 获取目录失败
#define GET_DIRETORY_CODE_ERROR 0x00002053     // 目录乱码
#define DELETEFILESUCCESS 0x00002061           // 删除文件成功
#define DELETEFILEFAIL 0x00002062              // 删除文件失败
#define FORMATSUCCESS 0x00002071               // 格式化成功
#define FORMATFAIL 0x00002072                  // 格式化失败

/*PowerPC返回状态*/
#define NONESTATUS 0x00000000             // 默认状态
#define INITDEVICESUCCESS 0x00000002      // 初始化设备成功
#define READREGISTERSUCCESS 0x00001001    // 读寄存器成功
#define WRITEREGISTERSUCCESS 0x00002001   // 写寄存器成功
#define UPLOADMEMORYSUCCESS 0x0000000c    // 上传PowerPc内存成功
#define UPLOADMEMORYFAILD 0x0000000d      // 上传PowerPc内存失败
#define RECONFIGURESUCCESS 0x00003001     // 重构/链路扫描成功
#define RECONFIGUREFAILED 0x00003002      // 重构失败
#define PROGRAMFLASHSUCCESS 0x00007001    // flash保存bin文件成功
#define PROGRAMFLASHFAILED 0x00007002     // flash保存bin文件失败
#define BINFLASHING 0x00007003            // 正在烧写flash
#define UPLOADTOFPGAONESUCCESS 0x0000a001 // 上传1号115成功
#define UPLOADTOFPGATWOSUCCESS 0x0000a003 // 上传2号115成功
#define UPLOADTOFPGAONEFAILED 0x0000a002  // 上传1号115失败
#define UPLOADTOFPGATWOFAILED 0x0000a004  // 上传2号115失败
#define UPLOADINGFPGA 0x0000a005          // 正在上传115

#define STARTTRANSFERONE 0x00000101       // 1号数据流开始传输
#define TRANSFERFAILEDONE 0x00000102      // 1号数据流传输失败
#define STARTTRANSFERTWO 0x00000103       // 2号数据流开始传输
#define TRANSFERFAILEDTWO 0x00000104      // 2号数据流传输失败
#define ALSTARTTRANSFERONE 0x00000105     // 1号流数据已经开始传输
#define ALSTARTTRANSFERTWO 0x00000106     // 2号流数据已经开始传输
#define STOPTRANSFERONE 0x00000301        // 1号数据流结束传输
#define STOPTRANSFERTWO 0x00000303        // 2号数据流结束传输
#define CHANGESPEEDSUCCESS 0x00008001     // 修改速度成功
#define CHANGESPEEDFAIL 0x00008002        // 修改速度失败
#define CHNAGESAMPLESUCCESSONE 0x00009001 // 1号通道采样量修改成功
#define CHNAGESAMPLESUCCESSTWO 0x00009003 // 2号通道采样量修改成功

#define STARTWRITETRANSFER 0x0000b001  // 下行流数据成功
#define WRITETRANSFERFAILED 0x0000b002 // 下行流数据失败
#define STOPWRITETRANSFER 0x0000b003   // 停止下行流数据（未使用）
#define STARTREADDDR 0x0000c001        // 开始读取DDR
#define STOPREADDDR 0x0000d001         // 停止读取DDR
#define READF1D1 0x0000c002            // fpga读FPGA1DDR1成功
#define READF1D2 0x0000c003            // fpga读FPGA1DDR2成功
#define READF2D1 0x0000c004            // fpga读FPGA2DDR1成功
#define READF2D2 0x0000c005            // fpga读FPGA2DDR2成功
#define BLOCK_READ 0x0000e001          // 块读寄存器中断（告知上层在读1Mbuffer中存在数据）
#define BLOCK_WRITE_SUCCESS 0x0000f001 // 块写寄存器成功
#define BLOCK_WRITE_FAILED 0x0000f002  // 块写寄存器失败

#define SPI_READ_DATA_SUCCESS 0x00000301  // 读SPI数据成功
#define SPI_READ_DATA_FAIL 0x00000302     // 读SPI数据失败
#define SPI_WRITE_DATA_SUCCESS 0x00000401 // 写SPI数据成功
#define SPI_WRITE_DATA_FAIL 0x00000402    // 写SPI数据失败

#define RESETSUCCESS 0x00200001       // PowerPC重置成功
#define RESETFAILED 0x00200002        // PowerPC重置失败
#define GETSELFTESTSUCCESS 0x00300001 // 获取自检信息成功

/*************新建文件部分***********************/
#define CREATE_DIRECTORY_SUCEESS 0x00010001 // 创建文件目录成功
#define FILENAME_EXISTING 0x00010005        // 文件名已经存在
#define SPACE_NO_ENOUGH 0x00010004          // 空间不足，无法创建文件
#define CREATE_DIRECTORY_FAILED 0x00010002  // 创建文件目录失败
#define CREATE_FILE_SUCCESS 0x00010011      // 新建文件成功
#define FILE_NUMBER_OUT 0x00010003          // 文件数量超过限制
#define CHANNEL_DAMAGED 0x00010006          // 底层链路出错，崩溃

/****************文件读写部分*******************/
#define NO_FIND_FILE 0x00010007  // 找不到该文件
#define FILE_BUSY 0x00010009     // 文件正在读或者写，请等待(被占用)
#define FILE_SIZE_OUT 0x00000109 // 文件大小超出范围（写或者读的指定长度超过文件长度）
#define READ_OR_WRITE_FILE_ERROR 0x0000010A              // 读/写错误
#define GET_DIRECTORY_SUCCESS 0x00030001                 // 获取目录成功
#define GET_DIRETORY_FAILED 0x00030002                   // 获取目录失败
#define FILE_START_TRANSFER_SUCCESS 0x00040001           // 开始下载文件
#define FILE_STOP_TRANSFER_SUCCESS 0x00050001            // 结束下载文件
#define OPEN_CHANNEL_SUCCESS 0x0000010F                  // 建立通道成功
#define CLOSE_CHANNEL_SUCCESS 0x00000110                 // 通道关闭
#define CHANGE_SAMPLE_SUCCESS 0x00000111                 // 采样量修改成功
#define STOP_CREATE_FILE_SUCCESS 0x00020001              // 停止填充文件成功
#define STOP_CREATE_FILE_FAILED 0x00020002               // 停止填充文件失败
#define DELETE_FILE_SUCCESS 0x00060001                   // 删除文件成功
#define DELETE_FILE_FAILED 0x00060002                    // 删除文件失败
#define FORMAT_SUCCESS 0x00100001                        // 格式化成功
#define FORMAT_FAILED 0x00100002                         // 格式化失败
#define OPEN_CHANNEL_FAILED 0x00000118                   // 建立通道失败
#define FILE_STRAT_TRANSFER_FAILED 0x00000119            // 文件下载失败
#define STORAGEBOARD_BANDWIDTH_ASSIGN_SUCCESS 0x00400001 // 修改板间带宽比成功
#define STORAGEBOARD_BANDWIDTH_ASSIGN_FAIL 0x00400002    // 修改板间带宽比失败

/*单次采样量*/
#define SAMPLE_SIZE_4MB 0x400000
#define SAMPLE_SIZE_2MB 0x200000
#define SAMPLE_SIZE_1MB 0x100000
#define SAMPLE_SIZE_512KB 0x80000
#define SAMPLE_SIZE_256KB 0x40000
#define SAMPLE_SIZE_128KB 0x20000
#define SAMPLE_SIZE_64KB 0x10000
#define SAMPLE_SIZE_32KB 0x8000
#define SAMPLE_SIZE_16KB 0x4000
#define SAMPLE_SIZE_8KB 0x2000
#define SAMPLE_SIZE_4KB 0x1000
#define SAMPLE_SIZE_3MB_512KB 0x380000
#define SAMPLE_SIZE_3MB 0x300000
#define SAMPLE_SIZE_2MB_512KB 0x280000
#define SAMPLE_SIZE_1MB_512KB 0x180000

#define DISKBLOCK 0x400000
#define MAX_FPGA_CHL_NUMS 256

#define FILE_NAME_LEN_FS 32
#define FILE_NAME_LEN_PC 160

/*通道*/
#define CHANNEL_ONE 0x0001
#define CHANNEL_TWO 0x0002

/*传输模式*/
#define TRANSFER_MODE_FOREVER 0x2001
#define TRANSFER_MODE_LENGTH 0x2002 //(未使用）
#define TRANSFER_MODE_BLOCK 0x2003

#define RW_DATA_OK 0x8000
#define RW_DATA_TIMEOUT 0x8001
#define RW_DATA_ERROR 0x8002

typedef struct hFile_Paramter
{
    char name[160];        // 文件名
    ULONG fpgaNumber;      // 文件数据来源的FPGA号
    ULONG fpgaChannel;     // 文件数据来源的信道号
    ULONG AllocSize;       // 文件大小
    ULONG DataSize;        // 文件数据大小
    char busy_reserved[3]; // 保留
    char busy;             // 占用位
    char One_reserved[4];  // 保留
    struct tm moditime[1]; // 创建时间
    char Two_reserved[36]; // 保留
} _FILE_PROPERTY;

typedef struct hDirectory_Head
{
    ULONG reserved;   // 保留
    ULONG VolSize;    // 硬盘空间
    ULONG FreeSize;   // 硬盘剩余空间
    ULONG FileNumber; // 文件数量
} _HEAD_DIRECTORY;

typedef struct hDirctory_Buffer
{
    _HEAD_DIRECTORY Head;
    _FILE_PROPERTY FileQueue[2048];
} _DIRECTORY;

/*调试接口（未使用）*/
void pci_fifo_show(int hDev);

/*检测PowerPC是否存在*/
int PowerPCExitCheck();
/*
函数返回：
-1		未检测到PowerPC
1		检测到PowerPC
*/

/*初始化启动应用程序时所需的配置*/
/*注意：
应用程序开启阶段调用，只能调用一次
*/
int InitApp(int bus_num // 设备总线号，从设备管理器查看（相同背板的机箱的总线号是一直固定的）
);
/*函数固定写法：InitApp(hSemaphore_PciStatus);*/
/*函数返回：
-1		总线号小于0，初始化失败
0		成功
*/

/*连接设备*/
int OpenDevice(int bus_num // 设备总线号，从设备管理器查看（相同背板的机箱的总线号是一直固定的）
);
/*函数固定写法：hDevice= OpenDevice();*/
/*函数返回值
-1                      设备未成功连接
0                       设备连接成功
*/

/*初始化设备，一般与连接设备一起使用*/
void InitDevice(int hDev // 类型：输入，参数输入值固定，即hDevice
);
/*函数固定写法：InitDevice(hDevice);*/

/*异步读取PowerPC的返回状态*/
void FileReadPciStatus(int hDev,             // 类型：输入，参数输入值固定，即hDevice
                       ULONG& PciStatus_Api, // 类型：输出，PowerPC返回状态
                       ULONG& Channel_Api,   // 类型：输出，PowerPC返回状态对应的通道
                       char* FileName_Api    // 类型：输出，PowerPC返回状态对应的文件名
);

/*断开设备*/
void CloseDevice(int hDev // 类型：输入，参数输入值固定，即hDevice
);
/*函数固定写法：CloseDevice(hDevice);*/

/*关闭应用程序*/
/*注意：
退出应用程序时调用
*/
void CloseApp(int hDev // 类型：输入，参数输入值固定，即hDevice
);
/*函数固定写法：CloseApp(hDevice);*/

/*写19eg寄存器*/
ULONG WriteRegTo19eg(int hDev,                 // 类型：输入，参数输入值固定，即hDevice
                     ULONG RegisterOffset_Api, // 类型：输入，寄存器号，输入值：0~255（10进制）
                     ULONG RegisterData_Api    // 类型：输入，寄存器值，输入值：0~0xFFFFFFFF
);

ULONG ReadRegFrom19eg(int hDev, ULONG RegisterOffset_Api, ULONG& RegisterData_Api);

/*写115寄存器*/
ULONG WriteRegToFpga(int hDev,                 // 类型：输入，参数输入值固定，即hDevice
                     UCHAR Channel_Api,        // 类型：输入，通道选择，输入值：1或2
                     ULONG RegisterNumber_Api, // 类型：输入，寄存器号，输入值：0~255（10进制）
                     ULONG RegisterData_Api    // 类型：输入，寄存器值，输入值：0~0xFFFFFFFF
);
/*示例：WriteRegToFpga(hDevice,1,124,0xFFFFFFFF);*/
/*函数返回值
WRITEREGFAILED          写寄存器命令格式有误
WRITEREGSUCCESS         写寄存器命令发送成功
RECONFIGING				正在重构
TIME_OUT_FUNC			写寄存器超时

PowerPC返回值
WRITEREGISTERSUCCESS    写寄存器成功
*/

/*读115寄存器命令*/
ULONG ReadRegCtrlToFpga(int hDev,                // 类型：输入，参数输入值固定，即hDevice
                        UCHAR Channel_Api,       // 类型：输入，通道选择，输入值：1或2
                        ULONG RegisterNumber_Api // 类型：输入，寄存器号，输入值：0~255（10进制）
);
/*示例：ReadRegCtrlToFpga(hDevice,1,124);*/
/*函数返回值
READREGCTRLFAILED           读寄存器命令格式有误
READREGCTRLSUCCESS          读寄存器命令发送成功
RECONFIGING					正在重构

PowerPC返回值
READREGISTERSUCCESS         读寄存器成功
*/

/*读115寄存器*/
/*注意：
先发送读115寄存器命令，等到PowerPC返回READREGISTERSUCCESS时，再开线程去读寄存器值
*/
ULONG ReadRegFromFpga(int hDev,                 // 类型：输入，参数输入值固定，即hDevice
                      UCHAR Channel_Api,        // 类型：输入，通道选择，输入值：1或2
                      ULONG RegisterNumber_Api, // 类型：输入，寄存器号，输入值：0~255（10进制）
                      ULONG& RegisterData_Api   // 类型：输出，寄存器值，输出值：0~0xFFFFFFFF
);
/*示例：ReadRegFromFpga(hDevice,1,124,RegisterData);*/
/*函数返回值
READREGFAILED           读寄存器格式有误
READREGSUCCESS         读寄存器成功
*/

/*重构/链路扫描115*/
/*注意：
1.重构/链路扫描命令发送成功后，直到PowerPC返回RECONFIGURESUCCESS或RECONFIGUREFAILED前不能发起任何操作
2.当还存在发送出去的命令PowerPC没有返回的情况下，不能发起重构/链路扫描命令
3.当两片115重构bin文件选择都是0时，默认为重新链路扫描不进行重构
*/
ULONG ReConfigureFpga(int hDev, // 类型：输入，参数输入值固定，即hDevice
                      UCHAR Channel_Api,
                      int BinNumber1, // 类型：输入，1号115重构bin文件选择，输入值0~8
                      int BinNumber2  // 类型：输入，2号115重构bin文件选择，输入值0~8
);
/*示例：ReConfigureFpga(hDevice,1,3);*/
/*函数返回值
RECONFIGFAILED           重构/链路扫描命令格式有误
RECONFIGSUCCESS          重构/链路扫描命令发送成功
CHANNELBUSY              正在传输
RECONFIGING				 正在重构

PowerPC返回值
RECONFIGURESUCCESS      重构/链路扫描成功
RECONFIGUREFAILED        重构/链路扫描失败
*/

/*上传Bin文件重构115*/
/*注意：
1.重构/链路扫描命令发送成功后，直到PowerPC返回RECONFIGURESUCCESS或RECONFIGUREFAILED前不能发起任何操作
2.当还存在发送出去的命令PowerPC没有返回的情况下，不能发起重构/链路扫描命令
3.必须先分配1个48MB的内存空间，并将需要重构的bin文件读到内存中，再将首地址赋给函数
*/
ULONG ReConfigCtrlWithBin(int hDev,          // 类型：输入，参数输入值固定，即hDevice
                          UCHAR Channel_Api, // 类型：输入，通道选择，输入值：1或2
                          char* buffer,      // 类型：输入，存放Bin文件的48MB内存首地址
                          unsigned int size);
/*ReConfigCtrlWithBin(hDevice,1,BufferForBin);*/

/*上传Bin文件到flash*/
/*注意：
1.flash命令发送成功后，直到PowerPC烧写flash结束前不能发起任何操作
2.当还存在发送出去的命令PowerPC没有返回的情况下，不能发起flash命令
*/
ULONG WriteBinToFlash(int hDev,            // 类型：输入，参数输入值固定，即hDevice
                      ULONG BinNumber_Api, // 类型：输入，flash位置选择，输入值：1~8
                      ULONG BinLength_Api, // 类型：输入，上传文件长度
                      char* buffer         // 类型：输入，存放文件的内存首地址
);
/*WriteBinToFlash(hDevice,1,0x3000000,BufferForBin);*/
/*函数返回值
WRITEBINTOFLASHFIALD            上传flash命令格式有误
WRITEBINTOFLASHSUCCESS          上传flash命令发送成功
CHANNELBUSY						正在传输

PowerPC返回值
PROGRAMFLASHSUCCESS             flash保存bin文件成功
PROGRAMFLASHFAILED              flash保存bin文件失败
*/

/*获取自检信息命令*/
ULONG GetTestInformationCtrl(int hDev // 类型：输入，参数输入值固定，即hDevice
);
/*
函数返回值：
0                     正常
RECONFIGING           正在重构
PowerPC返回值
GETSELFTESTSUCCESS    获取自检信息成功（收到该状态后调用GetTestInformation()）
*/

/*获取自检信息*/
void GetTestInformation(int hDev,              // 类型：输入，参数输入值固定，即hDevice
                        char* InforBuffer_Api, // 类型：输出，存放自检信息的缓冲区地址
                        ULONG BufferSize_Api   // 类型：输入，缓冲区大小
);

/*重置PowerPC命令*/
void Reset19EG(int hDev // 类型：输入，参数输入值固定，即hDevice
);
/*PowerPC返回值
RESETSUCCESS                     PowerPC重置成功
RESETFAILED                      PowerPC重置失败
*/

/*重置App和驱动命令*/
void ResetAppAndDriver(int hDev // 类型：输入，参数输入值固定，即hDevice
);

/*开始数据流传输*/
/*注意：
在没有结束对应通道的传输情况下，不能发起相同通道的传输命令，比如：先发送了1号传输命令后，在没有结束1号传输情况下，又发起1号传输命令，这是不被允许的
*/
ULONG StratTransferDataFromFpga(
    int hDev,          // 类型：输入，参数输入值固定，即hDevice
    UCHAR Channel_Api, // 类型：输入，通道选择，输入值：1或2
    ULONG Sample_Api, // 类型：输入，单次采样量选择，输入值：SAMPLE_SIZE_MAX、SAMPLE_SIZE_4MB、SAMPLE_SIZE_1MB、SAMPLE_SIZE_256KB、SAMPLE_SIZE_64KB
                      // （TRANSFER_MODE_BLOCK模式下固定为SAMPLE_SIZE_4MB）
    ULONG
        Mode_Api, // 类型：输入，传输模式选择，输入值：TRANSFER_MODE_FOREVER、TRANSFER_MODE_LENGTH、TRANSFER_MODE_BLOCK
    ULONG Length_Api // 类型：输入，传输长度，输入值：NULL（TRANSFER_MODE_FOREVER）、0~0xC0000000KB 即
                     // 最大3TB（TRANSFER_MODE_LENGTH）、0~0x200000KB 即 最大2GB（TRANSFER_MODE_BLOCK）
);
/*示例：StratTransferDataFromFpga(hDevice,1,SAMPLE_SIZE_MAX,TRANSFER_MODE_FOREVER,NULL);*/
/*函数返回值
TRANSFERCTRLFAILED          传输命令格式有误
TRANSFERCTRLSUCCESS         传输命令发送成功
NONETRANSFERLENGTH          传输长度超出范围
RECONFIGING					正在重构

PowerPC返回值
STARTTRANSFERONE            1号数据流开始传输
STARTTRANSFERTWO            2号数据流开始传输
TRANSFERFAILEDONE           1号数据流传输失败
TRANSFERFAILEDTWO           2号数据流传输失败
*/

/*结束数据流传输*/
ULONG StopTransferDatafromFpga(int hDev,         // 类型：输入，参数输入值固定，即hDevice
                               UCHAR Channel_Api // 类型：输入，通道选择，输入值：1或2
);
/*示例：StopTransferDatafromFpga(hDevice,1);*/
/*函数返回值
STOPTRANSFERCTRLFAILED          结束传输命令格式有误
STOPTRANSFERCTRLSUCCESS         结束传输命令发送成功
RECONFIGING						正在重构

PowerPC返回值
STOPTRANSFERONE                 1号数据流结束传输
STOPTRANSFERTWO                 2号数据流结束传输
*/

/*读取FPGA的数据到CPU板内存*/
/*注意：
1.先发起开始传输命令，再开线程运行该函数获取数据，记得判断是否有数据返回
2.缓冲区必须大于4MB
3.如果有数据返回，则根据对应通道的单次采样量从缓冲区内获取相应的有效数据，比如：1号数据流采样量为SAMPLE_SIZE_64KB，则如果数据来源于1号数据流，那么缓冲区的0~64KB为有效数据,64KB以后为无效数据
4.单次采样量与有效数据对应情况：SAMPLE_SIZE_MAX(0~4MB)、SAMPLE_SIZE_4MB(0~4MB)、SAMPLE_SIZE_1MB(0~1MB)、SAMPLE_SIZE_256KB(0~256KB)、SAMPLE_SIZE_64KB(0~64KB)
*/
unsigned int ReadDataFromFpga(
    int hDev,     // 类型：输入，参数输入值固定，即hDevice
    char* buffer, // 类型：输入，参数输入值固定，输入值：1个4MB大小的缓冲区
    UCHAR&
        Channel_Api, // 类型：输出，输出值：当前数据块的来源（数值：1:数据来源于1号、2：数据来源于2号、0：没有读取到FPGA的数据）
    ULONG& Data_Type,
    ULONG& nGet, // 类型：输出，输出值：是否读取到了FPGA的数据（数值：0:没有读取到、其它:读取FPGA的数据长度）
    unsigned int wait_ms // 类型：输入，超时时间，单位ms， 0xFFFFFFFF代表永久等待
);
/*示例：
UCHAR Channel_Src=0;
ULONG nRead=0;
char *buffer=(char*)malloc(sizeof(char)*4*1024*1024);
memset(buffer,0,4*1024*1024);
ReadDataFromFpga(hDevice,buffer,Channel_Src,nRead);
if(Channel_Src!=0)
{
//此处添加相应的数据处理代码
}
*/

/*修改通道采样量*/
ULONG ChangeSampleSize(int hDev,          // 类型：输入，参数输入值固定，即hDevice
                       ULONG Channel_Api, // 类型：输入，通道选择，输入值：1或2
                       ULONG Sample_Api   // 类型：输入，采样量
);
/*函数返回值
SAMPLEERROR                         采样量错误
CHANNELERROR                        当前传输通道错误
SENDCHANGESAMPLESUCCESS				修改采样量命令发送成功
RECONFIGING							正在重构

PowerPC返回值
CHNAGESAMPLESUCCESSONE				1号通道采样量修改成功
CHNAGESAMPLESUCCESSTWO				2号通道采样量修改成功
*/

/*修改传输速度*/
ULONG ChangeSpeed(int hDev,          // 类型：输入，参数输入值固定，即hDevice
                  ULONG Channel_Api, // 类型：输入，通道选择，输入值：1或2
                  ULONG Speed_Api    // 类型：输入，速度，单位MB/s
);
/*函数返回值
CHANGETRANSSPEEDSUCCESS				修改传输速度成功
CHANGETRANSSPEEDFAILED				修改传输速度失败
RECONFIGING							正在重构

PowerPC返回值
CHANGESPEEDSUCCESS					修改传输速度成功
CHANGESPEEDFAIL						修改传输速度失败
*/

/*开始传输前和结束传输后对传输参数的初始化(未使用)*/
void InitTransfer(int hDev // 类型：输入，参数输入值固定，即hDevice
);

/*上传数据到PowerPC内存（未使用）*/
ULONG WriteToPowerPCMemory(int hDev,                    // 类型：输入，参数输入值固定，即hDevice
                           char* buffer,                // 类型：输入，数据内存首地址
                           ULONG MemoryOffsetToPowerPC, // 类型：输入，PowerPC端内存偏移
                           ULONG BufferLengthToPowerPC  // 类型：输入，数据长度（最大16MB）
);
/*函数返回值
WRITEMEMORYSUCCESS               上传PowerPc内存命令发送成功
WRITEMEMORYFAILD                 上传PowerPc内存命令有误
RECONFIGING						 正在重构

PowerPC返回值
UPLOADMEMORYSUCCESS				 上传PowerPc内存成功
UPLOADMEMORYFAILD				 上传PowerPc内存失败
*/

/*上传数据到Fpga（未使用）*/
ULONG WriteToFpga(int hDev,         // 类型：输入，参数输入值固定，即hDevice
                  char* buffer,     // 类型：输入，数据内存首地址，数据大小8KB
                  ULONG Channel_Api // 类型：输入，上传数据目标Fpga号码，1~2
);

/*函数返回值
WRITEMEMORYSUCCESS              上传PowerPc内存命令发送成功
WRITEMEMORYFAILD                上传PowerPc内存命令有误
RECONFIGING						正在重构

PowerPC返回值
UPLOADTOFPGAONESUCCESS			上传1号115成功
UPLOADTOFPGATWOSUCCESS			上传2号115成功
UPLOADTOFPGAONEFAILED			上传1号115失败
UPLOADTOFPGATWOFAILED			上传2号115失败
*/

/*发送控制消息给PowerPC（未使用）*/
/*不要过于频繁的发送该消息，上限：一秒内发送十次*/
ULONG SendCtrlToPPC(int hDev,      // 类型：输入，参数输入值固定，即hDevice
                    ULONG Ctrl_Api // 类型：输入，控制消息，取值0x10000000-0x9fffffff
);
/*SendCtrlToPPC(hDevice,0x10000000);*/
/*函数返回值
CTRLOVERFLOW                      消息超出范围
CTRLFREQUENTLY                    消息发送太频繁
CTRLSUCCESS                       往PowerPC发送消息成功
*/

/*2018.10.19更新*/
/*类似于从FPGA读数据流传输，添加向FPGA写数据流传输通路*/
/*开始写流传输命令*/
ULONG StartTransferDataToFpga(
    int hDev, // 类型：输入，参数输入值固定，即hDevice
    ULONG
        DDRBaseAddr_Api // 类型：输入，DDR或BRAM基址，如果是DDR则输入值为1字节单位，如果BRAM则输入值为4字节单位，下发4240后，4240会*4
);
/*
函数返回值：
STARTWRITETRANSFERCTRLSUCCESS		下传通道打开成功
STARTWRITETRANSFERCTRLFAILED		下传通道打开失败
*/

/*结束写流传输命令*/
ULONG StopTransferDataToFpga(int hDev // 类型：输入，参数输入值固定，即hDevice
);
/*
函数返回值：
STOPWRITETRANSFERCTRLSUCCESS		下传通道关闭成功
STOPWRITETRANSFERCTRLFAILED			下传通道关闭失败
*/

/*将CPU板内存中的数据写入FPGA*/
unsigned int WriteDataToFpga(int hDev,             // 类型：输入，参数输入值固定，即hDevice
                             char* buffer,         // 类型：输入，缓冲区首地址
                             ULONG DataLength_Api, // 类型：输入，有效数据大小，单位字节B，单次最大4MB
                             UCHAR Channel_Api, // 类型：输入，写入FPGA编号，1或2
                             ULONG Data_Type, // 类型：输入，写入指定FPGA的DDR或BRAM编号，0，1代表1，2号DDR，4代表BRAM
                             ULONG& nSend,        // 类型：输出，单次写入长度
                             unsigned int wait_ms // 类型：输入，超时时间，单位ms， 0xFFFFFFFF代表永久等待
);
/*
无返回值。
在线程中循环调用，单次最大传输4MB
*/

/*FPGA开始读取DDR中的内容*/
#if PRODUCT_HPEC
ULONG StartReadFpgaDDR(int hDev,                // 类型：输入，参数输入值固定，即hDevice
                       UCHAR Channel_Api,       // 类型：输入，FPGA编号，1，2
                       UCHAR DDRnumber_Api,     // 类型：输入，DDR编号,0，1代表1，2号DDR，2代表BRAM
                       ULONG DDROffsetAddr_Api, // 类型：输入，DDR或BRAM偏移地址
                       ULONG DDRLength_Api,     // 类型：输入，读DDR长度
                       UCHAR Mode_Api           // 类型：输入，模式，0循环读，1单次读
);
#endif

#if NETWORK_HPEC
ULONG StartReadFpgaDDR(int hDev,                // 类型：输入，参数输入值固定，即hDevice
                       UCHAR Channel_Api,       // 类型：输入，FPGA编号
                       UCHAR DDRnumber_Api,     // 类型：输入，DDR编号,0，1代表1，2号DDR，2代表BRAM
                       ULONG DDROffsetAddr_Api, // 类型：输入，DDR或BRAM偏移地址
                       ULONG DDRLength_Api      // 类型：输入，读DDR长度
);
#endif
/*
函数返回值：
STARTREADDDRSUCCESS				开始读取DDR指令发送成功
STARTREADDDRFAILED				开始读取DDR指令发送失败
PowerPC返回值
STARTREADDDR					开始读取DDR
*/

/*FPGA停止读取DDR中的内容*/
ULONG StopReadFpgaDDR(int hDev,           // 类型：输入，参数输入值固定，即hDevice
                      UCHAR Channel_Api,  // 类型：输入，FPGA编号
                      UCHAR DDRnumber_Api // 类型：输入，DDR编号
);
/*
函数返回值：
STOPREADDDRSUCCESS            停止读取DDR指令发送成功
STOPREADDDRFAILED             停止读取DDR指令发送失败
PowerPC返回值
STOPREADDDR					  停止读取DDR
*/

/*块下传参数*/
ULONG BlockWriteReg(int hDev, ULONG Channel_Api, char* buffer, ULONG Length_Api);
/*
输入参数
Channel_Api：	通道号，1/2
buffer：		一个1M大小的申请好的buffer
Length_Api：	写入数据长度，单位：4B(非4B对齐会返回BLOCKWRITEFAILED)，总长不超过1MB

函数返回值
BLOCKWRITESUCCESS
BLOCKWRITEFAILED
RECONFIGING

PowerPC返回值
BLOCK_WRITE_SUCCESS
BLOCK_WRITE_FAILED
*/

#if PRODUCT_HPEC
/*块读寄存器*/
ULONG BlockReadReg(int hDev, ULONG& Channel_Api, char* buffer, ULONG& Length_Api);
/*
输入参数
Channel_Api：	返回通道号，1/2
buffer：		一个1M大小的申请好的buffer，用于存放返回数据。注意这个buffer必须大小正好1MB
Length_Api：	读出数据有效长度

函数返回值
BLOCKREADSUCCESS
BLOCKREADFAILED
RECONFIGING

PowerPC返回值
无返回值
*/

/*SPI通道宏定义*/
#define CHANNEL1_X1 1
#define CHANNEL2_X2 2
#define CHANNEL3_X3 3
#define CHANNEL4_X4 4
#define CHANNEL5_X5 5
#define CHANNEL6_X6 6
#define CHANNEL7_S2 7
#define CHANNEL8_S3 8

/*SPI写数据*/
ULONG WriteSpiData(int hDev, ULONG Channel_Api, char* buffer, ULONG Length_Api);
/*
输入参数
Channel_Api：	SPI通道号，X1~X6,S2~S3,详见SPI通道宏定义
buffer：		需要写给硬件的数据的缓冲区起始地址
Length_Api：	缓冲区数据长度(字节)，必须为4的整数倍

函数返回值
WRITESPIDATASUCCESS
WRITESPIDATAFAILED
RECONFIGING
TIME_OUT_FUNC

PowerPC返回值
SPI_WRITE_DATA_SUCCESS
SPI_WRITE_DATA_FAIL
*/

/*SPI读数据*/
ULONG ReadSpiData(int hDev, UCHAR Channel_Api, char* buffer, ULONG Length_Api);
/*
输入参数
Channel_Api：	SPI通道号，X1~X6,S2~S3,详见SPI通道宏定义
buffer：		一个申请好的buffer，用于存放返回数据，申请缓冲区大小需要大于等于Length_Api
Length_Api：	需要获取的数据长度(字节)，必须为4的整数倍

函数返回值
READSPIDATASUCCESS
READSPIDATAFAILED
RECONFIGING
TIME_OUT_FUNC

PowerPC返回值
SPI_READ_DATA_SUCCESS
SPI_READ_DATA_FAIL
*/
#endif

/*文件系统部分*************************************************************************************************************************************/
/*创建文件*/
ULONG CreateFileInStorage(
    int hDev, // 类型：输入，参数输入值固定，即hDevice
    char*
        FileName_Api, // 类型：输入，文件名（最长31字节），输入一个32字节数组（除了文件名外，其它空余位请置0，最后一个字节必须为0，详见示例）
    ULONGLONG FileSize_Api, // 类型：输入，创建文件的大小，单位Byte，（文件大小必需4MB对齐）
    ULONG fNumber_Api,      // 类型：输入，数据来源于第几片FPGA，1或者2
    ULONG fChannel_Api,     // 类型：输入，数据来源于FPGA的几号通道，输入0~8
    ULONG WaitTime // 类型：输入，等待PowerPC返回时间（单位：ms），如果<=1000ms,则自动设定1000ms
);
/*函数返回值
CREATEFILECTRLSUCCESS				创建文件命令发送成功
SIZEERROR							文件大小错误
FPGANUMBERERROR						115号错误
FPGACHANNELERROR					115通道错误
TIME_OUT_FUNC                       超时
RECONFIGING							正在重构

PowerPC返回值
CREATE_DIRECTORY_SUCEESS			创建文件目录成功
FILENAME_EXISTING					文件名已经存在
SPACE_NO_ENOUGH						空间不足，无法创建文件
CREATE_DIRECTORY_FAILED				创建文件目录失败
CREATE_FILE_SUCCESS					新建文件成功
FILE_NUMBER_OUT						文件数量超过限制
CHANNEL_DAMAGED						底层链路出错，崩溃
READ_OR_WRITE_FILE_ERROR			读/写错误
*/

/*示例：
char *name="hlb"
char FileNmae[32];
memset(FileNmae,0,32);
strcpy(FileName,name);
CreateFileInStorage(hDevice,FileName,4096,1,0);
*/

/*停止填充文件（终止正在新建的文件）*/
ULONG StopFillFile(
    int hDev, // 类型：输入，参数输入值固定，即hDevice
    char*
        FileName_Api, // 类型：输入，文件名（最长31字节），输入一个32字节数组（除了文件名外，其它空余位请置0，最后一个字节必须为0）
    ULONG WaitTime // 类型：输入，等待PowerPC返回时间（单位：ms），如果<=1000ms,则自动设定1000ms
);
/*函数返回值
TIME_OUT_FUNC                       超时
RECONFIGING							正在重构
STOP_FILL_FILE_SUCCESS              状态正确

PowerPC返回值
STOP_CREATE_FILE_SUCCESS			停止填充文件成功
STOP_CREATE_FILE_FAILED				停止填充文件失败
CREATE_FILE_SUCCESS					新建文件成功
*/
/*发送获取目录命令（备用，无需调用）*/
/*void GetDirectoryCtrl(
int hDev								//类型：输入，参数输入值固定，即hDevice
);*/
/*PowerPC返回值
GET_DIRECTORY_SUCCESS				获取目录成功
GET_DIRETORY_FAILED					获取目录失败
*/

/*获取目录（收到PowerPC返回获取目录成功后调用）*/
ULONG GetDirctoryFromStorage(int hDev, // 类型：输入，参数输入值固定，即hDevice
                             _DIRECTORY& buffer // 类型：输入，存储目录的地址，目录结构请详见_DIRECTORY结构
);
/*函数返回
TIME_OUT_FUNC                       获取目录超时
RECONFIGING                         正在重构
GET_DIRETORY_CODE_ERROR				目录乱码
GETDIRSUCCESS                       获取目录成功

PowerPC返回值
GET_DIRECTORY_SUCCESS				获取目录成功
GET_DIRETORY_FAILED					获取目录失败
*/

/*下载文件（请先开启相应通道再开始下载文件）*/
ULONG StartFileTransfer(int hDev, char* FileName_Api, char* buf, ULONGLONG& nRead, ULONGLONG FileOffset_Api,
                        ULONGLONG FileGetSize_Api, ULONG WaitTime);
/*函数返回值
FILETRANSFERCTRLSUCCESS				文件传输命令发送成功
CHANNELERROR						当前传输通道错误
SIZEERROR							文件大小错误
CHANNLECLOSED						通道未开启
TIME_OUT_FUNC                       超时

PowerPC返回值
NO_FIND_FILE						找不到该文件
FILE_BUSY							文件正在读或者写，请等待(被占用)
FILE_SIZE_OUT						文件大小超出范围（写或者读的指定长度超过文件长度）
READ_OR_WRITE_FILE_ERROR			读/写错误
FILE_START_TRANSFER_SUCCESS			开始下载文件
FILE_STRAT_TRANSFER_FAILED			文件下载失败
*/

/*结束下载文件（文件中途结束下载或者已经下载对应大小的文件都需调用这个函数来结束文件下载）*/
ULONG StopFileTransfer(
    int hDev, // 类型：输入，参数输入值固定，即hDevice
    char*
        FileName_Api, // 类型：输入，文件名（最长31字节），输入一个32字节数组（除了文件名外，其它空余位请置0，最后一个字节必须为0）
    ULONG WaitTime // 类型：输入，等待PowerPC返回时间（单位：ms），如果<=1000ms,则自动设定1000ms
);
/*函数返回值
STOPFILETRANSFERCTRLSUCCESS			文件结束传输命令发送成功
CHANNELERROR						当前传输通道错误
TIME_OUT_FUNC                       超时

PowerPC返回值
NO_FIND_FILE						找不到该文件
READ_OR_WRITE_FILE_ERROR			读/写错误
FILE_STOP_TRANSFER_SUCCESS			结束下载文件
*/

/*建立通道(未使用)*/
ULONG OpenChannelForTransfer(
    int hDev,              // 类型：输入，参数输入值固定，即hDevice
    ULONG FileChannel_Api, // 类型：输入，通道号，取值3~8
    ULONG Sample_Api,      // 类型：输入，通道采样量,固定输入SAMPLE_SIZE_4MB（0x1000KB）
    ULONG WaitTime // 类型：输入，等待PowerPC返回时间（单位：ms），如果<=1000ms,则自动设定1000ms
);
/*函数返回值
CHANNELERROR						当前传输通道错误
CHANNELBUSY							当前传输通道已经开启
SAMPLEERROR							采样量错误
OPENCHANNELSUCCESS					开启通道传输命令发送成功
TIME_OUT_FUNC                       超时
RECONFIGING							正在重构

PowerPC返回值
OPEN_CHANNEL_SUCCESS				建立通道成功
OPEN_CHANNEL_FAILED					建立通道失败
*/

/*关闭通道(未使用)*/
ULONG CloseChannelForTransfer(
    int hDev,              // 类型：输入，参数输入值固定，即hDevice
    ULONG FileChannel_Api, // 类型：输入，通道号，取值3~8
    ULONG WaitTime // 类型：输入，等待PowerPC返回时间（单位：ms），如果<=1000ms,则自动设定1000ms
);
/*函数返回值
CHANNELERROR						当前传输通道错误
CLOSECHANNELSUCCESS					关闭通道传输命令发送成功
TIME_OUT_FUNC                       超时
RECONFIGING							正在重构

PowerPC返回值
CLOSE_CHANNEL_SUCCESS				通道关闭
*/

/*删除文件*/
ULONG DeleteFileHpec(
    int hDev, // 类型：输入，参数输入值固定，即hDevice
    char*
        FileName_Api, // 类型：输入，文件名（最长31字节），输入一个32字节数组（除了文件名外，其它空余位请置0，最后一个字节必须为0）
    ULONG WaitTime // 类型：输入，等待PowerPC返回时间（单位：ms），如果<=1000ms,则自动设定1000ms
);
/*函数返回值
TIME_OUT_FUNC                       超时
RECONFIGING							正在重构
DELETEFILESUCCESS                   状态正常

PowerPC返回值
DELETE_FILE_SUCCESS					删除文件成功
DELETE_FILE_FAILED					删除文件失败
*/

/*格式化*/
ULONG FormatHardDisk(int hDev, // 类型：输入，参数输入值固定，即hDevice
                     ULONG WaitTime // 类型：输入，等待PowerPC返回时间（单位：ms），如果<=1000ms,则自动设定1000ms
);
/*函数返回值
TIME_OUT_FUNC                       超时
RECONFIGING							正在重构
FORMATSUCCESS                       状态正常

PowerPC返回值
FORMAT_SUCCESS						格式化成功
FORMAT_FAILED						格式化失败
*/

ULONG WriteRegToFpgaPri(int hDev, UCHAR Channel_Api, ULONG RegisterNumber_Api, ULONG RegisterData_Api);

ULONG ReadRegFromFpgaPri(int hDev, UCHAR Channel_Api, ULONG RegisterNumber_Api, ULONG& RegisterData_Api);

#endif