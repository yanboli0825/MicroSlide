#pragma once
#include <QImage>
#include <opencv2/opencv.hpp>

namespace utils
{
/**
 * @brief 将RGB888格式的QImage图像转换为RGB格式的cv::Mat图像
 *
 * @param[in] qimage 输入图像，格式为RGB888，若不为RGB888则强制转换为RGB888
 *
 * @return RGB格式的cv::Mat
 */
cv::Mat QImageToMatRGB(const QImage& qimage);

/**
 * @brief 将宽字符转换为标准字符串
 *
 * @param[in] wchar 宽字符
 *
 * @return 标准字符串
 */
std::string wcharToString(const wchar_t* wchar);

} // namespace utils
