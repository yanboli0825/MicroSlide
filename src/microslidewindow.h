#pragma once

#include <QMainWindow>
#include <memory>

QT_FORWARD_DECLARE_CLASS(QDialogButtonBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QLabel)

#define PIC_WIDTH 325 ///< 图片在ImageTable中的显示宽度。曾用值：280

class SysConfigForm;
struct AIResult;
struct BestImage;
class MagDebug;
class FileTest;
class ScannerProcessor;

namespace Ui
{
class MicroSlideWindow;
}

class MicroSlideWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MicroSlideWindow(QWidget* parent = nullptr);
    ~MicroSlideWindow();

    /**
     * @brief 初始化 PowerPC 设备，准备Windows上位机资源
     *
     *@note 必须在 MicroSlideWindow 构造函数中调用，只能调用一次
     */
    void initializePowerPC();

    /**
     * @brief 释放 PowerPC 设备资源
     *
     */
    void releasePowerPC();

    /**
     * @brief 建立信号和槽函数连接
     *
     */
    void setupConnections();

    /**
     * @brief 设置图片栏的样式，包括表头字体、滑杆等
     */
    void setImgTableStyle();

    /**
     * @brief 将诊断结果按钮设置为非互斥模式
     */
    void setBtnsNonExclusive();

public slots:
    /**
     * @brief 将预处理后的图片及AI推理结果显示在ImageTable中
     */
    void slot_addImageToTable(const std::shared_ptr<BestImage>& img, const std::shared_ptr<AIResult>& img_inference_res,
                              const QString& _video_res);

    /**
     * @brief 实时预览图片，增强显示的实时性
     * @param _preview_img 预览图片
     * @param _best_img_num 筛选出的图片的数量，用于确定预览图片所在的行数
     */
    void slot_addPreviewImgToTable(QPixmap _preview_img, int _best_img_num);

    /**
     * @brief 系统配置修改槽函数
     * @param _sharp
     * @param _similarity
     * @param _area
     * @param _filepath
     * @param db_addr
     * @param db_port
     */
    void slot_on_syscfgChanged(int _sharp, int _similarity, int _area, QString _filepath, std::string db_addr,
                               int db_port);

    /**
     * @brief 等待线程池推理完所有图片后，控制确定病理号的按钮以及完成诊断相关UI操作
     * @param diagnose 医生给出的诊断结果，用于发送给数据库
     */
    void slot_threadPoolFinished(QString diagnose);

signals:
    /**
     * @brief 当AI推理线程池处理完成所有图片后发出信号
     *
     * @param diagnose 医生给出的诊断结果
     */
    void signal_threadPoolFinished(QString diagnose);

    /**
     * @brief 解除被事件循环阻塞的线程的信号
     *
     */
    void signal_unblockThread();

private:
    /*PowerPC设备相关*/
    int m_busNum = 0; ///< PowerPC设备总线号

    Ui::MicroSlideWindow* ui;
    SysConfigForm* systenCfg; ///< 系统配置窗口
    FileTest* ft;             ///< 文件测试窗口
    MagDebug* magdebug;       ///< 倍率调试窗口

    /*扫描仪相关*/
    /**
     * @brief 处理扫描仪输入的PID
     *
     * @param input 输入的有效PID
     */
    void slot_scannerPIDCaptured(const std::string& input);

    /**
     * @brief 初始化扫描仪设备
     */
    void initializeScanner();

    /**
     * @brief 释放扫描仪设备
     */
    void releaseScanner();

    /**
     * @brief 连接与扫描仪相关的信号和槽函数。主要是处理扫描仪解码数据的槽函数
     */
    void setupScannerConnections();

    /**
     * @brief 从按钮获取医生诊断结果
     *
     * @return 医生诊断结果
     */
    QString getDoctorDiagnosisResultFromBtn();

    /**
     * @brief 清空图片栏中的所有widget。QT会自动释放widget的内存
     */
    void clearImagesInTable();

private slots:

    /**
     * @brief 点击确认病理号按钮后的槽函数
     *
     */
    void on_btn_id_clicked();

    /**
     * @brief 点击确认医生诊断结果按钮后的槽函数
     *
     */
    void on_btn_confirm_doc_clicked();

    /**
     * @brief 点击系统配置菜单后的槽函数
     *
     */
    void on_act_sysConfig_triggered();

    /**
     * @brief 点击文件测试菜单后的槽函数
     *
     */
    void on_action_filetest_triggered();
};
