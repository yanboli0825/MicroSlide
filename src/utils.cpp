#include "utils.h"

#include <codecvt>

cv::Mat utils::QImageToMatRGB(const QImage& qimage)
{
    QImage rgbImage;

    // 检查输入图像格式，若不是RGB888格式则将其转换为RGB888格式
    if (qimage.format() != QImage::Format_RGB888)
    {
        rgbImage = qimage.convertToFormat(QImage::Format_RGB888);
    }
    else
    {
        rgbImage = qimage; // 这里直接赋值，因为QImage有隐式共享机制，不会深拷贝
    }

    cv::Mat matRGB(rgbImage.height(), rgbImage.width(), CV_8UC3);
    std::memcpy(matRGB.data, rgbImage.constBits(), rgbImage.sizeInBytes());

    return matRGB;
}

std::string utils::wcharToString(const wchar_t* wchar)
{
    try
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(wchar);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Conversion error: " << e.what() << std::endl;
        return "";
    }
}
