#pragma once
#include <QPainter>
#include <QString>
#include <QWidget>
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

/*OnnxRuntime设置*/
#define INTRA_OP_NUM_THREADS 4

/*Onnx AI模型路径*/
#define ONNX_CTRANS_PATH L"../../../assets/onnx_model/ctranspath_v14_batch28.onnx" // 特征提取基础模型
#define ONNX_POLYP_PATH L"../../../assets/onnx_model/AMIL_1219_blur_v14.onnx"      // 肠息肉分类预测模型路径
#define ONNX_FROZEN_PATH L"../../../assets/onnx_model/frozen_cancer.onnx"          // 冷冻切片。目前仅支持癌症筛查
#define ONNX_MMR_PATH L"../../../assets/onnx_model/MMR.onnx"                       // MMR基因预测，针对肠息肉切片
#define ONNX_LUNG_PATH L"../../../assets/onnx_model/Lung.onnx"                     // 肺分类预测模型路径
#define ONNX_LYMPHNODE_PATH L"../../../assets/onnx_model/LymphNode.onnx"           // 淋巴结分类预测模型路径

/*裁剪patch参数*/
#define WINDOW_SIZE 454  // 裁剪patch时的窗长
#define STRIDE 227       // 裁剪patch时的步长
#define MAX_PATCH_NUM 28 // 最大裁剪patch数量
#define EMBEDDING_DIM 768

/*图像筛选参数*/
#define BINARY_THRESHOLD 200 // patchFilter()函数的参数，用于过滤二值图像
#define VAILD_THRESHOLD 0.03 // patchFilter()函数的参数，用于判断patch是否有效

/*切片来源定义*/
#define SLICESOURCE_STOMACH "2000"   // 胃
#define SLICESOURCE_GUT "2010"       // 肠
#define SLICESOURCE_PROSTATE "2020"  // 前列腺
#define SLICESOURCE_LUNG "1140"      // 肺
#define SLICESOURCE_LYMPHNODE "1160" // 淋巴结
#define SLICESOURCE_UNKNOWN "1000"   // 未知,暂时当冰冻切片使用
#define SLICESOURCE_DEFAULT "1000"   // 均不属于上面定义的来源类型时，默认使用癌和非癌分类模型

/*定义疾病类型*/
#define CLSNAME_STOMACH {"NM", "C"}
#define CLSNAME_GUT {"C", "SSA", "TA", "HP", "IP", "NM"}
#define CLSNAME_PROSTATE {"NM", "C"}
#define CLSNAME_LUNG {"NM", "OP", "SCC", "AIS", "MIA", "AC"}
#define CLSNAME_LYMPHNODE {"N", "P"}
#define CLSNAME_KNOWN {"NM", "C"}
#define CLSNAME_DEFAULT {"NM", "C"}

/**
 * @brief AI诊断结果结构体
 */
struct AIResult
{
    QString diagnosis_res;   // 疾病诊断结果，例如：“C”
    float diagnosis_prob;    // 疾病诊断概率，例如：0.918
    QVector<float> cls_prob; // 疾病分类分布
    cv::Point ROI;           // ROI坐标

    // 构造函数
    AIResult() {};
    AIResult(QString res, float prob, QVector<float> distribution, cv::Point coords)
        : diagnosis_res(res)
        , diagnosis_prob(prob)
        , cls_prob(distribution)
        , ROI(coords) {};

    /**
     * @brief 设置AI诊断结果
     *
     * @param res 诊断结果
     * @param prob 诊断概率
     * @param distribution 分类分布
     * @param coords ROI坐标
     */
    void setValue(const QString res, const float prob, const QVector<float> distribution, const cv::Point coords)
    {
        diagnosis_res = res;
        diagnosis_prob = prob;
        cls_prob = distribution;
        ROI = coords;
    };
};

