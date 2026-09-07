#include "OnnxDeployer.h"

#include "Logger.h"

OrtComponents::OrtComponents(const char* envName, const wchar_t* modelPath, int intraOpThreadsNum)
{
    env = Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_ERROR, envName);

    memoryInfo = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    sessionOptions.SetIntraOpNumThreads(intraOpThreadsNum);

    session = std::make_unique<Ort::Session>(env, modelPath, sessionOptions);

    // 动态获取输入/输出节点名称
    inputNodesSize = session->GetInputCount();
    inputNodesNames.reserve(inputNodesSize);
    inputNamesHolder.reserve(inputNodesSize);

    outputNodesSize = session->GetOutputCount();
    outputNodesNames.reserve(outputNodesSize);
    outputNamesHolder.reserve(outputNodesSize);

    for (size_t i = 0; i < inputNodesSize; i++)
    {
        inputNamesHolder.push_back(session->GetInputNameAllocated(i, allocator));
        inputNodesNames.push_back(inputNamesHolder.back().get());
    }
    for (size_t i = 0; i < outputNodesSize; i++)
    {
        outputNamesHolder.push_back(session->GetOutputNameAllocated(i, allocator));
        outputNodesNames.push_back(outputNamesHolder.back().get());
    }
}

OnnxDeployer::OnnxDeployer()
    : embeddingOrtComponents(std::make_unique<OrtComponents>("embedding", ONNX_CTRANS_PATH, INTRA_OP_NUM_THREADS))
    , polypOrtComponents(std::make_unique<OrtComponents>("polypInference", ONNX_POLYP_PATH, INTRA_OP_NUM_THREADS))
    , lungOrtComponents(std::make_unique<OrtComponents>("lungInference", ONNX_LUNG_PATH, INTRA_OP_NUM_THREADS))
    , lymphNodeOrtComponents(
          std::make_unique<OrtComponents>("lymphNodeInference", ONNX_LYMPHNODE_PATH, INTRA_OP_NUM_THREADS))
    , mmrOrtComponents(std::make_unique<OrtComponents>("mmrInference", ONNX_MMR_PATH, INTRA_OP_NUM_THREADS))
    , frozenSectionOrtComponents(
          std::make_unique<OrtComponents>("frozenSectionInference", ONNX_FROZEN_PATH, INTRA_OP_NUM_THREADS))
{
    slicePartToOrtComponentsMap = {{SLICESOURCE_GUT, polypOrtComponents.get()},
                                   {SLICESOURCE_LUNG, lungOrtComponents.get()},
                                   {SLICESOURCE_LYMPHNODE, lymphNodeOrtComponents.get()},
                                   {SLICESOURCE_UNKNOWN, frozenSectionOrtComponents.get()},
                                   {SLICESOURCE_STOMACH, frozenSectionOrtComponents.get()},
                                   {SLICESOURCE_PROSTATE, frozenSectionOrtComponents.get()}};
}

OnnxDeployer::~OnnxDeployer() {}

std::pair<std::vector<cv::Mat>, std::vector<cv::Point>> OnnxDeployer::extractPatches(const cv::Mat& rgbImage)
{
    std::vector<cv::Mat> patches; // patch.size()表示里面有多少个Mat元素。patch[i]表示查看第几个Mat元素
    std::vector<cv::Point> coords;
    patches.reserve(MAX_PATCH_NUM);
    coords.reserve(MAX_PATCH_NUM);

    // 预先计算边界
    int maxX = rgbImage.cols - WINDOW_SIZE;
    int maxY = rgbImage.rows - WINDOW_SIZE;

    for (int x = 0; x <= maxX; x += STRIDE)
    {
        for (int y = 0; y <= maxY; y += STRIDE)
        {
            cv::Rect rect(x, y, WINDOW_SIZE, WINDOW_SIZE); // cv:Rect 是opencv库中矩形区域的表示

            cv::Mat patch = rgbImage(rect);

            // 检查patch是否有效
            if (patchFilter(patch))
            {
                patches.push_back(preprocess(patch));
                coords.emplace_back(cv::Point(x, y));
            }
        }
    }

    return {patches, coords};
}

bool OnnxDeployer::patchFilter(const cv::Mat& patch)
{
    cv::Mat grayImage;
    cv::cvtColor(patch, grayImage, cv::COLOR_RGB2GRAY); // 将RGB图转换为灰度图
    cv::Mat binaryMask;
    cv::threshold(grayImage, binaryMask, BINARY_THRESHOLD, 255, cv::THRESH_BINARY_INV);
    int nonZeroCount = cv::countNonZero(binaryMask);
    float coloredRatio = static_cast<float>(nonZeroCount) / (binaryMask.rows * binaryMask.cols);
    return coloredRatio > VAILD_THRESHOLD;
}

