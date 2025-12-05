#pragma once
#include "DBInterfaceDLL.h"  
#include <QString>
#include <QVideoWidget>
#include <QImage>
#include <opencv2/opencv.hpp>
#include <thread>
#include <atomic>
#include <qmetatype.h>
#include "bgimaging.h"
#include "nlohmann/json.hpp"


/*向数据库FTP传输处理结果*/
#define FTP_SEND


/*AI处理线程和Buffer*/
#define DEAL_THREAD_MAX_NUMS			     3    // AI处理线程数
#define SIZE_BUFFER                8*1024*1024    // Buffer大小，单位为bits


/*倍率检测相机部分*/
#define MAGNIFICATION_CAMERA_ID     "1080P USB Camera"    // 倍率检测相机的ID


/*FPGA读写相关*/
#define FPGA_CHANNEL        1    // 读写FPGA的第几个通道，默认是第一个


/*显微镜相机部分*/
#define BG_TEMP             5760    // 白平衡参数：色温
#define BG_TINT             1052    // 白平衡参数：色调
#define DEFUALT_WIDTH       1824    // 显微镜捕获的图片的宽度
#define DEFUALT_HEIGHT      1216    // 显微镜捕获的图片的高度


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

namespace bgConverter {
    std::string wchar_to_string_bg(const wchar_t* wstr);
}

/**
 * @brief AI诊断结果结构体
 */
struct AIResult {
    QString diagnosis_res;                  // 疾病诊断结果，例如：“C”
    float diagnosis_prob;                   // 疾病诊断概率，例如：0.918
    QVector<float> cls_prob;                // 疾病分类分布
    cv::Point ROI;                          // ROI坐标

    AIResult() {};
    AIResult(QString res, float prob, QVector<float> distribution, cv::Point coords):
        diagnosis_res(res),
        diagnosis_prob(prob),
        cls_prob(distribution),
        ROI(coords)
    {

    };

    void set_value(QString res, float prob, QVector<float> distribution, cv::Point coords) {
        diagnosis_res = res;
        diagnosis_prob = prob;
        cls_prob = distribution;
        ROI = coords;
    };
};


/**
* @brief 这个类用来实现与显微镜视频流数据处理相关的功能
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
    * @brief 初始化UI
    */
    void initUI();
    
    /**
    * @brief 初始化倍率检测线程
    */
    void initMagDetThread();

    /**
    * @brief 释放倍率检测线程
    */
    void freeMagDetThread();

    /**
    * @brief 将PowerPC的设备号传递给Camera
    * 
    * @param[in] _num PC设备号
    */
    void set_cur_bus_num(int _num);

    /**
    * @brief 设置按钮（开始传输/停止传输）的状态
    * 
    * @param[in] _trans 传输状态
    */
    void set_trans_state(bool _trans);

    /**
    * @brief 创建线程数组
    * 
    * @param[in] deal_cnt 线程数量
    */
    void enable_deal_thd(unsigned int deal_cnt);

    /**
    * @brief 释放线程数组
    */
    void disable_deal_thd();

    /**
    * @brief 将RGB888格式的QImage图像转换为RGB格式的cv::Mat图像
    * 
    * @param[in] qimage 输入图像，格式为RGB888，若不为RGB888则强制转换为RGB888
    * 
    * @return 返回RGB格式的cv::Mat
    */
    static cv::Mat qimageToMatRGB(const QImage& qimage);

    // ================================ 公共成员变量 ================================
    static const std::map<std::string, std::vector<std::string>> classes_map;             // 部位与对应类型疾病map表

    TMagnifyDetect* m_magDetWorkwer;
    bool m_connect;               // 设备是否连接
    bool m_trans;                 // 设备是否正在传输。false:未传输，true:正在传输

    /*显微镜设备部分*/
    BgcamDeviceV2   m_cur;         // 显微镜设备id

    /*数据库相关部分*/
    std::atomic_flag warning_shown = ATOMIC_FLAG_INIT;      // 原子标识，用于限制SOCKET初始化失败时警告只弹出一次
    PIS_RES* m_slide_info;                                  // 从PIS获取的切片相关信息
    std::string m_db_addr = "192.168.11.15";                // 数据库IP
    int m_db_port = 8888;                                   // 数据库端口
    json m_images_features;                                 // 以json格式保存一张切片对应的所有图片的手工特征及AI诊断等信息


    /*筛选BEST图片部分*/
    BestImage* m_best_image;
    int m_similar_img_num = 0;                  // 保存相似的图片的数目，每5张图片输出一次，或移动输出。与m_same_best_img_num联合使用
    int m_same_best_img_num = 0;                // 保存显微镜镜头静止时，给出的图片的最大数量。最大值为_max_num的值
    int m_best_img_num = 0;                     // 保存输出的图片的数量
    unsigned int m_last_similarity = 1024;      // 保存上一张图片的相似度（初始设置为1024）

    int m_sharp = 0;            // 清晰度筛选阈值
    int m_similarity = 12;      // 相似度筛选阈值
    int m_area;                 // 区块面积筛选阈值


    /*AI推理部分*/
    std::vector<float> m_video_features;                      // 视频特征
    std::vector<float> m_mmr_features;                        // MMR特征，只记录10、20、40倍率图片的特征。仅在视频诊断结果为C时使用
    char m_free_thread_num;                                   // 空闲AI处理线程数量
    std::thread m_deal_thread[DEAL_THREAD_MAX_NUMS];          // AI处理线程池
    //OrtComponents* m_cur_AI;                                // 当前AI模型


    /*配置文件变量*/
    int m_show_fpga_debug_info = 0;                 // 打印FPGA的调试信息，1表示打印
    int m_save_all_images = 0;                      // 是否保存所有图片，1表示保存
    int m_show_save_image_debug_info = 0;           // 是否打印图片保存时的调试信息，1表示打印
    int m_inference_video_result = 0;               // 是否推理视频级诊断结果，1表示推理；参数从config.ini配置文件中读取
    unsigned int m_FPGADNA = 0;

    //子线程
    //病人文件夹名，由子线程传过来

    QString m_save_dir;         // 保存图片的文件夹路径
    QString m_save_root;        // 图片保存根目录

    CImgPool* m_img_pool;

    unsigned int deal_flag = 0;
    unsigned int deal_thd_cnt = 1;
    int m_hDevice = -1;                   //连接成功后返回的设备号
    std::pair<std::string, std::vector<float>> m_video_res = { "........." ,{0,0,0,0,0,0} };       //保存视频级的诊断结果


    // ================================ 公共槽/信号函数 ================================
