#ifndef __CAM_IMG_POOL_H__
#define __CAM_IMG_POOL_H__

#include "CamSemDeal.h"

#include <QImage>
#include <QObject>
#include <QString>
#include <list>
#include <mutex>
#include <unordered_map>

using namespace std;

#define IMAGE_POOL_OK 0
#define IMAGE_POOL_ERROR 1
#define IMAGE_POOL_TIMEOUT 2

#define IMAGE_POOL_NONE 0
#define IMAGE_POOL_INITED 1

#define IMAGE_FREE_LIST_TYPE 0
#define IMAGE_DEAL_LIST_TYPE 1
#define IMAGE_USED_LIST_TYPE 2
#define IMAGE_BACK_LIST_TYPE 3

#define POOL_IMAGE_SIZE 8 * 1024 * 1024 // 每个存储池的大小为8MB
#define IMAGE_POOL_NODE_NUMS 8          // 存储池个数

/**
 * @brief 最优图片结构体
 */
struct BestImage
{
    QImage image;                      // 图片
    QString time_stamp;                // 时间戳
    unsigned int image_num;            // 帧号，即图片编号。对每张切片来说，总是从1开始累加
    unsigned int total_image_count;    // 总图片数，即CCD发给软件的图片总数
    unsigned int sharpness;            // 清晰度
    unsigned int similarity;           // 相似度
    unsigned int effective_area;       // 有效面积
    unsigned int normalized_sharpness; // 归一化后的清晰度
    unsigned int magnification;        // 倍率
    bool save;

    /**
     * @brief 构造函数，由成员变量直接复制
     */
    BestImage(QImage _image, QString _time_stamp, unsigned int _image_num, unsigned int _total_image_count,
              unsigned int _sharpness, unsigned int _similarity, unsigned int _effective_area,
              unsigned int _normalized_sharpness, unsigned int _magnification, bool _save);
    /**
     * @brief 构造函数，指针赋值
     * @param ptr BestImage结构体指针
     */
    BestImage(const BestImage* ptr);

    /**
     * @brief 构造函数，成员变量全部为空
     */
    BestImage();

    /**
     * @brief 清空结构体
     */
    void clear();
};

typedef struct __img_pool_node
{
    uchar* m_pData = nullptr; // 传输的图像数据
    unsigned int size = 0;

    // QString m_cur_clock;                        // 接收到图片的时间
    // unsigned int m_best_img_num = 0;            // 送给AI处理的图片的帧号
    // unsigned int m_best_img_sharpness = 0;      // 送给AI处理的图片的清晰度
    // unsigned int m_best_img_similartiy = 0;     // 送给AI处理的图片的相似度
    // unsigned int m_best_img_eff_area = 0;       // 送给AI处理的图片的有效组织大小
    // unsigned int m_best_img_norm_clear = 0;     // 送给AI处理的图片的归一化清晰度
    // QImage m_best_img;
    // int m_mag_num;                              // 放大倍数
    // bool m_save;                                // 是否保存图片
    // int m_cnt;                                  // CCD发给软件的总的图片数

    BestImage best_image = BestImage();

} HL_IMG_POOL_NODE;

typedef struct __img_pool_list
{
    CCSem sem_free;
    CCSem sem_used;
    mutex lock;
    unordered_map<HL_IMG_POOL_NODE*, HL_IMG_POOL_NODE*> free_head;
    unordered_map<HL_IMG_POOL_NODE*, HL_IMG_POOL_NODE*> deal_head;
    unordered_map<HL_IMG_POOL_NODE*, HL_IMG_POOL_NODE*> used_head;
    unordered_map<HL_IMG_POOL_NODE*, HL_IMG_POOL_NODE*> back_head;
    unsigned int mem_pool_flag; // 初始化与否
    unsigned int mem_size;      // 每个node的大小
    unsigned int node_nums;     // 一个pool中多少个节点
} HL_IMG_POOL_LIST;

class CImgPool
{
public:
    HL_IMG_POOL_LIST g_mem_pool_list; // free,deal_list,mutex,sem_free
    void disable_mem_pool();
    int enable_mem_pool(); // 构造函数中调用。初始化图片池对象，主要是给成员链表等成员变量初始化，赋初值
    void mem_pool_clear();
    int mem_pool_reset();

    HL_IMG_POOL_NODE* malloc_free_mem_pool();       // sem_free -1
    void fill_deal_mem_pool(HL_IMG_POOL_NODE* ops); // sem_used+1
    HL_IMG_POOL_NODE* malloc_used_mem_pool();       // sem_used-1
    void free_back_mem_pool(HL_IMG_POOL_NODE* ops); // sem_free +1
    int mem_pool_list_empty(unsigned int type);

public:
    CImgPool();
    ~CImgPool();
};

#endif
