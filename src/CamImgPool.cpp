#include "CamImgPool.h"

#include <QDebug>
#include <iostream>

CImgPool::CImgPool()
{
    enable_mem_pool();
}

CImgPool::~CImgPool()
{
    disable_mem_pool();
}

// 创建结点
static int mem_pool_create(HL_IMG_POOL_LIST* pool_list)
{
    unsigned int size = pool_list->mem_size;
    HL_IMG_POOL_NODE* tmp_node = (HL_IMG_POOL_NODE*)malloc(sizeof(HL_IMG_POOL_NODE));
    if (tmp_node == NULL)
    {
        return IMAGE_POOL_ERROR;
    }

    memset(tmp_node, 0, sizeof(HL_IMG_POOL_NODE));
    tmp_node->m_pData = (uchar*)malloc(size);
    if (tmp_node->m_pData == NULL)
    {
        free(tmp_node);
        return IMAGE_POOL_ERROR;
    }
    memset(tmp_node->m_pData, 0, size);
    tmp_node->size = size;

    pool_list->free_head.insert(make_pair(tmp_node, tmp_node));

    return IMAGE_POOL_OK;
}

static void mem_pool_sem_create(HL_IMG_POOL_LIST* pool_list)
{
    if (pool_list->sem_used.sem_flag == CAM_SEM_INVALID)
    {
        pool_list->sem_used.hl_sem_init(0, pool_list->node_nums);
    }
    if (pool_list->sem_free.sem_flag == CAM_SEM_INVALID)
    {
        pool_list->sem_free.hl_sem_init(pool_list->node_nums, pool_list->node_nums);
    }
}

static void mem_pool_sem_close(HL_IMG_POOL_LIST* pool_list)
{
    if (pool_list->sem_used.sem_flag == CAM_SEM_VALID)
    {
        pool_list->sem_used.hl_sem_close();
    }

    if (pool_list->sem_free.sem_flag == CAM_SEM_VALID)
    {
        pool_list->sem_free.hl_sem_close();
    }
}

static void mem_list_close(unordered_map<HL_IMG_POOL_NODE*, HL_IMG_POOL_NODE*>* mem_head)
{
    HL_IMG_POOL_NODE* ops = NULL;
    struct list_head* tmp_node;
    struct list_head* tmp;

    unordered_map<HL_IMG_POOL_NODE*, HL_IMG_POOL_NODE*>::iterator iter;

    for (iter = mem_head->begin(); iter != mem_head->end(); iter++)
    {
        HL_IMG_POOL_NODE* ops = iter->second;
        free(ops->m_pData);
        ops->m_pData = NULL;
        free(ops);
        ops = NULL;
    }

    mem_head->clear();
}

static void mem_pool_close(HL_IMG_POOL_LIST* pool_list)
{
    mem_list_close(&pool_list->free_head);
    mem_list_close(&pool_list->deal_head);
    mem_list_close(&pool_list->used_head);
    mem_list_close(&pool_list->back_head);
}

static int mem_pool_enable(HL_IMG_POOL_LIST* pool_list)
{
    for (unsigned int i = 0; i < pool_list->node_nums; i++)
    {
        if (mem_pool_create(pool_list) == IMAGE_POOL_ERROR)
        {
            goto err;
        }
    }

    mem_pool_sem_create(pool_list);

    return IMAGE_POOL_OK;

err:
    mem_pool_close(pool_list);
    return IMAGE_POOL_ERROR;
}

static void mem_pool_disable(HL_IMG_POOL_LIST* pool_list)
{
    mem_pool_close(pool_list);
    mem_pool_sem_close(pool_list);
}

void CImgPool::disable_mem_pool()
{
    mem_pool_disable(&(g_mem_pool_list));
}

int CImgPool::enable_mem_pool()
{
    g_mem_pool_list.mem_pool_flag = IMAGE_POOL_NONE;
    g_mem_pool_list.mem_size = POOL_IMAGE_SIZE;
    g_mem_pool_list.node_nums = IMAGE_POOL_NODE_NUMS;
    //
    if (mem_pool_enable(&(g_mem_pool_list)) == IMAGE_POOL_ERROR)
    {
        return IMAGE_POOL_ERROR;
    }

    g_mem_pool_list.mem_pool_flag = IMAGE_POOL_INITED;
    return IMAGE_POOL_OK;
}

