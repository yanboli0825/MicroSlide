#include "deployment.h"
#include <iostream>
#include <onnxruntime_cxx_api.h>



// 定义静态变量
int deployment::m_show_model_debug_info = 0;

struct OrtComponents {
public:
    /**
    * @brief Onnxruntime组件构造函数
    *
    * @param[in] env_name onnxruntime环境名称
    * @param[in] model_path onnxruntime模型路径
    * @param[in] intra_op_num_threads 内部运算所使用的线程数
    */
    OrtComponents(const char* env_name, const wchar_t* model_path, int intra_op_num_threads);

    std::unique_ptr<Ort::Session> session;                      // onnxruntime会话
    OrtMemoryInfo* memory_info;                                 // onnxruntime内存信息
    size_t input_nodes_size;                                    // 输入节点数
    std::vector<const char*> input_nodes_names;                 // 输入节点名称
    size_t output_nodes_size;                                   // 输出节点数
    std::vector<const char*> output_nodes_names;                // 输出节点名称

private:
    Ort::Env env;                                               // onnxruntime环境
    Ort::SessionOptions session_options;                        // onnxruntime会话设置器
    Ort::AllocatorWithDefaultOptions allocator;                 // onnxruntime内存分配器
    std::vector<Ort::AllocatedStringPtr> input_names_holder;    // 输入节点名称指针holder，防止指针在构造函数中析构产生野指针
    std::vector<Ort::AllocatedStringPtr> output_names_holder;   // 输出节点名称指针holder，防止指针在构造函数中析构产生野指针
};

OrtComponents::OrtComponents(const char* env_name, const wchar_t* model_path, int intra_op_num_threads)
{

    env = Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_ERROR, env_name);

    memory_info = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    session_options.SetIntraOpNumThreads(intra_op_num_threads);

    session = std::make_unique<Ort::Session>(env, model_path, session_options);

    // 动态获取输入/输出节点名称
    input_nodes_size = session->GetInputCount();
    input_nodes_names.reserve(input_nodes_size);
    input_names_holder.reserve(input_nodes_size);

    output_nodes_size = session->GetOutputCount();
    output_nodes_names.reserve(output_nodes_size);
    output_names_holder.reserve(output_nodes_size);

    for (size_t i = 0; i < input_nodes_size; i++) {
        input_names_holder.push_back(session->GetInputNameAllocated(i, allocator));
        input_nodes_names.push_back(input_names_holder.back().get());
    }
    for (size_t i = 0; i < output_nodes_size; i++) {
        output_names_holder.push_back(session->GetOutputNameAllocated(i, allocator));
        output_nodes_names.push_back(output_names_holder.back().get());
    }
}


deployment::deployment() :
    cls_name(POLYP_CLASSES),
    embedding_ort_components(std::make_unique<OrtComponents>("embedding", ONNX_CTRANS_PATH, INTRA_OP_NUM_THREADS)),
    polyp_ort_components(std::make_unique< OrtComponents>("polyp_inference", ONNX_POLYP_PATH, INTRA_OP_NUM_THREADS)),
    mmr_inference_ort_components(std::make_unique<OrtComponents>("mmr_inference", ONNX_MMR_PATH, INTRA_OP_NUM_THREADS)),
    frozen_ort_components(std::make_unique<OrtComponents>("frozen_inference", ONNX_FROZEN_PATH, INTRA_OP_NUM_THREADS))
{

}


deployment::~deployment()
{
   
}


std::pair<std::vector<cv::Mat>, std::vector<cv::Point>> deployment::extract_patches(const cv::Mat& rgb_image)
{
    if (m_show_model_debug_info == 1) {
        std::cout << "-------------------------EXTRACT_PATCHES-----------------------------" << std::endl;
    }


    std::vector<cv::Mat> patches; //patch.size()表示里面有多少个Mat元素   patch[i]表示查看第几个Mat元素
    std::vector<cv::Point> coords;


    for (int x = 0; x < rgb_image.cols; x += STRIDE) {
        for (int y = 0; y < rgb_image.rows; y += STRIDE) {
            cv::Rect rect(x, y, WINDOW_SIZE, WINDOW_SIZE); //cv:Rect 是opencv库中矩形区域的表示
            //若patch超出了图片的行或列，则跳出本轮循环
            if (rect.x + rect.width > rgb_image.cols || rect.y + rect.height > rgb_image.rows) {
                continue;
            }

            cv::Mat patch = rgb_image(rect).clone(); //获取小patch

            //检查patch是否是空白背景
            if (patch_filter(patch)) { 
                patches.push_back(ctranspath_preprocess(patch));
                //patches.push_back(patch);
                coords.push_back(cv::Point(x, y));

            }
        }
    }

    if (m_show_model_debug_info == 1) {
        std::cout << "-------------------------EXTRACT_PATCHES_OVER-----------------------------" << std::endl;
    }

    return { patches, coords };
    //std::cout << "DEBUG: " << a.second << std::endl;
}

