/**
 * @file scanner_processor.h
 * @brief 定义扫描仪输入处理函数
 * @author JadeSprings
 */

#pragma once

#include <QObject>
#include <windows.h>
#include <N-ScanHub.h>

/**
 * @class ScannerProcessor
 * @brief 该类用于处理与扫描仪相关的功能
 * 
 * 通过开启串口与扫描仪通讯，解码获取扫描仪扫描的结果
 */
class ScannerProcessor : public QObject {
	Q_OBJECT
public:
	explicit ScannerProcessor(QObject* parent = nullptr);
	~ScannerProcessor();

	/**
	 * @brief 初始化扫描仪设备
	 * @return 初始化状态 
	 */
	bool initialize();

	/**
	 * @brief 释放扫描仪设备
	 */
	void release();

	HANDLEDEVLST m_hDeviceList;					/*设备句柄列表*/
	HANDLEDEV m_hDevice;						/*设备句柄*/
	int m_deviceCount;							/*设备列表数量*/
	bool m_isInitialized;

signals:
	/**
	 * @brief 发出错误信号
	 */
	void error_occured(const QString& error);

	/**
	 * @brief 信号：接收到扫描仪解码的数据时发出
	 * @param[in] data 扫描仪解码的数据 
	 */
	void decodeDataReceived(const std::string& PID);

	/**
	 * @brief 信号：扫描仪连接时发出
	 */
	void deviceConnected();

	/**
	 * @brief 信号：扫描仪断开连接时发出
	 */
	void deviceDisconnected();



private:
	/**
	 * @brief 回调函数，处理扫描仪解码的数据
	 * @param[in] hDevice 扫描仪句柄
	 * @param[in] buf 解码数据首地址
	 * @param[in] len 解码数据长度
	 */
	static void __stdcall ReadCallback(const HANDLEDEV hDevice, const char* buf, int len);

	/**
	 * @brief 回调函数，处理扫描仪状态改变事件
	 * @param hDevice 扫描仪句柄
	 * @param isDevExisted 扫描仪设备存在状态
	 */
	static void __stdcall DevStatChangeCallback(const HANDLEDEV hDevice, bool isDevExisted);

	static ScannerProcessor* s_instance;		/*用于回调函数访问实例，始终指向最后一个创建的ScannerProcessor类对象*/
};