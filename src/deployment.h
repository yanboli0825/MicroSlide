#pragma once
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include <QWidget>
#include <QPainter>
#include <QString>


/*OnnxRuntime设置*/
#define INTRA_OP_NUM_THREADS     4

/*Onnx AI模型路径*/
#define ONNX_CTRANS_PATH         L"../../../assets/onnx_model/ctranspath_v14_batch28.onnx"    // 特征提取基础模型
#define ONNX_POLYP_PATH          L"../../../assets/onnx_model/AMIL_1219_blur_v14.onnx"       // 肠息肉。L表示该字符串为宽字符
#define ONNX_FROZEN_PATH         L"../../../assets/onnx_model/frozen_cancer.onnx"             // 冷冻切片。目前仅支持癌症筛查
#define ONNX_MMR_PATH            L"../../../assets/onnx_model/MMR.onnx"                       // MMR基因预测，针对肠息肉切片

/*裁剪patch参数*/
#define WINDOW_SIZE       454    // 裁剪patch时的窗长
#define STRIDE            227    // 裁剪patch时的步长

/*图像筛选参数*/
#define BINARY_THRESHOLD  200    // patch_filter()函数的参数，用于过滤二值图像
#define VAILD_THRESHOLD  0.03    // patch_filter()函数的参数，用于判断patch是否有效

/*常量定义*/
inline std::vector<std::string> POLYP_CLASSES = { "C", "SSA", "TA", "HP", "IP", "NM" };

/*切片来源定义*/
#define SLICESOURCE_STOMACH    "2000"   // 胃
#define SLICESOURCE_GUT        "2010"   // 肠
#define SLICESOURCE_PROSTATE   "2020"   // 前列腺
#define SLICESOURCE_UNKNOWN    "1000"   // 未知,暂时当冰冻切片使用
#define SLICESOURCE_DEFAULT    "1000"   // 均不属于上面定义的来源类型时，默认使用癌和非癌分类模型

/*定义疾病类型*/
#define CLSNAME_STOMACH        {"NM", "C"}
#define CLSNAME_GUT            {"C", "SSA", "TA", "HP", "IP", "NM"}
#define CLSNAME_PROSTATE       {"NM", "C"}
#define CLSNAME_KNOWN          {"NM", "C"}
#define CLSNAME_DEFAULT        {"NM", "C"}

struct OrtComponents;


class deployment
{
public:
    // ================================ 构造/析构函数 ================================
	explicit deployment();
	~deployment();

    // ================================ 公共成员函数 ================================
    /**
    * @brief 从输入图像中提取有效图像块（patch）及其位置
    * 
    * @param[in] rgb_image 输入图像，需满足：
    *                   1. 三通道RGB格式
    *                   2. 尺寸不小于WINDOW_SIZE × WINDOW_SIZE
    * 
    * @return std::pair包含：
    *         1. first：有效patch集合
    *         2. second：对应patch的左上角坐标
    */
    std::pair<std::vector<cv::Mat>, std::vector<cv::Point>> extract_patches(const cv::Mat& rgb_image);

    /**
    * @brief 通过统计patch中有效像素点数来判断patch是否有效
    * 
    * @param[in] patch 输入patch
    * 
    * @return true表示patch有效，false表示patch无效
    */
    bool patch_filter(const cv::Mat& patch); //检查patch是否是空白背景

    /**
    * @brief 对patch进行预处理: 
    *        1. resize到224×224
    *        2. normalization
    *
    * @param[inout] image 输入patch
    * 
    * @return 预处理后的patch
    */
    cv::Mat ctranspath_preprocess(const cv::Mat& image);

    /**
    * @brief 对输入的patches提取特征，每个patch的特征维度为768
    * 
    * @param[inout] patches 输入patches
    * 
    * @return 输入patches的特征
    */
    std::vector<float> embedding(std::vector<cv::Mat>& patches);

    /**
    * @brief 对输入的图片特征进行分类，得到肠道疾病流分类置信度
    * 
    * @param[in] features 输入图片特征
    * 
    * @return std::pair<std::array<float, size_t(6)>, int>，包括：
    *         1. first：疾病流分类置信度
    *         2. second：关键patch的index
    */
    std::pair<std::vector<float>, int> image_predict(std::vector<float>& features, const std::string& slice_part);

    //std::pair<std::array<float, size_t(6)>, int> video_predict(std::vector<float>& features);

    /**
     * @brief MMR预测函数，只在slide预测为C时调用
     * 
     * @param features 10、20、40倍率图像的特征集合
     * 
     * @return 返回MLH1,MSH2,MSH6,PMS2四种基因的二分类预测结果
     */
    std::array<float, 4> mmr_predict(const std::vector<float>& features);


    // ================================ 公共成员变量 ================================

    static int m_show_model_debug_info;      //是否打印模型推理时的信息，从config.ini配置文件中读取

private:
    // ================================ 私有成员变量 ================================

