#pragma once

#include <N-ScanHub.h>
#include <QObject>
#include <windows.h>

/**
 * @brief 扫描仪资源管理器
 *
 * 该类用于初始化和释放扫描仪设备，处理扫描仪输入的数据，并通过信号与其他组件通信
 */
class ScannerProcessor : public QObject
{
    Q_OBJECT
public:
    explicit ScannerProcessor(QObject* parent);

    /**
     * @brief 初始化扫描仪设备
     * @return true 初始化成功；false 初始化失败
     */
    static bool initialize();

    /**
     * @brief 释放扫描仪设备
     */
    static void release();

    /**
     * @brief 获取扫描仪处理器的唯一实例
     * @return
     * */
    static ScannerProcessor* get();

signals:

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

    static HANDLEDEVLST m_hDeviceList;                  ///< 设备句柄列表
    static HANDLEDEV m_hDevice;                         ///< 设备句柄
    static int m_deviceCount;                           ///< 设备列表数量
    static std::shared_ptr<ScannerProcessor> m_scanner; ///< 类的唯一实例
    static bool m_isInitialized;
};