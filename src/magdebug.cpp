#include "magdebug.h"
#include <QCamera>
#include <QVideoSink>
#include <QMediaCaptureSession>
#include <QMessageBox>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QPainter>
#include "ui_magdebug.h"
#include "IniParser.h"


struct Color2 {
    std::string color;
    cv::Scalar lower;
    cv::Scalar upper;
};

// 定义颜色的 HSV 范围
std::map<int, Color2> colors2 = {
    {0, {"2",cv::Scalar(274, 94 * 1.0 / 255, 52 * 1.0 / 255), cv::Scalar(320, 215 * 1.0 / 255, 219 * 1.0 / 255)}},
    {1, {"4", cv::Scalar(327, 104 * 1.0 / 255, 100 * 1.0 / 255),cv::Scalar(360, 255 * 1.0 / 255, 255 * 1.0 / 255)}},
    {2, {"10",cv::Scalar(32, 70 * 1.0 / 255, 91 * 1.0 / 255), cv::Scalar(77, 255 * 1.0 / 255, 255 * 1.0 / 255)}},
    {3, {"20",cv::Scalar(93, 73 * 1.0 / 255, 65 * 1.0 / 255), cv::Scalar(198, 244 * 1.0 / 255, 146 * 1.0 / 255)}},
    {4, {"40",cv::Scalar(173, 153 * 1.0 / 255, 106 * 1.0 / 255),cv::Scalar(254, 255 * 1.0 / 255, 197 * 1.0 / 255)}}
};



MagDebug::MagDebug(QWidget *parent):
    QWidget(parent),
    ui(new Ui::MagDebug),
    m_str_hint("请移动摄像头，直至物镜位于视野中央"),
    m_camera(nullptr),
    m_frame_counter(0),
    m_fps_controller(0),
    m_captureSession(new QMediaCaptureSession(this)),
    m_sink(new QVideoSink(this))
{
    ui->setupUi(this);

    ui->label_hint->setText(m_str_hint);

    QString iniPath = M_INIT_FILE_PATH;
    QString groupName = M_GROUP_NAME;
    QString rectX = IniParser::readIniSettings(iniPath, groupName, "RectX");
    QString rectY = IniParser::readIniSettings(iniPath, groupName, "RectY");

    m_rectX = rectX.toInt();
    m_rectY = rectY.toInt();
    ui->lineEdit_rectx->setText(rectX);
    ui->lineEdit_recty->setText(rectY);

    QString m_show_magdet_info = IniParser::readIniSettings(iniPath, groupName, "ShowMagDetInfo");

    // RectX的输入范围是0-1160,RectY的输入范围是0-770
    QIntValidator* validator_x = new QIntValidator(0, 1160, this);
    ui->lineEdit_rectx->setValidator(validator_x);
    QIntValidator* validator_y = new QIntValidator(0, 770, this);
    ui->lineEdit_recty->setValidator(validator_y);

    // 先找所有相机
    // 隐患：相机名称并不是相机的唯一标识。当存在多个同名相机时，只会使用最后一个相机作为倍率检测相机
    const QList<QCameraDevice> videoDevices = QMediaDevices::videoInputs();
    for (const QCameraDevice &device : videoDevices)
    {
        QString camera_name = device.description();
        QByteArray camera_id = device.id();

        ui->combo_list->addItem(camera_name);

        qDebug() << "ID: " << camera_id;
        qDebug() << "Description: " << device.description();
        qDebug() << "Is default: " << (device.isDefault() ? "Yes" : "No");

        if(camera_name == CAMERA_NAME){
            if(m_camera){
                delete m_camera;
                m_camera = nullptr;
            }
            m_camera = new QCamera(device);
        }
    }

    if(m_camera){
        m_captureSession->setCamera(m_camera);      // 设置捕获会话的视频源
        m_captureSession->setVideoSink(m_sink);     // 设置捕获会话的视频输出
        m_camera->start();      // 启动摄像头开始捕获视频流
    }

    // 连接视频帧处理信号
    connect(m_sink, &QVideoSink::videoFrameChanged,this, &MagDebug::slot_on_video_frame_changed);
}


MagDebug::~MagDebug()
{
    delete ui;
}


void MagDebug::slot_on_video_frame_changed(const QVideoFrame &frame)
{
    if(frame.isValid()){
        // 将视频流图像转换为QImage，格式为QImage::Format_RGBA8888_Premultiplied
        QImage img = frame.toImage();
        m_frame_counter++;

        if (m_show_magdet_info == 1) {
            qDebug() << "Frame Number:" << m_frame_counter;
        }
        
        m_fps_controller++;
        if(m_fps_controller == 5){
            m_fps_controller = 0;
            show_camera_img(img);
        }
    }
}

