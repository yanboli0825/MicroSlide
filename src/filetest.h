#pragma once
#include <QWidget>

namespace Ui
{
class FileTest;
}

class FileTest : public QWidget
{
    Q_OBJECT

public:
    explicit FileTest(QWidget* parent = nullptr);
    ~FileTest();

private:
    Ui::FileTest* ui;

private slots:
    /**
     * @brief 选择测试文件路径槽函数
     */
    void on_pushBtn_select_clicked();

    /**
     * @brief 按照一定间隔发送signal_testSingleImage信号，等待连接的槽函数处理
     */
    void on_pushBtn_confirm_clicked();

signals:
    /**
     * @brief 待处理图片发送信号
     *
     * @param img 待处理图片
     */
    void signal_testSingleImage(QImage img);
};
