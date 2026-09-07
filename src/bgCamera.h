#pragma once
#include "DBInterfaceDLL.h"
#include "bgimaging.h"

#include <QImage>
#include <QString>
#include <QVideoWidget>
#include <atomic>
#include <memory>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <qmetatype.h>
#include <thread>

/*向FTP数据库传输处理结果*/
#define DB_SEND

/*图片筛选相关*/
#define SIMILAR_FRAME_COUNT 5
#define MAX_STATIC_VIEW_IMAGES 2
#define SIMILARITY_RESET_VALUE 1024

/*AI处理线程和Buffer*/
#define DEAL_THREAD_MAX_NUMS 3      // AI处理线程数
#define SIZE_BUFFER 8 * 1024 * 1024 // Buffer大小，单位为bits

/*倍率检测相机部分*/
#define MAGNIFICATION_CAMERA_ID "1080P USB Camera" // 倍率检测相机的ID

/*FPGA读写相关*/
#define FPGA_CHANNEL 1 // 读写FPGA的第几个通道，默认是第一个

/*显微镜相机部分*/
#define BG_TEMP 5760        ///< 白平衡参数：色温
#define BG_TINT 1052        ///< 白平衡参数：色调
#define DEFUALT_WIDTH 1824  ///< 显微镜捕获的图片的宽度
#define DEFUALT_HEIGHT 1216 ///< 显微镜捕获的图片的高度

class CImgPool;
class QMediaCaptureSession;
class QVideoSink;
class QCamera;
class QVideoFrame;
class QPushButton;
class QSlider;
class QLabel;
class QThread;
class QTimer;
class QCheckBox;
class QVBoxLayout;
class TMagnifyDetect;
class TMagDetImage;
typedef struct pis_response PIS_RES;
struct BestImage;
struct AIResult;

/**
 * @brief 这个类用来实现与显微镜视频流数据处理相关的功能
 *
 */
class bgCamera : public QWidget
{
    Q_OBJECT

public:
    // ================================ 构造/析构函数 ================================

    bgCamera(QWidget* parent = nullptr);

    ~bgCamera();

    // ================================ 公共成员函数 ================================

    /**
     * @brief 设置当前PowerPC设备的总线号
     *
     * @param[in] busNum PC设备号
     */
    void setBusNum(const int busNum);

    /**
     * @brief 创建线程池
     *
     * @param[in] deal_cnt 线程数量
     */
    void initThreadPool(const unsigned int deal_cnt);

    /**
     * @brief 释放线程池
     */
    void releaseThreadPool();

    /**
     * @brief 处理单张图片
     *
     * @param img
     */
    void handleSingleImage(QImage img);

    // ================================ 公共成员变量 ================================
    static const std::map<std::string, std::vector<std::string>> slicePartToClassNamesMap; ///< 部位与对应类型疾病map表

    // 临时调试变量 -------------
    char m_tempCounter = 0;
    // -------------------------

    /*PowerPC设备相关*/
    int m_busNum;  ///< PowerPC设备总线号
    int m_hDevice; ///< PowerPC设备句柄

    /*FPGA相关*/
    int m_imgNumToFPGA;     ///< 向FPGA传输的总图片数
    unsigned int m_FPGADNA; ///< FPGA的DNA编号
    bool m_isTrans;         ///< 设备是否正在传输视频至FPGA。false:未传输，true:正在传输

    /*显微镜设备部分*/
    BgcamDeviceV2 m_microDevice; ///< 显微镜设备实例
    HBgcam m_hMicro;             ///< 显微镜句柄。该句柄仅在点击打开显微镜按钮后有效，关闭后失效
    QTimer* m_fpsTimer;          ///< 计算显微镜FPS的定时器
    int m_temp;                  ///< 色温
    int m_tint;                  ///< 色调
    unsigned m_imgWidth;         ///< 图像宽度
    unsigned m_imgHeight;        ///< 图像高度
    uchar* m_pData;              ///< 图像帧数据缓冲区
    bool m_saveImg;              ///< 是否保存显微镜视频流图片

    /*数据库相关部分*/
    std::atomic_flag warning_shown = ATOMIC_FLAG_INIT; ///< 原子标识，用于限制SOCKET初始化失败时警告只弹出一次
    std::unique_ptr<PIS_RES> m_slideInfo;              ///< 从PIS获取的切片相关信息
    std::string m_dbAddr;                              ///< 数据库IP
    int m_dbPort;                                      ///< 数据库端口
    json m_imagesMetrics;                              ///< 以json格式保存一张切片对应的所有图片的手工特征及AI诊断等信息