void MagDebug::show_camera_img(QImage image)
{
    // 视频帧颜色格式转换
    if(image.format() == QImage::Format_RGBA8888_Premultiplied){
        image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }
    cv::Mat mat = qImageToMatBGR(image);

    // 转换为浮点
    cv::Mat bgr;
    mat.convertTo(bgr, CV_32FC3, 1.0 / 255, 0);

    // 颜色空间转换
    cv::Mat hsvImage;
    cv::cvtColor(bgr, hsvImage, cv::COLOR_BGR2HSV);

    // 选取中心区域
    int width = image.width();
    int height = image.height();
    int rectWidth = width / 3;
    int rectHeight = height / 3.5;
    int rectX = m_rectX;
    int rectY = m_rectY;
    cv::Mat centerRegion = hsvImage(cv::Rect(rectX, rectY, rectWidth, rectHeight));

    // 计算主要颜色
    std::string mainColor = calculateMainColor(centerRegion);

    // 定义矩形的左上角和右下角坐标
    // 缩小图像
    int _ui_width = ui->label_video->width();
    int _scale_para = width/_ui_width;
    QImage scaledImage = image.scaled(width/_scale_para, height/_scale_para, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 绘制矩形框
    QImage labeledImage = scaledImage.copy(); // 创建图像的副本以绘制矩形框
    QPainter painter(&labeledImage);
    painter.setPen(Qt::red); // 设置画笔颜色为红色

    // 框出缩小后图片的中心区域
    int rectWidth2 = scaledImage.width() / 3;
    int rectHeight2 = scaledImage.height() / 3.5;
    painter.drawRect(rectX/ _scale_para, rectY/_scale_para, rectWidth2, rectHeight2); // 绘制矩形框
    painter.end();

    QPixmap pixmap = QPixmap::fromImage(labeledImage);
    ui->label_video->setPixmap(pixmap);

    // 输出主要颜色
    QString _color_res;
    if(mainColor == "2"|| mainColor == "4"|| mainColor == "10"|| mainColor == "20"|| mainColor == "40"){
        _color_res = QString::fromStdString(mainColor);
    }
    else
    {
        _color_res = "Null";
    }
    ui->label_res->setText("×" + _color_res);
}


cv::Mat MagDebug::qImageToMatBGR(const QImage &image)
{
    switch (image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    {
        cv::Mat mat(image.height(), image.width(), CV_8UC4, const_cast<uchar*>(image.bits()));
        cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);  //移除alpha在rgb通道上的比重
        return mat;
    }
    default:
        throw std::runtime_error("Unsupported image format");
    }
}

std::string MagDebug::calculateMainColor(const cv::Mat &frame)
{
    std::map<std::string, int> colorCounts;

    for (const auto& color : colors2) {
        cv::Mat mask;

        // 二值化处理
        cv::inRange(frame, color.second.lower, color.second.upper, mask);
        colorCounts[color.second.color] = cv::countNonZero(mask);
    }

    int maxCount = 0;
    std::string mainColor;

    for (const auto& count : colorCounts) {
        if (m_show_magdet_info == 1) {
            qDebug() << "color: " << count.first << "-" << count.second;
        }
        
        if (count.second > maxCount) {
            maxCount = count.second;
            mainColor = count.first;
        }
    }
    return mainColor;
}


void MagDebug::on_pushButton_clicked()
{
    QString rectx_temp = ui->lineEdit_rectx->text().remove(QRegularExpression("\\s+"));
    rectx_temp.remove(QRegularExpression("[,，]$"));
    QString recty_temp = ui->lineEdit_recty->text().remove(QRegularExpression("\\s+"));
    recty_temp.remove(QRegularExpression("[,，]$"));

    if (recty_temp.toInt() <= 770 && rectx_temp.toInt() <= 1160) {
        QString iniPath = M_INIT_FILE_PATH;
        QString groupName = M_GROUP_NAME;

        IniParser::saveIniSettings(iniPath, groupName, "RectX", rectx_temp);
        IniParser::saveIniSettings(iniPath, groupName, "RectY", recty_temp);
        m_rectX = IniParser::readIniSettings(iniPath, groupName, "RectX").toInt();
        m_rectY = IniParser::readIniSettings(iniPath, groupName, "RectY").toInt();

        emit signal_rect_value_changed(m_rectX, m_rectY);
    }
    else {
        QMessageBox::warning(this, "参数范围错误", "RectX的允许范围为：0-1160 \n RectY的允许范围为：0-770");
    }

}
