#include "SystemCfg.h"

#include "IniParser.h"
#include "ui_sysconfigform.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSettings>
#include <QTimer>

SysConfigForm::SysConfigForm(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::SysConfigForm)
{
    ui->setupUi(this);
    initUI();
}

SysConfigForm::~SysConfigForm()
{
    delete ui;
}

void SysConfigForm::initUI()
{
    // 读取之前保存的清晰度、相似度、文件保存目录数据
    QString iniPath = INIT_FILE_PATH;
    QString groupName = GROUP_NAME;
    QString clarity = IniParser::readIniSettings(iniPath, groupName, "Clarity");
    QString similarity = IniParser::readIniSettings(iniPath, groupName, "Similarity");
    QString filePath = IniParser::readIniSettings(iniPath, groupName, "FilePath");
    QString area = IniParser::readIniSettings(iniPath, groupName, "Area");
    QString db_addr = IniParser::readIniSettings(iniPath, groupName, "DBAddr");
    QString db_port = IniParser::readIniSettings(iniPath, groupName, "DBPort");

    // 设置清晰度、相似度、文件保存目录的初始显示内容
    ui->lineEdit_clarity->setText(clarity);
    ui->lineEdit_similarity->setText(similarity);
    ui->lineEdit_filePath->setText(filePath);
    ui->lineEdit_area->setText(area);
    ui->lineEdit_db_ip->setText(db_addr);
    ui->lineEdit_db_port->setText(db_port);
}

void SysConfigForm::on_btn_filePath_clicked()
{
    QString selectDir = QFileDialog::getExistingDirectory();
    ui->lineEdit_filePath->setText(selectDir);
}

void SysConfigForm::on_btn_save_clicked()
{
    // 写config.ini
    QString clarity_temp = ui->lineEdit_clarity->text().remove(QRegularExpression(
        "\\s+")); // QRegularExpression是Qt里的一个类，这里用来创建了一个正则表达式匹配文本里的所有空白（"\s+"）字符[若为\s则只匹配一个空白字符]，然后用remove()移除
    clarity_temp.remove(
        QRegularExpression("[,，]$")); // 正则表达式匹配字符串末尾的英文和中文逗号。[]是字符集，$是锚点，表示末尾
    QString similarity_temp = ui->lineEdit_similarity->text().remove(QRegularExpression("\\s+"));
    similarity_temp.remove(QRegularExpression("[,，]$"));
    QString filePath_temp = ui->lineEdit_filePath->text().remove(QRegularExpression("\\s+")); // 保存目录里不要有空格
    filePath_temp.remove(QRegularExpression("[,，]$"));
    QString area_temp = ui->lineEdit_area->text().remove(QRegularExpression("\\s+"));
    area_temp.remove(QRegularExpression("[,，]$"));
    QString db_addr_temp = ui->lineEdit_db_ip->text().remove(QRegularExpression("\\s+"));
    db_addr_temp.remove(QRegularExpression("[,，]$"));
    QString port_temp = ui->lineEdit_db_port->text().remove(QRegularExpression("\\s+"));
    port_temp.remove(QRegularExpression("[,，]$"));

    QMessageBox* box = new QMessageBox(QMessageBox::Information, tr("提示"), tr("已完成修改"));
    // QTimer::singleShot(1500,box,SLOT(close()));//1500ms后单次执行close()
    box->exec(); // 这行代码显示消息框，并进入一个模态事件循环，直到消息框被关闭。exec
                 // 方法将使消息框成为模态对话框，这意味着在用户关闭消息框之前，他们不能与其他窗口交互
    emit signal_syscfgChanged(clarity_temp.toInt(), similarity_temp.toInt(), area_temp.toInt(), filePath_temp,
                              db_addr_temp.toStdString(), port_temp.toInt());
    qDebug() << "The System Configuration has been changed!";

    QString iniPath = INIT_FILE_PATH;
    QString groupName = GROUP_NAME;
    IniParser::saveIniSettings(iniPath, groupName, "Clarity", clarity_temp);
    IniParser::saveIniSettings(iniPath, groupName, "Similarity", similarity_temp);
    IniParser::saveIniSettings(iniPath, groupName, "FilePath", filePath_temp);
    IniParser::saveIniSettings(iniPath, groupName, "DBAddr", db_addr_temp);
    IniParser::saveIniSettings(iniPath, groupName, "DBPort", port_temp);
    SysConfigForm::destroy();
}

void SysConfigForm::on_btn_return_clicked()
{
    close();
}
