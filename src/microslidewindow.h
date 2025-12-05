#pragma once

#include <QMainWindow>
#include <QSqlDatabase>
#include "qsqltablemodel.h"


QT_FORWARD_DECLARE_CLASS(QDialogButtonBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QSqlTableModel)


//#define OPEN_DATA_BASE_STORE

// 定义图片在左侧栏中的显示宽度
#define PIC_WIDTH 325       // 之前是280


class QLabel;


/*Class declaration*/
class SysConfigForm;
struct AIResult;
struct BestImage;
class MagDebug;
class FileTest;
class ScannerProcessor;

namespace Ui {
    class MicroSlideWindow;
}


class MicroSlideWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MicroSlideWindow(QWidget* parent = nullptr);
    ~MicroSlideWindow();

    void initApp();
    void initBusNumWindow();            // 初始化得到设备id
    void initVariable();                // 初始化一些变量
    void initTableImg();                // 初始化表格-输出结果样式
    void initUI();                      // ui上的优化

    //QString m_img_save_path;


    void initDataBase();
    

public slots:
    /**
     * @brief 将筛选出来的图片及AI处理的结果显示在图片栏
     */
    void slot_add_image_to_table(const BestImage* img, const AIResult* img_inference_res, QString _video_res);

    /**
     * @brief 实时预览图片，增强显示的实时性
     * @param _preview_img 预览图片
     * @param _best_img_num 筛选出的图片的数量，用于确定预览图片所在的行数
     */
    void slot_add_preview_img_to_table_line(QPixmap _preview_img, int _best_img_num);

    /**
     * @brief 删除预览图片
     * @param _best_img_num 筛选出的图片的数量，用于确定预览图片所在的行数
     */
    void slot_delete_preview_img_from_table_line(int _best_img_num);

    void slot_change_preview_img_from_table_line(unsigned int _best_img_num);

    /**
     * @brief 系统配置修改槽函数
     * @param _sharp 
     * @param _similarity 
     * @param _area 
     * @param _filepath 
     * @param db_addr 
     * @param db_port 
     */
    void slot_on_syscfgChanged(int _sharp, int _similarity, int _area, QString _filepath, std::string db_addr, int db_port);

    /**
     * @brief 11111111111111111111
     * @param diagnose 
     */
    void slot_on_ai_inference_finished(QString diagnose);

    /*wll数据库相关*/
    void slot_on_update_view_port();


signals:
    void signal_ai_inference_finished(QString diagnose);

    void doctor_diagnosis_finished();


private:
    Ui::MicroSlideWindow* ui;
    SysConfigForm* sysCfgForm;
    FileTest* ft;
    MagDebug* magdebug;
    QSqlDatabase db;
    int m_count_res_img = 0;

    /*扫描仪对象*/
    ScannerProcessor* m_scanner;
    bool is_scannerConnected;
    QTimer* m_scannerTimer;
    QTimer* m_scannerChecker;

    /*powerPC有关设备问题*/
    int m_cur_bus_num = 0;                    /*总线id号*/


    /*wll数据库相关*/
    QSqlTableModel* model;
    QSqlTableModel* model2;
    unsigned int m_db_id = 0;
    int m_db_patient_id;
    QString m_db_slice_id = "TEST";

    /*扫描仪相关*/
    /**
     * @brief 处理扫描仪输入的PID
     * 
     * @param input 输入的有效PID
     */
    void scanner_PID_captured(const std::string& input);

    /**
     * @brief 初始化扫描仪设备
     */
    void initialize_scanner();

    /**
     * @brief 释放扫描仪设备
     */
    void release_scanner();

    /**
     * @brief 连接与扫描仪相关的信号和槽函数
     */
    void setup_scanner_connections();

    /**
     * @brief 设置扫描仪连接状态
     * @param isConnected 
     */
    void set_scanner_state(bool isConnected);

    void update_scannerTimer_state();


private slots:
    /**
     * @brief 清空图片栏中的所有widget。QT会自动释放widget的内存
     */
    void clear_image_in_table();

    void on_btn_id_clicked();

    void on_btn_confirm_doc_clicked();

    QString get_doctor_diagnosis_result();

    void on_act_sysConfig_triggered();

    /*wll数据库相关*/
    void on_combo_diagnose_currentTextChanged(const QString& arg1);

    void on_btn_search_clicked();

    void on_btn_all_info_clicked();

    void slot_on_tableView_db_2_double_clicked(const QModelIndex& index);

    void slot_on_tableView_data_changed(const QModelIndex& index);

    void on_btn_close_res_img_clicked();


    //添加数据到病理结果表
    void slot_on_add_data_to_diagnosis_results(int patient_id = 0,       //病人id，可以理解为病人的病历号
        QString slice_id = "",     //切片名
        int mag = 0,              //放大倍数
        QString img_path = "",    //图像存储路径
        QString diag_res = "",     //诊断结果
        float confidence = 0,     //置信度
        int coord_x = 0,          //判断依据x坐标
        int coord_y = 0          //判断依据y坐标
    );


    void on_action_filetest_triggered();

    /*扫描仪相关函数*/
    /**
     * @brief 弹窗提示扫描仪错误事件
     * @param error 错误提示字符
     */
    void on_scanner_error_occured(const QString &error);

    /**
     * @brief 每隔一段时间尝试初始化扫描仪
     */
    void try_initialize_scanner();

    /**
     * @brief 当扫描仪连接后，一定时间查询一下扫描仪USB口状态，判断设备是否拔出
     */
    void check_scanner_usb_status();

};