void CImgPool::mem_pool_clear()
{
    HL_IMG_POOL_NODE* ops = NULL;
    struct list_head* tmp_node;
    struct list_head* tmp;

    g_mem_pool_list.lock.lock();

    unordered_map<HL_IMG_POOL_NODE*, HL_IMG_POOL_NODE*>::iterator iter;

    for (iter = g_mem_pool_list.used_head.begin(); iter != g_mem_pool_list.used_head.end(); iter++)
    {
        HL_IMG_POOL_NODE* ops = iter->second;
        g_mem_pool_list.free_head.insert(make_pair(ops, ops));
    }

    g_mem_pool_list.used_head.clear();

    for (iter = g_mem_pool_list.deal_head.begin(); iter != g_mem_pool_list.deal_head.end(); iter++)
    {
        HL_IMG_POOL_NODE* ops = iter->second;
        g_mem_pool_list.free_head.insert(make_pair(ops, ops));
    }

    g_mem_pool_list.deal_head.clear();

    for (iter = g_mem_pool_list.back_head.begin(); iter != g_mem_pool_list.back_head.end(); iter++)
    {
        HL_IMG_POOL_NODE* ops = iter->second;
        g_mem_pool_list.free_head.insert(make_pair(ops, ops));
    }

    g_mem_pool_list.back_head.clear();

    mem_pool_sem_close(&(g_mem_pool_list));
    mem_pool_sem_create(&(g_mem_pool_list));
    g_mem_pool_list.lock.unlock();
}

int CImgPool::mem_pool_reset()
{
    g_mem_pool_list.lock.lock();
    mem_pool_disable(&(g_mem_pool_list));
    int ret = mem_pool_enable(&(g_mem_pool_list));
    g_mem_pool_list.lock.unlock();

    return ret;
}

HL_IMG_POOL_NODE* CImgPool::malloc_free_mem_pool()
{
    HL_IMG_POOL_NODE* ops = NULL;
    if (g_mem_pool_list.sem_free.hl_sem_wait(CAM_SEM_WAIT_10MS) != CAM_SEM_OK)
    {
        return NULL;
    }
    g_mem_pool_list.lock.lock();
    if (g_mem_pool_list.mem_pool_flag == IMAGE_POOL_NONE)
    {
        g_mem_pool_list.lock.unlock();
        return NULL;
    }
    if (g_mem_pool_list.free_head.empty())
    {
        g_mem_pool_list.lock.unlock();
        return NULL;
    }
    ops = g_mem_pool_list.free_head.begin()->second;
    g_mem_pool_list.free_head.erase(ops);
    g_mem_pool_list.deal_head.insert(make_pair(ops, ops));

    g_mem_pool_list.lock.unlock();
    return ops;
}

void CImgPool::fill_deal_mem_pool(HL_IMG_POOL_NODE* ops)
{
    if (ops == NULL)
    {
        return;
    }
    g_mem_pool_list.lock.lock();
    if (g_mem_pool_list.mem_pool_flag == IMAGE_POOL_NONE)
    {
        g_mem_pool_list.lock.unlock();
        return;
    }
    if (g_mem_pool_list.deal_head.find(ops) == g_mem_pool_list.deal_head.end())
    {
        g_mem_pool_list.lock.unlock();
        return;
    }
    g_mem_pool_list.deal_head.erase(ops);
    g_mem_pool_list.used_head.insert(make_pair(ops, ops));
    g_mem_pool_list.sem_used.hl_sem_post();
    g_mem_pool_list.lock.unlock();
}