    /*显微镜视频流预处理相关*/
    BestImage* m_validImg;
    int m_similarImgNum = 0;       ///< 保存相似的图片的数目，每5张图片输出一次，或移动输出
    int m_staticViewImgNum = 0;    ///< 保存显微镜镜头静止时，给出的图片的最大数量。最大值为MAX_STATIC_VIEW_IMAGES的值
    int m_validImgNum = 0;         ///< 保存输出的图片的数量
    unsigned int m_lastSimilarity; ///< 保存上一张图片的相似度（初始设置为1024）
    int m_sharpThres;              ///< 清晰度筛选阈值
    int m_similarityThres;         ///< 相似度筛选阈值
    int m_areaThres;               ///< 区块面积筛选阈值

    /*AI推理相关*/
    std::vector<float> m_video_features; ///< 视频特征
    std::vector<float> m_mmr_features;   ///< MMR特征，只记录10、20、40倍率图片的特征。仅在视频诊断结果为C时使用

    std::thread m_aiThreadPool[DEAL_THREAD_MAX_NUMS]; ///< AI处理线程池

    /*配置文件变量*/
    QString m_saveRoot; ///< 保存图片的根目录路径
    QString m_saveDir;  ///< 保存图片的文件夹路径，命名与当前病理号相关

    CImgPool* m_img_pool;

    /*线程池相关*/
    bool m_dealFlag = 0;              ///< 控制AI处理线程的运行标志，false表示停止，true表示运行
    std::atomic<int> m_numFreeThread; ///< 空闲AI处理线程数量

    std::pair<std::string, std::vector<float>> m_video_res = {".........",
                                                              {0, 0, 0, 0, 0, 0}}; ///< 保存视频级的诊断结果

    // ================================ 公共槽/信号函数 ================================
public slots:
    /**
     * @brief 点击按钮（开始传输/停止传输）后的槽函数
     *
     * @return 函数执行状态码：
     *         - true：成功
     *         - false：未连接FPGA
     */
    bool onBtnTransClicked();

    /**
     * @brief 倍率检测相机矩形框位置变化的槽函数
     *
     * @param _rectX 矩形框X坐标
     * @param _rectY 矩形框Y坐标
     */
    void slot_cameraRectValueChanged(int _rectX, int _rectY);
    /**
     * @brief 处理测试文件图片的槽函数（逐张处理图片）
     *
     * @param img 测试图片
     */
    void slot_handleFileTest(QImage img);

    /**
     * @brief 倍率检测复选框状态变化的槽函数
     *
     * @param arg1
     */
    void slot_cbox_magDetectStateChanged(const Qt::CheckState& arg1);

signals:
    /**
     * @brief 显示数据库连接失败警告
     *
     */
    void signal_showDBDisconnectWarning();

    /**
     * @brief 显示AI处理后的图片
     *
     * @param img BestImage结构体，包含图片及相关信息
     * @param img_inference_res 输入AI处理结果
     * @param _video_res 视频诊断结果
     */
    void signal_show_image(std::shared_ptr<BestImage> img, std::shared_ptr<AIResult> img_inference_res,
                           QString _video_res);

    /**
     * @brief 显示预览图片
     *
     * @param previewImg 预览图片
     * @param bestImgNum 预览图片帧号
     */
    void signal_showPreviewImg(QPixmap previewImg, int bestImgNum);

    /**
     * @brief 将预览图的Loading信息改为invalid
     *
     * @param imgNum
     */
    void signal_changePreviewImgToInvalid(unsigned int imgNum);

protected:
    // ================================ 保护成员函数 ================================

    void closeEvent(QCloseEvent*) override;

    // ================================ 保护槽/信号函数 ================================
signals:
    /**
     * @brief 显微镜事件信号函数
     *
     * @param nEvent 事件类型
     */
    void evtCallback(unsigned nEvent);

private:
    // ================================ 私有成员函数 ================================
    /**
     * @brief 初始化UI
     */
    void initUI();

    /**
     * @brief 读取配置文件中的设置项
     */
    void readIniSettings();

    /**
     * @brief 设置信号与槽的连接
     */
    void setupConnections();

    /**
     * @brief 释放FPGA资源
     *
     */
    void releaseFPGA();

    /**
     * @brief 初始化倍率检测线程
     */
    void initMagDetectThread();

    /**
     * @brief 释放倍率检测线程
     */
    void freeMagDetectThread();

    /**
     * @brief 处理视频流连续帧的回调函数
     *
     */
    void handleVideoStreamEvent();

    /**
     * @brief 处理曝光相关事件
     *
     */
    void handleExpoEvent();

    /**
     * @brief 处理白平衡相关事件
     *
     */
    void handleTempTintEvent();