//检查patch是否是空白背景
bool deployment::patch_filter(const cv::Mat& patch) {
    //std::cout << "Execute patch check: if the current patch is mostly blank?" << std::endl;
    cv::Mat gray_image;
    cv::cvtColor(patch, gray_image, cv::COLOR_RGB2GRAY); //将RGB图转换为灰度图
    cv::Mat binary_image;
    cv::threshold(gray_image, binary_image, BINARY_THRESHOLD, 255, cv::THRESH_BINARY_INV);
    int non_zero_count = cv::countNonZero(binary_image);
    float colored_ratio = static_cast<float>(non_zero_count) / (binary_image.rows * binary_image.cols);
    return colored_ratio > VAILD_THRESHOLD;
}


cv::Mat deployment::ctranspath_preprocess(const cv::Mat& image) { 
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(224, 224), 0, 0, cv::INTER_CUBIC); //缩放图像

    cv::Mat normalized;
    resized.convertTo(normalized, CV_32FC3, 1.0 / 255.0); //转换图像精度并归一化。CV_32FC3表示32位浮点数3通道图像

    std::vector<double> mean = { 0.485, 0.456, 0.406 };
    std::vector<double> std = { 0.229, 0.224, 0.225 };

    cv::Mat ctrans_processed;
    std::vector<cv::Mat> channels(normalized.channels());
    cv::split(normalized, channels); // 分割通道  
    for (int i = 0; i < normalized.channels(); ++i) 
    {
        channels[i] = (channels[i] - mean[i]) / std[i]; // 对第i个通道减去均值并除以标准差
    }
    cv::merge(channels, ctrans_processed); // 合并通道

    return ctrans_processed;
}




std::vector<float> deployment::embedding(std::vector<cv::Mat>& patches) {
    if (m_show_model_debug_info == 1) {
        std::cout << "-------------------------EMBEDDING-----------------------------" << std::endl;
    }
    
    const int valid_patches_size = patches.size();

    // 对适用空白patch对输入patches进行补齐，补至最大长度
    cv::Mat blank_patch(WINDOW_SIZE, WINDOW_SIZE, CV_32FC3, cv::Scalar(0, 0, 0));
    for (int i = 0; i < (28 - valid_patches_size); i++)
    {
        patches.push_back(blank_patch);
    }
    

    // 定义输入张量
    int64_t input_h = 224;
    int64_t input_w = 224;
    int64_t channels = 3;
    int64_t patches_size = 28;

    std::vector<float> model_input(patches_size * channels * input_h * input_w);

    size_t counter = 0;

    for (int64_t n = 0; n < patches_size; n++)
    {
        for (int64_t k = 0; k < channels; k++)
        {
            for (int64_t i = 0; i < input_h; i++)
            {
                for (int64_t j = 0; j < input_w; j++)
                {
                    model_input[counter++] = static_cast<float>(patches[n].at<cv::Vec3f>(i, j)[k]);
                }
            }
        }
    }

    std::array<int64_t, 4> model_input_shape = { patches_size, channels, input_h, input_w };    // N * C * H * W

    size_t model_input_shape_len = model_input_shape.size();

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(embedding_ort_components->memory_info, 
                                                                model_input.data(), 
                                                                model_input.size(), 
                                                                model_input_shape.data(), 
                                                                model_input_shape_len); 

    std::vector<Ort::Value> output_tensor = embedding_ort_components->session->Run(Ort::RunOptions{ nullptr },
                                                                                    embedding_ort_components->input_nodes_names.data(),  // 输入节点名
                                                                                    &input_tensor,                                      // 输入张量
                                                                                    embedding_ort_components->input_nodes_size,          // 输入节点数量
                                                                                    embedding_ort_components->output_nodes_names.data(), // 输出节点名
                                                                                    embedding_ort_components->output_nodes_size);        // 输出节点数量

    float* output = output_tensor[0].GetTensorMutableData<float>(); //返回的是输出结果的第一个数据的地址

    //保存输出结果
    std::vector<float> embedding(output, output + valid_patches_size * 768);

    return embedding;
}    