HL_IMG_POOL_NODE* CImgPool::malloc_used_mem_pool()
{
    HL_IMG_POOL_NODE* ops = NULL;
    if (g_mem_pool_list.sem_used.hl_sem_wait(CAM_SEM_WAIT_FOREVER) != CAM_SEM_OK)
    {
        return NULL;
    }
    g_mem_pool_list.lock.lock();
    if (g_mem_pool_list.mem_pool_flag == IMAGE_POOL_NONE)
    {
        g_mem_pool_list.lock.unlock();
        return NULL;
    }
    if (g_mem_pool_list.used_head.empty())
    {
        g_mem_pool_list.lock.unlock();
        return NULL;
    }
    ops = g_mem_pool_list.used_head.begin()->second;
    g_mem_pool_list.used_head.erase(ops);
    g_mem_pool_list.back_head.insert(make_pair(ops, ops));

    g_mem_pool_list.lock.unlock();
    return ops;
}

void CImgPool::free_back_mem_pool(HL_IMG_POOL_NODE* ops)
{
    if (ops == NULL)
    {
        return;
    }
    g_mem_pool_list.lock.lock();
    if (g_mem_pool_list.mem_pool_flag == IMAGE_POOL_NONE)
    {
        g_mem_pool_list.lock.unlock();
        return;
    }
    if (g_mem_pool_list.back_head.find(ops) == g_mem_pool_list.back_head.end())
    {
        g_mem_pool_list.lock.unlock();
        return;
    }
    g_mem_pool_list.back_head.erase(ops);
    g_mem_pool_list.free_head.insert(make_pair(ops, ops));
    g_mem_pool_list.sem_free.hl_sem_post();
    g_mem_pool_list.lock.unlock();
}

int CImgPool::mem_pool_list_empty(unsigned int type)
{
    switch (type)
    {
        case IMAGE_FREE_LIST_TYPE:
            if (g_mem_pool_list.free_head.empty())
            {
                return 0;
            }
            break;
        case IMAGE_DEAL_LIST_TYPE:
            if (g_mem_pool_list.deal_head.empty())
            {
                return 0;
            }
            break;
        case IMAGE_USED_LIST_TYPE:
            if (g_mem_pool_list.used_head.empty())
            {
                return 0;
            }
            break;
        case IMAGE_BACK_LIST_TYPE:
            if (g_mem_pool_list.back_head.empty())
            {
                return 0;
            }
            break;
        default:
            break;
    }

    return 1;
}

BestImage::BestImage(QImage _image, QString _time_stamp, unsigned int _image_num, unsigned int _total_image_count,
                     unsigned int _sharpness, unsigned int _similarity, unsigned int _effective_area,
                     unsigned int _normalized_sharpness, unsigned int _magnification, bool _save)
{
    image = _image;
    time_stamp = _time_stamp;
    image_num = _image_num;
    total_image_count = _total_image_count;
    sharpness = _sharpness;
    similarity = _similarity;
    effective_area = _effective_area;
    normalized_sharpness = _normalized_sharpness;
    magnification = _magnification;
    save = _save;
}

BestImage::BestImage(const BestImage* ptr)
{
    image = ptr->image;
    time_stamp = ptr->time_stamp;
    image_num = ptr->image_num;
    total_image_count = ptr->total_image_count;
    sharpness = ptr->sharpness;
    similarity = ptr->similarity;
    effective_area = ptr->effective_area;
    normalized_sharpness = ptr->normalized_sharpness;
    magnification = ptr->magnification;
    save = ptr->save;
}

BestImage::BestImage()
{
    image = QImage(1824, 1216, QImage::Format_RGB888);
    image.fill(Qt::black);
    time_stamp = "";
    image_num = 0;
    total_image_count = 0;
    sharpness = 0;
    similarity = 0;
    effective_area = 0;
    normalized_sharpness = 0;
    magnification = 0;
    save = true;
}

void BestImage::clear()
{
    image = QImage(1824, 1216, QImage::Format_RGB888);
    image.fill(Qt::black);
    time_stamp = "";
    image_num = 0;
    total_image_count = 0;
    sharpness = 0;
    similarity = 0;
    effective_area = 0;
    normalized_sharpness = 0;
    magnification = 0;
    save = false;
}