    /**
     * @brief 初始化显微镜设备，设置图片分辨率、字节序（RGB or BGR）、白平衡使能、锐化等
     *
     */
    void initMicro();

    /**
     * @brief 释放显微镜设备资源
     *
     */
    void releaseMicro();

    /**
     * @brief 从FPGA读取图片清晰度、帧号、相似度、有效区域大小等指标
     *
     * @param _pData 图片数据指针
     * @param rsharp 清晰度
     * @param rframe 发送给FPGA的总图片数
     * @param rsimilarity 相似度
     * @param rarea 有效区域大小
     * @return true
     * @return false
     */
    bool getMetricsFromFPGA(const uchar* _pData, unsigned int& rsharp, unsigned int& rframe, unsigned int& rsimilarity,
                            unsigned int& rarea);

    /**
     * @brief 从视频流中筛选出有效图片
     *
     * @param image 显微镜图片
     * @param frame 从FPGA读取的总图片数
     * @param clear 清晰度
     * @param normClear 归一化清晰度
     * @param similarity 相似度
     * @param area 有效区域大小
     */
    void getValidImage(const QImage& image, const unsigned int frame, const unsigned int clear,
                       const unsigned int normClear, const unsigned int similarity, const unsigned int area);

    /**
     * @brief 显微镜事件回调函数
     *
     * @param nEvent 事件类型
     * @param pCallbackCtx 回调上下文指针（指向bgCamera类实例）
     */
    static void __stdcall eventCallBack(unsigned nEvent, void* pCallbackCtx);

    static QVBoxLayout* makeLayout(QLabel*, QSlider*, QLabel*, QLabel*, QSlider*, QLabel*);
    static QVBoxLayout* makeLayout3(QLabel*, QSlider*, QLabel*, QLabel*, QSlider*, QLabel*, QLabel*, QSlider*, QLabel*);

    // ================================ 私有槽/信号函数 ================================
private slots:

    /**
     * @brief 处理倍率检测相机帧变化的槽函数
     *
     * @param frame 当前的视频帧
     */
    void slot_cameraFrameChanged(const QVideoFrame& frame);

    /**
     * @brief 存储入库复选框状态变化的槽函数
     *
     * @param arg1
     */
    void slot_cbox_saveStateChanged(const Qt::CheckState& arg1);

    /**
     * @brief 点击打开显微镜/关闭显微镜按钮后的槽函数
     *
     */
    void onBtnOpenClicked();

    /**
     * @brief 点击连接FPGA/断开FPGA按钮后的槽函数
     *
     */
    void onBtnConnectClicked();

signals:
    /**
     * @brief 当用户没有确认PID，直接点击开始传输时，emit该信号，默认为点击了确认PID的按钮
     */
    void directly_click_btn_trans();

    // ================================ 私有成员变量 ================================
private:
    /*显微镜倍率检测相关*/
    QCamera* m_camera; ///< 倍率检测相机对象
    QMediaCaptureSession* m_captureSession;
    QVideoSink* m_mySink;
    QThread* m_magDetThread;        ///< 倍率检测线程
    TMagDetImage* m_magDetImage;    ///< 倍率检测图片对象
    TMagnifyDetect* m_magDetWorker; ///< 倍率检测工作对象
    int m_cameraFrameCnt;           ///< 倍率检测相机帧计数器，用于控制倍率检测频率。谨慎调整
    int m_microMagnification;       ///< 2， 4，10，20，40 五种倍率检测结果

    /*界面部分*/
    QCheckBox* m_cbox_auto;
    QCheckBox* m_cbox_save;
    QCheckBox* m_cbox_mag;

    QSlider* m_slider_expoTarget; ///< 滑动条：曝光目标
    QSlider* m_slider_expoTime;   ///< 滑动条：曝光时间
    QSlider* m_slider_expoGain;   ///< 滑动条：曝光增益
    QSlider* m_slider_temp;
    QSlider* m_slider_tint;

    QLabel* m_lbl_expoTarget;
    QLabel* m_lbl_expoTime;
    QLabel* m_lbl_expoGain;
    QLabel* m_lbl_temp;
    QLabel* m_lbl_tint;
    QLabel* m_lbl_video;
    QLabel* m_lbl_fps;
    QLabel* m_lbl_debug;

    QPushButton* m_btn_defaultWB;
    QPushButton* m_btn_autoWB;
    QPushButton* m_btn_open;    ///< 按钮：打开/关闭
    QPushButton* m_btn_connect; ///< 按钮：连接设备/断开设备
    QPushButton* m_btn_trans;   ///< 按钮：开始传输/停止传输
};
