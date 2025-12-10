#include "tmagnifydetect.h"

#include "IniParser.h"

#include <QDebug>

TMagnifyDetect::TMagnifyDetect(TMagDetImage* _det_img)
{
    m_det_img = _det_img;
    m_slice = 3;
    m_last_color = 0; // 默认白色

    QString iniPath = INIT_FILE_PATH;
    QString groupName = GROUP_NAME;
    m_show_magdet_info = IniParser::readIniSettings(iniPath, groupName, "ShowMagDetInfo").toInt();
    m_rectX = IniParser::readIniSettings(iniPath, groupName, "RectX").toInt();
    m_rectY = IniParser::readIniSettings(iniPath, groupName, "RectY").toInt();
}

void TMagnifyDetect::working()
{
    while (!is_closed())
    {
        while (!is_stopped())
        {
            if (m_det_img->m_lock.try_lock())
            {
                // std::cout << "------------DetTMagnifect::working start----------------";
                if (!m_det_img->m_image.isNull())
                {
                    QImage image = m_det_img->m_image;
                    if (image.format() == QImage::Format_RGBA8888_Premultiplied)
                    {
                        image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
                    }
                    cv::Mat mat = qImageToMat(image);
                    // 颜色转换成bgr格式
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
                    // int rectX = width / 2.35;
                    // int rectY = height * 0.4;
                    int rectX = m_rectX;
                    int rectY = m_rectY;
                    if (m_show_magdet_info == 1)
                    {
                        qDebug() << "RECTX:" << rectX << "RECTY:" << m_rectY;
                    }
                    cv::Mat centerRegion = hsvImage(cv::Rect(rectX, rectY, rectWidth, rectHeight));
                    // 计算主要颜色
                    // std::string mainColor = calculateMainColor(centerRegion);
                    std::pair<std::string, int> res = calculateMainColor(centerRegion);
                    std::string mainColor = res.first;
                    int maxCount = res.second;

                    int _color = 0;
                    if (mainColor == "2")
                    {
                        _color = 2;
                    }
                    else if (mainColor == "4")
                    {
                        _color = 4;
                    }
                    else if (mainColor == "10")
                    {
                        _color = 10;
                    }
                    else if (mainColor == "20")
                    {
                        _color = 20;
                    }
                    else if (mainColor == "40")
                    {
                        _color = 40;
                    }

                    m_det_img->color = _color;
                    m_det_img->count = maxCount;
                    float rt = (maxCount - m_last_mag_count) * 1.0 / maxCount;
                    if (_color == m_last_color && rt < 0.2 && rt > -0.2)
                    {
                        m_det_img->color = _color;
                    }
                    else
                    {
                        m_det_img->color = 0;
                    }
                    if (m_show_magdet_info == 1)
                    {
                        qDebug() << "----The detected magnification is:" << m_det_img->color
                                 << "----maxcount: " << maxCount << "------rt: " << rt;
                    }

                    m_last_color = _color;
                    m_last_mag_count = maxCount;
                    m_det_img->m_image = QImage();
                }
                m_det_img->m_lock.unlock();
            }
            // Sleep(100);
        }
        Sleep(300);
    }
}

cv::Mat TMagnifyDetect::qImageToMat(const QImage& image)
{
    switch (image.format())
    {
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32:
        case QImage::Format_ARGB32_Premultiplied:
        {
            cv::Mat mat(image.height(), image.width(), CV_8UC4, const_cast<uchar*>(image.bits()));
            cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR); // 移除alpha在rgb通道上的比重
            return mat;
        }
        default:
            qDebug() << "Unsupported image format";
            exit(1);
            // throw std::runtime_error("Unsupported image format");
    }
}

QImage TMagnifyDetect::matToQImage(const cv::Mat& _mat)
{
    if (_mat.type() == CV_8UC3)
    {
        // BGR转为RGB
        cv::Mat rgb;
        cv::cvtColor(_mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    }
    else
    {
        std::cerr << "Unsupported image format" << std::endl;
        return QImage();
    }
}

std::pair<std::string, int> TMagnifyDetect::calculateMainColor(const cv::Mat& frame)
{
    std::map<std::string, int> colorCounts;
    for (const auto& color : colors)
    {
        cv::Mat mask;
        // 二值化处理
        cv::inRange(frame, color.second.lower, color.second.upper, mask);

        colorCounts[color.second.color] = cv::countNonZero(mask);
    }

    int maxCount = 0;
    std::string mainColor;

    for (const auto& count : colorCounts)
    {
        // qDebug() << "color: "<< count.first << "-" << count.second;
        if (count.second > maxCount)
        {
            maxCount = count.second;
            mainColor = count.first;
        }
    }
    // qDebug()<<"mainColor: " << mainColor;
    {
        return {mainColor, maxCount};
    }
}

TMagDetImage::TMagDetImage()
{
    color = 0;
}

TMagDetImage::~TMagDetImage() {}
