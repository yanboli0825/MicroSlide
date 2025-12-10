#pragma once
#include <QWidget>
#include <QImage>
#include <opencv2/opencv.hpp>


#define CAMERA_NAME "1080P USB Camera"


struct Color2;
class QCamera;
class QVideoFrame;
class QVideoSink;
class QMediaCaptureSession;
namespace Ui {
    class MagDebug;
}


class MagDebug : public QWidget
{
    Q_OBJECT

public:
    // ================================ 构造/析构函数 ================================
    explicit MagDebug(QWidget *parent = nullptr);
    ~MagDebug();


    // ================================ 公共槽/信号函数 ================================
signals:
    /**
    * @brief 当倍率检测矩形框的位置被修改时发出信号，实时修改
    * 
    * @param[in] _rectX 方框左上角的X坐标
    * @param[in] _rectY 方框左上角的Y坐标
    */
    void signal_rect_value_changed(int _rectX, int _rectY);

private:
    // ================================ 私有成员函数 ================================
    
    /**
     * @brief 展示摄像头视频流图像和倍率检测框
     * 
     * @param[in] image 摄像头视频流图像
     */
    void show_camera_img(QImage image);


    /**
     * @brief 将QImage图像转换为cv::Mat，格式为BGR
     * 
     * @param[in] image 输入图像 
     * 
     * @return cv::Mat，格式为BGR
     */
    cv::Mat qImageToMatBGR(const QImage& image);

    /**
     * @brief 计算输入图像的倍率
     * 
     * @param frame 输入图像，格式为HSV
     * 
     * @return 倍率，格式字符串
     */
    std::string calculateMainColor(const cv::Mat& frame);


    // ================================ 私有成员变量 ================================

    Ui::MagDebug *ui;

    QCamera*                         m_camera;      // 倍率检测相机设备，用于捕获视频流
    QVideoSink*                        m_sink;      // 视频接收器。当有新帧可用时会发出videoFrameChanged信号
    QMediaCaptureSession*    m_captureSession;      // 视频流捕捉会话，用于将摄像头捕获的视频流路由到QVideoSink

    QString                        m_str_hint;      // 提示词

    unsigned                  m_frame_counter;      // 倍率检测相机帧计数器
    char                     m_fps_controller;      // 帧数控制器。若帧数设置的太高则软件会严重卡顿

    int                               m_rectX;      // 倍率检测框左上角的X坐标
    int                               m_rectY;      // 倍率检测框左上角的Y坐标

    int                    m_show_magdet_info;      // 是否打印倍率检测调试信息


    // ================================ 私有槽/信号函数 ================================
private slots:
    /**
     * @brief 有新帧可用时的槽函数
     */
    void slot_on_video_frame_changed(const QVideoFrame &frame);

    /**
     * @brief 点击按钮（修改方框位置）后，检查X，Y坐标范围并将其保存至INI配置文件
     */
    void on_pushButton_clicked();

};