public slots:
    /*
    * @brief 点击按钮（开始传输/停止传输）后的槽函数
    * 
    * @return 函数执行状态码：
    *         - 1：成功
    *         - -1：未连接FPGA
    */
    int onBtnTrans();

    void slot_on_rect_value_changed(int _rectX, int _rectY);
    void slot_on_handle_file_test(QString file_path);
    void on_m_cbox_magStateChanged(const Qt::CheckState& arg1);

signals:
    void signal_show_db_connect_warning();


protected:
    // ================================ 保护成员函数 ================================

    void closeEvent(QCloseEvent*) override;

    // ================================ 保护槽/信号函数 ================================
signals:
    void evtCallback(unsigned nEvent);

    /**
     * @brief 显示AI处理后的图片
     * 
     * @param img 输入BestImage结构体，包含图片及相关信息
     * @param img_inference_res 输入AI处理结果
     * @param _video_res 视频诊断结果
     */
    void signal_show_image(const BestImage* img, const AIResult* img_inference_res, QString _video_res);     // 传递图片保存的
    void signal_show_preview_img(QPixmap _preview_img, int _best_img_num);
    void signal_delete_preview_img(int m_best_img_num);
    void signal_show_invalid_img(unsigned int m_best_img_frame);


private:
    // ================================ 私有成员函数 ================================

    void onBtnOpen();
    void onBtnConnect();
    void on_m_cbox_saveStateChanged(const Qt::CheckState& arg1);
    void handleImageEvent();
    void handleExpoEvent();
    void handleTempTintEvent();
    void handleStillImageEvent();
    void openCamera();
    void closeCamera();
    void startCamera();
    static void __stdcall eventCallBack(unsigned nEvent, void* pCallbackCtx);
    static QVBoxLayout* makeLayout(QLabel*, QSlider*, QLabel*, QLabel*, QSlider*, QLabel*);
    static QVBoxLayout* makeLayout3(QLabel*, QSlider*, QLabel*, QLabel*, QSlider*, QLabel*, QLabel*, QSlider*, QLabel*);

    // ================================ 私有成员变量 ================================

    /*显微镜设备部分*/
    //BgcamDeviceV2   m_cur;         // 显微镜设备id
    HBgcam          m_hcam;        // 显微镜句柄

    unsigned        m_imgWidth;    // 图像宽度
    unsigned        m_imgHeight;   // 图像高度
    uchar*          m_pData;       // 图像帧数据

    int             m_temp;        // 色温
    int             m_tint;        // 色调
    unsigned        m_count;

    bool            m_save_img;    // 是否保存显微镜视频流图片
    double          m_frame;       // 显微镜视频流帧率


    /*PC设备部分*/
    int m_cur_bus_num;    // PC设备总线id


    /*FPGA部分*/
    int trans_cnt;      // 向FPGA传输的总图片数


    /*倍率检测部分*/
    QCamera* m_camera;                         // 倍率检测相机对象
    QVideoSink* m_mySink;
    QMediaCaptureSession* m_captureSession;    
    QThread* m_magDetThread;                   // 倍率检测线程
    TMagDetImage* m_magDetImage;               // 倍率检测图片对象
    int m_gap_image;                           // 倍率检测的帧间隔
    int m_color_res;                           // 0-1-2-3-4

    /*界面部分*/
    QCheckBox* m_cbox_auto;
    QCheckBox* m_cbox_save;
    QCheckBox* m_cbox_mag;

    QSlider* m_slider_expoTarget;   // 滑动条：曝光目标
    QSlider* m_slider_expoTime;     // 滑动条：曝光时间
    QSlider* m_slider_expoGain;     // 滑动条：曝光增益
    QSlider* m_slider_temp;
    QSlider* m_slider_tint;

    QLabel* m_lbl_expoTarget;
    QLabel* m_lbl_expoTime;
    QLabel* m_lbl_expoGain;
    QLabel* m_lbl_temp;
    QLabel* m_lbl_tint;
    QLabel* m_lbl_video;
    QLabel* m_lbl_frame;
    QLabel* m_lbl_debug;

    QPushButton* m_btn_defaultWB;
    QPushButton* m_btn_autoWB;
    QPushButton* m_btn_open;       // 按钮：打开/关闭
    QPushButton* m_btn_connect;    // 按钮：连接设备/断开设备
    QPushButton* m_btn_trans;      // 按钮：开始传输/停止传输

    QTimer* m_timer;

    // ================================ 私有槽/信号函数 ================================
private slots:

    void slot_on_magnify_frame_changed(const QVideoFrame& frame);

signals:
    /**
     * @brief 当用户没有确认PID，直接点击开始传输时，emit该信号，默认为点击了确认PID的按钮
     */
    void directly_click_btn_trans();
};