struct OrtComponents
{
public:
    /**
     * @brief Onnxruntime组件构造函数
     *
     * @param envName onnxruntime环境名称
     * @param modelPath onnxruntime模型路径
     * @param intraOpThreadsNum 内部运算所使用的线程数
     */
    OrtComponents(const char* envName, const wchar_t* modelPath, int intraOpThreadsNum);

    std::unique_ptr<Ort::Session> session;      // onnxruntime会话
    OrtMemoryInfo* memoryInfo;                  // onnxruntime内存信息
    Ort::AllocatorWithDefaultOptions allocator; // onnxruntime内存分配器
    size_t inputNodesSize;                      // 输入节点数
    std::vector<const char*> inputNodesNames;   // 输入节点名称
    size_t outputNodesSize;                     // 输出节点数
    std::vector<const char*> outputNodesNames;  // 输出节点名称

private:
    Ort::Env env;                                           // onnxruntime环境
    Ort::SessionOptions sessionOptions;                     // onnxruntime会话设置器
    std::vector<Ort::AllocatedStringPtr> inputNamesHolder;  // 输入节点名称指针holder
    std::vector<Ort::AllocatedStringPtr> outputNamesHolder; // 输出节点名称指针holder
};

class OnnxDeployer
{
public:
    // ================================ 构造/析构函数 ================================
    explicit OnnxDeployer();
    ~OnnxDeployer();

    // ================================ 公共成员函数 ================================
    /**
     * @brief 从输入图像中提取有效图像块（patch）及其位置
     *
     * @param rgbImage 输入图像，需满足：
     *                   1. 三通道RGB格式
     *                   2. 尺寸不小于WINDOW_SIZE × WINDOW_SIZE
     *
     * @return std::pair包含：
     *         1. first：有效patch集合
     *         2. second：对应patch的左上角坐标
     */
    std::pair<std::vector<cv::Mat>, std::vector<cv::Point>> extractPatches(const cv::Mat& rgbImage);

    /**
     * @brief 对输入的patches提取特征，每个patch的特征维度为768
     *
     * @param patches 输入patches
     *
     * @return 输入patches的特征
     */
    std::vector<float> embedding(const std::vector<cv::Mat>& patches);

    /**
     * @brief 对输入的图片特征进行分类，得到肠道疾病流分类置信度
     *
     * @param features 输入图片特征
     *
     * @return std::pair<std::array<float, size_t(6)>, int>，包括：
     *         1. first：疾病6分类置信度
     *         2. second：关键patch的index
     */
    std::pair<std::vector<float>, int> inference(const std::vector<float>& features, const std::string& slicePart);

    /**
     * @brief MMR预测函数，只在slide预测为C时调用
     *
     * @param features 10、20、40倍率图像的特征集合
     *
     * @return 返回MLH1,MSH2,MSH6,PMS2四种基因的二分类预测结果
     */
    std::array<float, 4> mmr_predict(const std::vector<float>& features);

private:
    // ================================ 私有成员变量 ================================

    /**
     * @brief 通过统计patch中有效像素点数来判断patch是否有效
     *
     * @param patch 输入patch
     *
     * @return true表示patch有效，false表示patch无效
     */
    bool patchFilter(const cv::Mat& patch);

    /**
     * @brief 对patch进行预处理:
     *        1. resize到224×224
     *        2. normalization
     *
     * @param[inout] image 输入patch
     *
     * @return 预处理后的patch
     */
    cv::Mat preprocess(const cv::Mat& image);

    std::unordered_map<std::string, OrtComponents*> slicePartToOrtComponentsMap;

    std::unique_ptr<OrtComponents> embeddingOrtComponents;
    std::unique_ptr<OrtComponents> polypOrtComponents;
    std::unique_ptr<OrtComponents> lungOrtComponents;
    std::unique_ptr<OrtComponents> lymphNodeOrtComponents;
    std::unique_ptr<OrtComponents> mmrOrtComponents;
    std::unique_ptr<OrtComponents> frozenSectionOrtComponents;
};

