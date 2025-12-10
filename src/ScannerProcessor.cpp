#include "ScannerProcessor.h"

#include "Logger.h"

#include <iostream>

std::shared_ptr<ScannerProcessor> ScannerProcessor::m_scanner = nullptr;
bool ScannerProcessor::m_isInitialized = false;
HANDLEDEVLST ScannerProcessor::m_hDeviceList = NULL;
HANDLEDEV ScannerProcessor::m_hDevice = NULL;
int ScannerProcessor::m_deviceCount = 0;

ScannerProcessor::ScannerProcessor(QObject* parent)
    : QObject(parent)
{
}

ScannerProcessor* ScannerProcessor::get()
{
    return m_scanner.get();
}

bool ScannerProcessor::initialize()
{
    if (m_isInitialized)
    {
        return true; // 已初始化，直接返回
    }
    // 枚举设备
    m_hDeviceList = nl_EnumDevices(&m_deviceCount, ENUM_ALL);
    if (m_deviceCount == 0 && m_hDeviceList == NULL)
    {
        m_isInitialized = false;
        return false;
    }

    // 打开第一个设备
    m_hDevice = nl_OpenDevice(m_hDeviceList, 0);
    if (m_hDevice == NULL)
    {
        m_isInitialized = false;
        return false;
    }

    // 设置数据回调
    nl_SetListener(m_hDevice, &ScannerProcessor::ReadCallback);

    // 设置设备状态变化回调。接口SDK存在问题，暂不启用
    // nl_SetCbDevStatusChanged(m_hDevice, &ScannerProcessor::DevStatChangeCallback);

    m_isInitialized = true;
    m_scanner = std::make_shared<ScannerProcessor>(nullptr);
    return true;
}

void ScannerProcessor::release()
{
    if (m_isInitialized && m_scanner)
    {
        if (m_hDevice)
        {
            // char* scanner_status = nl_GetLastError();

            nl_StopListener(m_hDevice);
            nl_CloseDevice(&m_hDevice);
            Sleep(1000);
            m_hDevice = NULL;
        }

        if (m_hDeviceList)
        {
            nl_ReleaseDevices(&m_hDeviceList);
            Sleep(1000);
            m_hDeviceList = NULL;
        }
        m_scanner = nullptr;
        m_isInitialized = false;
    }
}

void __stdcall ScannerProcessor::ReadCallback(const HANDLEDEV hDevice, const char* buf, int len)
{
    // 检查是否有类实例，数据长度是否有效
    if (m_scanner && len > 0)
    {
        std::string PID(buf, len);
        emit m_scanner->decodeDataReceived(PID);
    }
}

void __stdcall ScannerProcessor::DevStatChangeCallback(const HANDLEDEV hDevice, bool isDevExisted)
{
    if (m_scanner)
    {
        if (isDevExisted)
        {
            emit m_scanner->deviceConnected();
        }
        else
        {
            emit m_scanner->deviceDisconnected();
        }
    }
}
