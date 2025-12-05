#include "filetest.h"
#include "ui_filetest.h"
#include <QFileDialog>
#include <windows.h>
#include <QThread>
#include <thread>
#include <filesystem>
#include <QMessageBox>
#include "hpec_lib.h"
#include "hl_lib.h"


FileTest::FileTest(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FileTest)
{
    ui->setupUi(this);
    ui->lineEdit->setText("E:/test");       // 默认值
    
}

FileTest::~FileTest()
{
    delete ui;
}

void FileTest::on_pushBtn_confirm_clicked()
{
    QString root = ui->lineEdit->text();
    if (!std::filesystem::exists(root.toStdString())) {
        QMessageBox::warning(this, "测试错误", "该路径不存在");
        return;
    }
    if (!std::filesystem::is_directory(root.toStdString())) {
        QMessageBox::warning(this, "测试错误", "该路径不是目录");
        return;
    }
    ui->pushBtn_confirm->setEnabled(false);
    std::thread send_img_thread([root, this]() {
        for (const auto& entry : std::filesystem::directory_iterator(root.toStdString())) {
            std::string file_path = entry.path().string();
            QFileInfo file_info(QString::fromStdString(file_path));
            if (file_info.suffix().toLower() != QString::fromStdString("png")) {
                continue;
            }

            emit this->signal_send_img(QString::fromStdString(file_path));
            Sleep(100);
        }


        // ......
        int pc_bus_num = bus_num_get(0);
        if (pc_bus_num == -1) {
            qDebug() << "获取总线id失败";
        }
        else {
            qDebug() << "获取总线id成功：" << pc_bus_num;
        }

        int fpga_hDevice = OpenDevice(pc_bus_num);//连接设备
        if (fpga_hDevice >= 0)
        {
            int ret1 = 0;
            ret1 = write_19eg_reg(fpga_hDevice, 0x6000 + 24 * 4, 0x80000001);
            ret1 = write_19eg_reg(fpga_hDevice, 0x6000 + 24 * 4, 0x00000000);
        }


        this->close();
        });
    send_img_thread.detach();

}


void FileTest::on_pushBtn_select_clicked() {
    QString selectDir = QFileDialog::getExistingDirectory();
    ui->lineEdit->setText(selectDir);
}