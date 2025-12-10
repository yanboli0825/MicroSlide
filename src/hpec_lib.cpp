#include "hpec_lib.h"

#include "hl_lib.h"

#include <tchar.h>
#pragma comment(lib, "hl_lib.lib")

int PowerPCExitCheck()
{
    return dev_exist_chk();
}

int InitApp(int bus_num)
{
    return init_app(bus_num);
}

void CloseApp(int hDev)
{
    return close_app(hDev);
}

int OpenDevice(int bus_num)
{
    return open_dev(bus_num);
}

void CloseDevice(int hDev)
{
    return close_dev(hDev);
}

void InitDevice(int hDev)
{
    init_dev(hDev);
    return;
}

void FileReadPciStatus(int hDev, ULONG& PciStatus_Api, ULONG& Channel_Api, char* FileName_Api)
{
    return;
}

ULONG WriteRegTo19eg(int hDev, ULONG RegisterOffset_Api, ULONG RegisterData_Api)
{
    return write_19eg_reg(hDev, RegisterOffset_Api, RegisterData_Api);
    // return TIME_OUT_FUNC;
}

ULONG ReadRegFrom19eg(int hDev, ULONG RegisterOffset_Api, ULONG& RegisterData_Api)
{
    return read_19eg_reg(hDev, RegisterOffset_Api, (unsigned int&)RegisterData_Api);
    // return TIME_OUT_FUNC;
}

ULONG WriteRegToFpgaPri(int hDev, UCHAR Channel_Api, ULONG RegisterNumber_Api, ULONG RegisterData_Api)
{
    return write_fpga_reg_priv(hDev, Channel_Api - 1, RegisterNumber_Api, RegisterData_Api);
}

ULONG ReadRegFromFpgaPri(int hDev, UCHAR Channel_Api, ULONG RegisterNumber_Api, ULONG& RegisterData_Api)
{
    return read_fpga_reg_priv(hDev, Channel_Api - 1, RegisterNumber_Api, (unsigned int&)RegisterData_Api);
}

ULONG WriteRegToFpga(int hDev, UCHAR Channel_Api, ULONG RegisterNumber_Api, ULONG RegisterData_Api)
{
    return write_fpga_reg(hDev, Channel_Api - 1, RegisterNumber_Api, RegisterData_Api, REG_GROUP_1);
}

ULONG ReadRegCtrlToFpga(int hDev, UCHAR Channel_Api, ULONG RegisterNumber_Api)
{
    ULONG reg_data = 0;
    ULONG status = 0;
    status = ReadRegFromFpga(hDev, Channel_Api, RegisterNumber_Api, reg_data);
    if (status == READREGSUCCESS)
        return READREGCTRLSUCCESS;
    else
        return READREGCTRLFAILED;
}

ULONG ReadRegFromFpga(int hDev, UCHAR Channel_Api, ULONG RegisterNumber_Api, ULONG& RegisterData_Api)
{
    return read_fpga_reg(hDev, Channel_Api - 1, RegisterNumber_Api, (unsigned int&)RegisterData_Api, REG_GROUP_1);
}

ULONG ReConfigureFpga(int hDev, UCHAR Channel_Api, int BinNumber1, int BinNumber2)
{
    // return reconfig(hDev, Channel_Api - 1);
    return TIME_OUT_FUNC;
}

ULONG ReConfigCtrlWithBin(int hDev, UCHAR Channel_Api, char* buffer, unsigned int size)
{
    return reconfig_with_bin(hDev, Channel_Api - 1, buffer, size);
}

ULONG WriteBinToFlash(int hDev, ULONG BinNumber_Api, ULONG BinLength_Api, char* buffer)
{
    return TIME_OUT_FUNC;
}

ULONG GetTestInformationCtrl(int hDev)
{
    return 0;
}

void GetTestInformation(int hDev, char* InforBuffer_Api, ULONG BufferSize_Api)
{
    return;
}

void Reset19EG(int hDev)
{
    reset_dev(hDev);
    return;
}

void ResetAppAndDriver(int hDev)
{
    return;
}

ULONG StratTransferDataFromFpga(int hDev, UCHAR Channel_Api, ULONG Sample_Api, ULONG Mode_Api, ULONG Length_Api)
{
    return start_transfer_data_from_fpga(hDev, Channel_Api - 1);
}

ULONG StopTransferDatafromFpga(int hDev, UCHAR Channel_Api)
{
    return stop_transfer_data_from_fpga(hDev, Channel_Api - 1);
}

