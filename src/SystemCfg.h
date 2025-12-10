#pragma once
#include <QWidget>

namespace Ui
{
class SysConfigForm;
}

class SysConfigForm : public QWidget
{
    Q_OBJECT

public:
    explicit SysConfigForm(QWidget* parent = nullptr);
    ~SysConfigForm();

    /**
     * @brief 将.ini文件内保存的清晰度、相似度和文件保存的路径等读入并渲染
     */
    void initUI();

signals:
    /**
     * @brief 当点击保存按钮时发出信号，传递用户输入的清晰度、相似度、区块面积阈值，以及显微镜图片保存路径
     */
    void signal_syscfgChanged(int _sharp, int _similarity, int _area, QString _filepath, std::string db_addr,
                              int db_port);

private slots:
    /**
     * @brief 点击选择路径按钮后，弹出路径选择框
     */
    void on_btn_filePath_clicked();

    /**
     * @brief 保存
     */
    void on_btn_save_clicked();

    /**
     * @brief 取消
     */
    void on_btn_return_clicked();

private:
    Ui::SysConfigForm* ui;
};
