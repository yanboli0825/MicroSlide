#pragma once
#include <QObject>
#include <opencv2/opencv.hpp>
#include <QImage>
#include <QVideoFrame>
#include <QApplication>
#include "basethread.h"



// 定义颜色及其 HSV 范围
struct Color {
    std::string color;
    cv::Scalar lower;
    cv::Scalar upper;
};

class TMagDetImage {
public:
    TMagDetImage();
    ~TMagDetImage();

public:
    std::mutex m_lock;
    QImage m_image;
    int color;
    int count;
};

class TMagnifyDetect : public BaseThread
{
    Q_OBJECT
public:
    explicit TMagnifyDetect(TMagDetImage* _det_img);

public slots:
    virtual void working() override;

private:
    // 定义颜色的 HSV 范围
    std::map<int, Color> colors = {
        {0, {"2",cv::Scalar(274, 94 * 1.0 / 255, 52 * 1.0 / 255), cv::Scalar(320, 215 * 1.0 / 255, 219 * 1.0 / 255)}},
        {1, {"4", cv::Scalar(327, 104 * 1.0 / 255, 100 * 1.0 / 255),cv::Scalar(360, 255 * 1.0 / 255, 255 * 1.0 / 255)}},
        {2, {"10",cv::Scalar(32, 70 * 1.0 / 255, 91 * 1.0 / 255), cv::Scalar(77, 255 * 1.0 / 255, 255 * 1.0 / 255)}},
        {3, {"20",cv::Scalar(93, 73 * 1.0 / 255, 65 * 1.0 / 255), cv::Scalar(198, 244 * 1.0 / 255, 146 * 1.0 / 255)}},
        {4, {"40",cv::Scalar(173, 153 * 1.0 / 255, 106 * 1.0 / 255),cv::Scalar(254, 255 * 1.0 / 255, 197 * 1.0 / 255)}}
    };
    int m_slice;                    //分成几份
    TMagDetImage* m_det_img;        //带互斥锁的image
    int m_last_color;
    int m_last_mag_count = 0;
    int m_show_magdet_info;         //是否打印倍率检测线程的信息，1表示打印



public:
    cv::Mat qImageToMat(const QImage &image);
    QImage matToQImage(const cv::Mat &_mat);
    std::pair<std::string, int> calculateMainColor(const cv::Mat &frame);
    int m_rectX;
    int m_rectY;
};