unsigned int ReadDataFromFpga(int hDev, char* buffer, UCHAR& Channel_Api, ULONG& Data_Type, ULONG& nGet,
                              unsigned int wait_ms)
{
    unsigned int ret = read_data_from_fpga(hDev, buffer, (unsigned int&)Channel_Api, (unsigned int&)Data_Type,
                                           (unsigned int&)nGet, wait_ms);

    if (ret == RW_DATA_OK)
    {
        Channel_Api = Channel_Api + 1;
    }
    return ret;
}

ULONG ChangeSampleSize(int hDev, ULONG Channel_Api, ULONG Sample_Api)
{
    return TIME_OUT_FUNC;
}

void InitTransfer(int hDev)
{
    return;
}

ULONG WriteToPowerPCMemory(int hDev, char* buffer, ULONG MemoryOffsetToPowerPC, ULONG BufferLengthToPowerPC)
{
    return TIME_OUT_FUNC;
}

ULONG WriteToFpga(int hDev, char* buffer, ULONG Channel_Api)
{
    return TIME_OUT_FUNC;
}

ULONG SendCtrlToPPC(int hDev, ULONG Ctrl_Api)
{
    return TIME_OUT_FUNC;
}

ULONG StartTransferDataToFpga(int hDev, ULONG DDRBaseAddr_Api)
{
    return STARTWRITETRANSFERCTRLSUCCESS;
}

ULONG StopTransferDataToFpga(int hDev)
{
    return STOPWRITETRANSFERCTRLSUCCESS;
}

unsigned int WriteDataToFpga(int hDev, char* buffer, ULONG DataLength_Api, UCHAR Channel_Api, ULONG Data_Type,
                             ULONG& nSend, unsigned int wait_ms)
{
    return write_data_to_fpga(hDev, buffer, DataLength_Api, Channel_Api - 1, Data_Type, (unsigned int&)nSend, wait_ms);
}

#if PRODUCT_HPEC
ULONG StartReadFpgaDDR(int hDev, UCHAR Channel_Api, UCHAR DDRnumber_Api, ULONG DDROffsetAddr_Api, ULONG DDRLength_Api,
                       UCHAR Mode_Api)
{
    return TIME_OUT_FUNC;
}
#endif

#if NETWORK_HPEC
ULONG StartReadFpgaDDR(int hDev, UCHAR Channel_Api, UCHAR DDRnumber_Api, ULONG DDROffsetAddr_Api, ULONG DDRLength_Api)
{
    return TIME_OUT_FUNC;
}
#endif

ULONG StopReadFpgaDDR(int hDev, UCHAR Channel_Api, UCHAR DDRnumber_Api)
{
    return TIME_OUT_FUNC;
}

ULONG BlockWriteReg(int hDev, ULONG Channel_Api, char* buffer, ULONG Length_Api)
{
    return TIME_OUT_FUNC;
}

#if PRODUCT_HPEC
ULONG BlockReadReg(int hDev, ULONG& Channel_Api, char* buffer, ULONG& Length_Api)
{
    return TIME_OUT_FUNC;
}

ULONG WriteSpiData(int hDev, ULONG Channel_Api, char* buffer, ULONG Length_Api)
{
    return TIME_OUT_FUNC;
}

ULONG ReadSpiData(int hDev, UCHAR Channel_Api, char* buffer, ULONG Length_Api)
{
    return TIME_OUT_FUNC;
}
#endif

ULONG CreateFileInStorage(int hDev, char* FileName_Api, ULONGLONG FileSize_Api, ULONG fNumber_Api, ULONG fChannel_Api,
                          ULONG WaitTime)
{
    return TIME_OUT_FUNC;
}

ULONG StopFillFile(int hDev, char* FileName_Api, ULONG WaitTime)
{
    return TIME_OUT_FUNC;
}

ULONG GetDirctoryFromStorage(int hDev, _DIRECTORY& buffer)
{
    return TIME_OUT_FUNC;
}

ULONG StartFileTransfer(int hDev, char* FileName_Api, char* buf, ULONGLONG& nRead, ULONGLONG FileOffset_Api,
                        ULONGLONG FileGetSize_Api, ULONG WaitTime)
{
    return TIME_OUT_FUNC;
}

ULONG StopFileTransfer(int hDev, char* FileName_Api, ULONG WaitTime)
{
    return TIME_OUT_FUNC;
}

ULONG OpenChannelForTransfer(int hDev, ULONG FileChannel_Api, ULONG Sample_Api, ULONG WaitTime)
{
    return TIME_OUT_FUNC;
}

ULONG CloseChannelForTransfer(int hDev, ULONG FileChannel_Api, ULONG WaitTime)
{
    return TIME_OUT_FUNC;
}

ULONG DeleteFileHpec(int hDev, char* FileName_Api, ULONG WaitTime)
{
    return TIME_OUT_FUNC;
}

ULONG FormatHardDisk(int hDev, ULONG WaitTime)
{
    return TIME_OUT_FUNC;
}