cv::Mat OnnxDeployer::preprocess(const cv::Mat& image)
{
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(224, 224), 0, 0, cv::INTER_CUBIC); // 缩放图像

    cv::Mat normalized;
    resized.convertTo(normalized, CV_32FC3, 1.0 / 255.0); // 转换图像精度并归一化。CV_32FC3表示32位浮点数3通道图像

    std::vector<float> mean = {0.485, 0.456, 0.406};
    std::vector<float> std = {0.229, 0.224, 0.225};

    cv::Mat result;

    std::vector<cv::Mat> channels(normalized.channels());
    cv::split(normalized, channels); // 分割通道

    for (int i = 0; i < normalized.channels(); ++i)
    {
        channels[i] = (channels[i] - mean[i]) / std[i]; // 对第i个通道减去均值并除以标准差
    }
    cv::merge(channels, result); // 合并通道

    return result;
}

std::vector<float> OnnxDeployer::embedding(const std::vector<cv::Mat>& patches)
{
    const int validPatchSize = patches.size();

    // 定义输入张量
    int64_t input_h = 224;
    int64_t input_w = 224;
    int64_t channels = 3;

    // 使用空白patch对输入patches进行补齐，补至最大长度
    std::vector<float> tensorInput(MAX_PATCH_NUM * channels * input_h * input_w, 0.0f);

    // Opencv的内存布局默认是HWC，而Onnx Runtime的输入tensor的内存布局是NCHW，因此需要进行转换
    // HWC：内存按行排布，每行中每个像素点按通道存储
    // NCHW：内存按通道排布，每个通道中按行存储
    // 原始版本
    //    size_t counter = 0;
    // for (int64_t n = 0; n < validPatchSize; n++)
    // {
    //     for (int64_t k = 0; k < channels; k++)
    //     {
    //         for (int64_t i = 0; i < input_h; i++)
    //         {
    //             for (int64_t j = 0; j < input_w; j++)
    //             {
    //                 tensorInput[counter++] = static_cast<float>(patches[n].at<cv::Vec3f>(i, j)[k]);
    //             }
    //         }
    //     }
    // }
    // 优化版本
    for (int n = 0; n < validPatchSize; n++)
    {
        const float* srcPtr = patches[n].ptr<float>(0); // 获取第n个patch的首地址

        float* dstBase =
            tensorInput.data() + n * channels * input_h * input_w; // 计算第n个patch在model_input中对应的起始位置

        float* dstR = dstBase;                         // R通道的目标位置
        float* dstG = dstBase + input_h * input_w;     // G通道的目标位置
        float* dstB = dstBase + 2 * input_h * input_w; // B通道的目标位置

        // 遍历所有像素
        for (int i = 0; i < input_h * input_w; i++)
        {
            dstR[i] = srcPtr[i * 3 + 0]; // 复制R通道数据
            dstG[i] = srcPtr[i * 3 + 1]; // 复制G通道数据
            dstB[i] = srcPtr[i * 3 + 2]; // 复制B通道数据
        }
    }

    std::array<int64_t, 4> tensorInputShape = {MAX_PATCH_NUM, channels, input_h, input_w}; // N * C * H * W

    size_t tensorInputShapeLen = tensorInputShape.size();

    Ort::Value input =
        Ort::Value::CreateTensor<float>(embeddingOrtComponents->memoryInfo, tensorInput.data(), tensorInput.size(),
                                        tensorInputShape.data(), tensorInputShapeLen);
    std::vector<Ort::Value> output_tensor =
        embeddingOrtComponents->session->Run(Ort::RunOptions{nullptr},
                                             embeddingOrtComponents->inputNodesNames.data(),  // 输入节点名
                                             &input,                                          // 输入张量
                                             embeddingOrtComponents->inputNodesSize,          // 输入节点数量
                                             embeddingOrtComponents->outputNodesNames.data(), // 输出节点名
                                             embeddingOrtComponents->outputNodesSize);        // 输出节点数量

    float* output = output_tensor[0].GetTensorMutableData<float>(); // 返回的是输出结果的第一个数据的地址

    // 保存输出结果
    std::vector<float> embedding(output, output + validPatchSize * EMBEDDING_DIM);

    return embedding;
}