// 可视化分类结果的工具
class barChartWidget : public QWidget
{
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
    // void paintEvent(QPaintEvent* event) override {
    //    QPainter painter(this);
    //    painter.setRenderHint(QPainter::Antialiasing); // 设置抗锯齿

    //    int barWidth = width() / (confidences.size() + 1); // 每个柱状图的宽度
    //    int barSpacing = barWidth / 10; // 柱状图之间的间距

    //    std::vector<QColor> hist_color = {
    //    QColor("#003F5C"),QColor("#58508D"),QColor("#636E2C"),QColor("#BC4749"),QColor("#93A1A1"),QColor("#D25252") };

    //    for (int i = 0; i < confidences.size(); ++i) {
    //        // 绘制柱状图
    //        //QRect barRect(i * (barWidth + barSpacing), height() * (1 - confidences[i] - 0.1), barWidth, height() *
    //        confidences[i]); QRect barRect(barSpacing + i * (barWidth + barSpacing),
    //        0.1*height()+0.8*height()*(1-confidences[i]), barWidth, height() *0.8* confidences[i]); QBrush
    //        barBrush(hist_color[i]); // 柱状图的颜色 painter.fillRect(barRect, barBrush);

    //        // 绘制分类名称
    //        QString item_name = QString::fromStdString(classNames[i]);
    //        //painter.drawText(i * (barWidth + barSpacing) + barWidth / 2, height() * 0.1, item_name);
    //        painter.drawText(barSpacing + i * (barWidth + barSpacing) + barWidth / 4, height()*0.97, item_name);

    //        // 绘制置信度
    //        QString confidenceText = QString::number(confidences[i], 'f', 3);
    //        //painter.drawText(i * (barWidth + barSpacing) + barWidth / 2, height() * (1 - 0.1), confidenceText);
    //        painter.drawText(barSpacing + i * (barWidth + barSpacing), 0.07 * height()+0.8 * height() * (1 -
    //        confidences[i]), confidenceText);
    //    }
    //}

    // 柱状图（横向）
    void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing); // 设置抗锯齿

        int barWidth = 10;   // 每个柱状图的宽度
        int barSpacing = 18; // 柱状图之间的间距
        int barMargin = (height() - confidences.size() * barWidth - (confidences.size() - 1) * barSpacing) / 2;

        std::vector<QColor> hist_color = {QColor("#003F5C"), QColor("#58508D"), QColor("#636E2C"),
                                          QColor("#BC4749"), QColor("#93A1A1"), QColor("#D25252")};

        for (int i = 0; i < confidences.size(); ++i)
        {
            // 绘制柱状图
            // QRect(x, y, width, height)
            // QRect barRect(0.03 * width() + 0.13 * width(), barSpacing + i * (barWidth + barSpacing) + 0.05 * height()
            // + 0.4 * barWidth, width() * 0.65 * confidences[i], barWidth);
            QRect barRect(0.03 * width() + 0.13 * width(), barMargin + i * (barWidth + barSpacing),
                          width() * 0.65 * confidences[i], barWidth);
            QBrush barBrush(hist_color[i]); // 柱状图的颜色
            painter.fillRect(barRect, barBrush);

            // 绘制分类名称
            QString item_name = QString::fromStdString(classNames[i]);
            // painter.drawText(0.02 * width(), barSpacing + i * (barWidth + barSpacing) + 0.11 * height(), item_name);
            painter.drawText(0.02 * width(), barMargin + i * (barWidth + barSpacing) + 0.05 * height(), item_name);

            // 绘制置信度数字
            QString confidenceText = QString::number(confidences[i], 'f', 3);
            // painter.drawText(i * (barWidth + barSpacing) + barWidth / 2, height() * (1 - 0.1), confidenceText);
            painter.drawText(0.03 * width() + 0.13 * width() + width() * 0.65 * confidences[i],
                             barMargin + i * (barWidth + barSpacing) + 0.05 * height(), confidenceText);
        }
    }

private:
    QVector<float> confidences;
    std::vector<std::string> classNames;
};