std::pair<std::vector<float>, int> deployment::image_predict(std::vector<float>& features, const std::string& slice_part) {
    OrtComponents* ort_components;
    if (slice_part == SLICESOURCE_GUT) {
        ort_components = polyp_ort_components.get();
    }
    else if (slice_part == SLICESOURCE_UNKNOWN) {
        ort_components = frozen_ort_components.get();
    }
    else if (slice_part == SLICESOURCE_STOMACH) {
        ort_components = frozen_ort_components.get();
    }
    else if (slice_part == SLICESOURCE_PROSTATE) {
        ort_components = frozen_ort_components.get();
    }
    else {
        ort_components = frozen_ort_components.get();
    }

    // 定义输入张量 
    int64_t input_h = features.size() / 768;
    int64_t input_w = 768;
    int64_t frame_num = 1;

    std::vector<float> model_input = features;

    std::array<int64_t, 3> model_input_shape = { frame_num, input_h, input_w };
    size_t model_input_size = model_input_shape.size();

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(ort_components->memory_info,
                                                                model_input.data(), 
                                                                model_input.size(), 
                                                                model_input_shape.data(), 
                                                                model_input_size);

    std::vector<Ort::Value> output_tensor = ort_components->session->Run(Ort::RunOptions{ nullptr },
        ort_components->input_nodes_names.data(),
        &input_tensor, 
        ort_components->input_nodes_size,
        ort_components->output_nodes_names.data(),
        ort_components->output_nodes_size);

    float* output = output_tensor[0].GetTensorMutableData<float>();
    float* key_patch = output_tensor[1].GetTensorMutableData<float>();      //输出的每个patch的分数

    /*搜索ROI*/
    int key_patch_index = 0;
    float key_patch_score = -1;
    for (int i = 0; i < input_h; i++)
    {
        if (*(key_patch + i) > key_patch_score)
        {
            key_patch_score = *(key_patch + i);
            key_patch_index = i;
        }
    }

    // 若返回的关键patch的范围不对，则强行置零，保证程序安全
    if (key_patch_index < 0 || key_patch_index > 27) {
        key_patch_index = 0;
    }


    /*处理AI模型logits*/
    // 通过session获取shape，从而得到分类数
    auto class_type_info = ort_components->session->GetOutputTypeInfo(0);           // 默认输出节点的第0个tensor是分类
    auto class_tensor_info = class_type_info.GetTensorTypeAndShapeInfo();
    std::vector<int64_t> class_tensor_shape = class_tensor_info.GetShape();
    const int64_t class_num = class_tensor_shape[1];


    float max_prob = -1;
    std::vector<float> softmax_vec(class_num);       // 存储未归一化前的置信度

    // 搜索最大分类概率和最大概率对应的下表
    for (int i = 0; i < class_num; ++i)
    {
        softmax_vec[i] = *(output + i);     // 将未归一化的置信度复制到数组

        if (*(output + i) > max_prob)
        {
            max_prob = *(output + i);
        }
    }

    // 归一化logits
    float sum = 0;

    // 计算e的指数和
    for (int i = 0; i < class_num; ++i) {
        sum += std::exp(softmax_vec[i] - max_prob);  // 减去最大值提高数值稳定性
    }

    // 归一化
    for (int i = 0; i < class_num; ++i) {
        softmax_vec[i] = std::exp(softmax_vec[i] - max_prob) / sum;
    }

    return { softmax_vec, key_patch_index };
}


std::array<float, 4> deployment::mmr_predict(const std::vector<float>& features)
{
    // 定义输入张量的形状
    int64_t input_h = features.size() / 768;
    int64_t input_w = 768;
    int64_t video_num = 1;
    std::array<int64_t, 3> model_input_shape = { video_num, input_h, input_w };
    size_t model_input_size = model_input_shape.size();

    // 定义输入张量
    std::vector<float> model_input = features;

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(mmr_inference_ort_components->memory_info,
        model_input.data(),
        model_input.size(),
        model_input_shape.data(),
        model_input_size);

    // MMR模型的输出shape是（1，4，2）
    std::vector<Ort::Value> output_tensor = mmr_inference_ort_components->session->Run(Ort::RunOptions{ nullptr },
        mmr_inference_ort_components->input_nodes_names.data(),
        &input_tensor,
        mmr_inference_ort_components->input_nodes_size,
        mmr_inference_ort_components->output_nodes_names.data(),
        mmr_inference_ort_components->output_nodes_size);


    std::array<float, 4> result;
    // softmax
    for (int i = 0; i < 4; i++) {
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
    :QWidget(parent)
{
    this->confidences = { 0,0,0,0,0,0 };
    this->classNames = { "NA","NA","NA","NA","NA","NA" };
    this->setChartData(this->confidences, this->classNames);
}

void barChartWidget::setChartData(const QVector<float>& confidences, std::vector<std::string>& classNames) {
    this->confidences = confidences;
    this->classNames = classNames;
    update(); // 触发重绘
}