std::pair<std::vector<float>, int> OnnxDeployer::inference(const std::vector<float>& features,
                                                           const std::string& slicePart)
{
    OrtComponents* ort_components = nullptr;
    // if (slicePart == SLICESOURCE_GUT)
    // {
    //     ort_components = polypOrtComponents.get();
    // }
    // else if (slicePart == SLICESOURCE_UNKNOWN)
    // {
    //     ort_components = frozenSectionOrtComponents.get();
    // }
    // else if (slicePart == SLICESOURCE_STOMACH)
    // {
    //     ort_components = frozenSectionOrtComponents.get();
    // }
    // else if (slicePart == SLICESOURCE_PROSTATE)
    // {
    //     ort_components = frozenSectionOrtComponents.get();
    // }
    // else
    // {
    //     ort_components = frozenSectionOrtComponents.get();
    // }
    auto it = slicePartToOrtComponentsMap.find(slicePart);
    if (it != slicePartToOrtComponentsMap.end())
    {
        ort_components = it->second;
    }
    else
    {
        ort_components = frozenSectionOrtComponents.get();
    }

    // 定义输入张量
    int64_t input_h = features.size() / EMBEDDING_DIM;
    int64_t input_w = EMBEDDING_DIM;
    int64_t frame_num = 1;

    std::array<int64_t, 3> modelInputShape = {frame_num, input_h, input_w};
    size_t modelInputSize = modelInputShape.size();

    Ort::Value input_tensor =
        Ort::Value::CreateTensor<float>(ort_components->allocator, modelInputShape.data(), modelInputSize);

    memcpy(input_tensor.GetTensorMutableData<float>(), features.data(), features.size() * sizeof(float));
    // Ort::Value input_tensor =
    //     Ort::Value::CreateTensor<float>(ort_components->memoryInfo, const_cast<float*>(features.data()),
    //                                     features.size(), modelInputShape.data(), modelInputSize);

    std::vector<Ort::Value> output_tensor = ort_components->session->Run(
        Ort::RunOptions{nullptr}, ort_components->inputNodesNames.data(), &input_tensor, ort_components->inputNodesSize,
        ort_components->outputNodesNames.data(), ort_components->outputNodesSize);

    float* output = output_tensor[0].GetTensorMutableData<float>();
    float* attentionScores = output_tensor[1].GetTensorMutableData<float>(); // 输出的每个patch的分数

    // 搜索ROI
    int maxScoreIndex = 0;
    float maxScore = -1;
    for (int i = 0; i < input_h; i++)
    {
        if (attentionScores[i] > maxScore)
        {
            maxScore = attentionScores[i];
            maxScoreIndex = i;
        }
    }

    // 处理AI模型logits
    // 通过session获取shape，从而得到分类数
    auto classTypeInfo = ort_components->session->GetOutputTypeInfo(0); // 默认输出节点的第0个tensor是分类
    auto classTensorInfo = classTypeInfo.GetTensorTypeAndShapeInfo();
    std::vector<int64_t> classTensorShape = classTensorInfo.GetShape();
    const int64_t classNum = classTensorShape[1];

    // 分配并初始化存放未归一化置信度的向量，保证通过 operator[] 访问安全
    std::vector<float> softmax_vec(classNum);

    // 搜索最大分类概率和最大概率对应的下表
    float maxLogits = -1;
    for (int i = 0; i < classNum; ++i)
    {
        // 因为 softmax_vec 已按 classNum 大小分配，使用下标赋值而不是 push_back
        softmax_vec[i] = output[i]; // 将未归一化的置信度复制到数组

        if (output[i] > maxLogits)
        {
            maxLogits = output[i];
        }
    }

    // 计算e的指数和
    float sum = 0;
    for (int i = 0; i < classNum; ++i)
    {
        sum += std::exp(softmax_vec[i] - maxLogits); // 减去最大值提高数值稳定性
    }

    // 归一化
    for (int i = 0; i < classNum; ++i)
    {
        softmax_vec[i] = std::exp(softmax_vec[i] - maxLogits) / sum;
    }

    return {softmax_vec, maxScoreIndex};
}

std::array<float, 4> OnnxDeployer::mmr_predict(const std::vector<float>& features)
{
    // 定义输入张量的形状
    int64_t input_h = features.size() / 768;
    int64_t input_w = 768;
    int64_t video_num = 1;
    std::array<int64_t, 3> model_input_shape = {video_num, input_h, input_w};
    size_t model_input_size = model_input_shape.size();

    // 定义输入张量
    std::vector<float> model_input = features;

    Ort::Value input_tensor =
        Ort::Value::CreateTensor<float>(mmrOrtComponents->memoryInfo, model_input.data(), model_input.size(),
                                        model_input_shape.data(), model_input_size);

    // MMR模型的输出shape是（1，4，2）
    std::vector<Ort::Value> output_tensor = mmrOrtComponents->session->Run(
        Ort::RunOptions{nullptr}, mmrOrtComponents->inputNodesNames.data(), &input_tensor,
        mmrOrtComponents->inputNodesSize, mmrOrtComponents->outputNodesNames.data(), mmrOrtComponents->outputNodesSize);

    std::array<float, 4> result;
    // softmax
    for (int i = 0; i < 4; i++)
    {
        float p1 = output_tensor[0].GetTensorMutableData<float>()[i * 2];
        float p2 = output_tensor[0].GetTensorMutableData<float>()[i * 2 + 1];

        float max_val = (p1 > p2) ? p1 : p2;
        float p1_e = std::exp(p1 - max_val);
        float p2_e = std::exp(p2 - max_val);
        float sum = p1_e + p2_e;
        result[i] = std::round(p2_e / sum * 1000.0f) / 1000.0f;
    }

    return result;
}

barChartWidget::barChartWidget(QWidget* parent)
    : QWidget(parent)
{
    this->confidences = {0, 0, 0, 0, 0, 0};
    this->classNames = {"NA", "NA", "NA", "NA", "NA", "NA"};
    this->setChartData(this->confidences, this->classNames);
}

void barChartWidget::setChartData(const QVector<float>& confidences, std::vector<std::string>& classNames)
{
    this->confidences = confidences;
    this->classNames = classNames;
    update(); // 触发重绘
}