    std::vector<std::string> cls_name;    //类别

    std::unique_ptr<OrtComponents> embedding_ort_components;
    std::unique_ptr<OrtComponents> polyp_ort_components;
    std::unique_ptr<OrtComponents> mmr_inference_ort_components;
    std::unique_ptr<OrtComponents> frozen_ort_components;

};

// 可视化分类结果的工具
class barChartWidget : public QWidget {
public:
    // ================================ 构造/析构函数 ================================
    explicit barChartWidget(QWidget* parent = nullptr);

    // ================================ 公共成员函数 ================================
    /**
    * @brief 为logits设置类别和置信度用于显示
    * 
    * @param[in] confidences 输入置信度
    * @param[in] classNames 输入类别
    */
    void setChartData(const QVector<float>& confidences, std::vector<std::string>& classNames);

protected:
    // 柱状图（竖向）  ！请勿删除！
    //void paintEvent(QPaintEvent* event) override {
    //    QPainter painter(this);
    //    painter.setRenderHint(QPainter::Antialiasing); // 设置抗锯齿

    //    int barWidth = width() / (confidences.size() + 1); // 每个柱状图的宽度
    //    int barSpacing = barWidth / 10; // 柱状图之间的间距

    //    std::vector<QColor> hist_color = { QColor("#003F5C"),QColor("#58508D"),QColor("#636E2C"),QColor("#BC4749"),QColor("#93A1A1"),QColor("#D25252") };

    //    for (int i = 0; i < confidences.size(); ++i) {
    //        // 绘制柱状图
    //        //QRect barRect(i * (barWidth + barSpacing), height() * (1 - confidences[i] - 0.1), barWidth, height() * confidences[i]);
    //        QRect barRect(barSpacing + i * (barWidth + barSpacing), 0.1*height()+0.8*height()*(1-confidences[i]), barWidth, height() *0.8* confidences[i]);
    //        QBrush barBrush(hist_color[i]); // 柱状图的颜色
    //        painter.fillRect(barRect, barBrush);

    //        // 绘制分类名称
    //        QString item_name = QString::fromStdString(classNames[i]);
    //        //painter.drawText(i * (barWidth + barSpacing) + barWidth / 2, height() * 0.1, item_name);
    //        painter.drawText(barSpacing + i * (barWidth + barSpacing) + barWidth / 4, height()*0.97, item_name);

    //        // 绘制置信度
    //        QString confidenceText = QString::number(confidences[i], 'f', 3);
    //        //painter.drawText(i * (barWidth + barSpacing) + barWidth / 2, height() * (1 - 0.1), confidenceText);
    //        painter.drawText(barSpacing + i * (barWidth + barSpacing), 0.07 * height()+0.8 * height() * (1 - confidences[i]), confidenceText);
    //    }
    //}

    // 柱状图（横向）
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing); // 设置抗锯齿

        int barWidth = 10;   // 每个柱状图的宽度
        int barSpacing = 18; // 柱状图之间的间距
        int barMargin = (height() - confidences.size() * barWidth - (confidences.size() - 1) * barSpacing) / 2;

        std::vector<QColor> hist_color = { QColor("#003F5C"),QColor("#58508D"),QColor("#636E2C"),QColor("#BC4749"),QColor("#93A1A1"),QColor("#D25252") };

        for (int i = 0; i < confidences.size(); ++i) {
            // 绘制柱状图
            //QRect(x, y, width, height)
            //QRect barRect(0.03 * width() + 0.13 * width(), barSpacing + i * (barWidth + barSpacing) + 0.05 * height() + 0.4 * barWidth, width() * 0.65 * confidences[i], barWidth);
            QRect barRect(0.03 * width() + 0.13 * width(), barMargin + i * (barWidth + barSpacing), width() * 0.65 * confidences[i], barWidth);
            QBrush barBrush(hist_color[i]); // 柱状图的颜色
            painter.fillRect(barRect, barBrush);

            // 绘制分类名称
            QString item_name = QString::fromStdString(classNames[i]);
            //painter.drawText(0.02 * width(), barSpacing + i * (barWidth + barSpacing) + 0.11 * height(), item_name);
            painter.drawText(0.02 * width(), barMargin + i * (barWidth + barSpacing) + 0.05 * height(), item_name);

            // 绘制置信度数字
            QString confidenceText = QString::number(confidences[i], 'f', 3);
            //painter.drawText(i * (barWidth + barSpacing) + barWidth / 2, height() * (1 - 0.1), confidenceText);
            painter.drawText(0.03 * width() + 0.13 * width() + width() * 0.65 * confidences[i], barMargin + i * (barWidth + barSpacing) + 0.05 * height(), confidenceText);
        }
    }

private:
    QVector<float> confidences;
    std::vector<std::string> classNames;
};

