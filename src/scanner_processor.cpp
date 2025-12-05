/**
 * @file scanner_processor.cpp
 * @brief ScannerProcessor类的具体实现
 * @author JadeSprings
 */

#include "scanner_processor.h"
#include <iostream>

ScannerProcessor* ScannerProcessor::s_instance = nullptr;

ScannerProcessor::ScannerProcessor(QObject* parent):
	m_hDeviceList(NULL),
	m_hDevice(NULL),
	m_deviceCount(0),
	m_isInitialized(false)
{
	s_instance = this;
}

ScannerProcessor::~ScannerProcessor()
{
	release();
}

bool ScannerProcessor::initialize()
{
	// 枚举设备
	m_hDeviceList = nl_EnumDevices(&m_deviceCount, ENUM_ALL);
	if (m_deviceCount == 0 && m_hDeviceList == NULL) {
		return false;
	}

	// 打开第一个设备
	m_hDevice = nl_OpenDevice(m_hDeviceList, 0);
	if (m_hDevice == NULL) {
		emit error_occured("打开设备失败");
		return false;
	}

	// 设置数据回调
	nl_SetListener(m_hDevice, &ScannerProcessor::ReadCallback);

	// 设置设备状态变化回调
	//nl_SetCbDevStatusChanged(m_hDevice, &ScannerProcessor::DevStatChangeCallback);

	return true;
}

void ScannerProcessor::release()
{
	if (m_hDevice) {
		char* scanner_status = nl_GetLastError();
		std::cout << "sadsd111: " << scanner_status << std::endl;
		nl_StopListener(m_hDevice);
		nl_CloseDevice(&m_hDevice);
		m_hDevice = NULL;
	}

	if (m_hDeviceList) {
		nl_ReleaseDevices(&m_hDeviceList);
		Sleep(1000);
		m_hDeviceList = NULL;
	}
}

void __stdcall ScannerProcessor::ReadCallback(const HANDLEDEV hDevice, const char* buf, int len)
{
	// 检查是否有类实例，数据长度是否有效
	if (s_instance && len > 0) {
		QByteArray data(buf, len);
		std::string PID = data.toStdString();
		emit s_instance->decodeDataReceived(PID);
	}
}

void __stdcall ScannerProcessor::DevStatChangeCallback(const HANDLEDEV hDevice, bool isDevExisted)
{
	if (s_instance) {
		if (isDevExisted) {
			emit s_instance->deviceConnected();
		}
		else {
			emit s_instance->deviceDisconnected();
		}
	}
}
