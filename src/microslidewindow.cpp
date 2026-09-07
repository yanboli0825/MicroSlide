#include "MicroSlideWindow.h"

#include "CamImgPool.h"
#include "DBInterfaceDLL.h"
#include "FileTest.h"
#include "IniParser.h"
#include "Logger.h"
#include "MagnificationDetector.h"
#include "OnnxDeployer.h"
#include "ScannerProcessor.h"
#include "SystemCfg.h"
#include "hpec_lib.h"
#include "ui_microslidewindow.h"
#include "utils.h"

#include <QButtonGroup>
#include <QCamera>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QScrollBar>
#include <QThread>
#include <QTimer>
#include <codecvt>
#include <thread>
#include <unordered_map>

MicroSlideWindow::MicroSlideWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MicroSlideWindow)
    , ft(nullptr)
    , systenCfg(nullptr)
    , magdebug(nullptr)
{
    ui->setupUi(this);
    setBtnsNonExclusive();

    setImgTableStyle();

    initializePowerPC();

    setupConnections();

    // 启动程序时使焦点定位到ID输入框
    ui->lineEdit_id->setFocus();

    // 初始化扫描仪设备
    initializeScanner();
    setupScannerConnections();

    // qRegisterMetaType<QVideoFrame>("QVideoFrame");
}

MicroSlideWindow::~MicroSlideWindow()
{
    releaseScanner();
    releasePowerPC();
    delete ui;
}

void MicroSlideWindow::initializePowerPC()
{
    if (PowerPCExitCheck() == -1)
    {
        LOGGER_ERROR("Cannot detect the PowerPC.");
        QMessageBox::critical(this, "错误", "未检测到PowerPC设备");
#ifdef NDEBUG
        exit(1);
#endif
    }
    else
    {
        m_busNum = bus_num_get(0);
        if (m_busNum == -1)
        {
            LOGGER_ERROR("Failed to get PowerPC bus number.");
            QMessageBox::critical(this, "错误", "获取PowerPC总线号失败");
#ifdef NDEBUG
            exit(1);
#endif
        }
        else
        {
            LOGGER_INFO("Successfully get PowerPC bus number: {}", m_busNum);
            // 将总线号传给bgcamera对象
            ui->widget_camera->setBusNum(m_busNum);

            // 初始化应用程序
            int result = InitApp(m_busNum);
            if (result == 0)
            {
                LOGGER_INFO("Successfully initialize the PowerPC.");
            }
            else
            {
                LOGGER_ERROR("Failed to initialize the PowerPC.");
                QMessageBox::critical(this, "错误", "初始化PowerPC设备失败");
#ifdef NDEBUG
                exit(1);
#endif
            }
        }
    }
}

void MicroSlideWindow::releasePowerPC()
{
    // 释放Windows上位机接口资源
    CloseApp(m_busNum);
}

void MicroSlideWindow::setupConnections()
{
    connect(ui->widget_camera, &bgCamera::signal_show_image, this, &MicroSlideWindow::slot_addImageToTable);

    connect(ui->widget_camera, &bgCamera::signal_showPreviewImg, this, &MicroSlideWindow::slot_addPreviewImgToTable);

    connect(this, &MicroSlideWindow::signal_threadPoolFinished, this, &MicroSlideWindow::slot_threadPoolFinished);

    connect(ui->widget_camera, &bgCamera::directly_click_btn_trans, this, &MicroSlideWindow::on_btn_id_clicked);

    connect(ui->widget_camera, &bgCamera::signal_showDBDisconnectWarning, this,
            [this]() { QMessageBox::warning(this, tr("警告"), tr("连接数据库失败，请检查网络连接并重启应用程序")); });

    connect(ui->action_magdebug, &QAction::triggered, this,
            [this]()
            {
                if (ui->widget_camera->m_isTrans == true)
                {
                    ui->widget_camera->onBtnTransClicked();
                }
                magdebug = nullptr;
                magdebug = new MagDebug();
                magdebug->setAttribute(Qt::WA_DeleteOnClose);
                magdebug->setWindowModality(Qt::ApplicationModal);
                magdebug->setWindowTitle("倍率调试");
                connect(magdebug, &MagDebug::signal_rect_value_changed, ui->widget_camera,
                        &bgCamera::slot_cameraRectValueChanged);
                connect(magdebug, &MagDebug::destroyed,
                        [this]
                        {
                            ui->widget_camera->slot_cbox_magDetectStateChanged(Qt::CheckState::Unchecked);
                            ui->widget_camera->slot_cbox_magDetectStateChanged(Qt::CheckState::Checked);
                            magdebug = nullptr;
                        });
                magdebug->show();
            });
}

json convert2json(IF_QUE* que)
{
    json res = {
        {"image_dir", que->image_dir},
        {"feature_path", que->feature_path},
        {"pathological_id", que->pathological_id},
        {"slice_id", que->slice_id},
        {"pathological_order", que->pathological_order},
        {"port", que->port},
        {"ftpnode", que->ftpnode},
        {"ftpusr", que->ftpusr},
        {"ftppwd", que->ftppwd},
    };
    return res;
}

void MicroSlideWindow::setImgTableStyle()
{
    // 设置表头字体
    QFont font("Microsoft YaHei", 10);
    font.setBold(true);
    ui->table_img->horizontalHeader()->setFont(font);

    // 设置最后一列拉伸至最大
    ui->table_img->horizontalHeader()->setStretchLastSection(true);

    // 设置表头颜色
    ui->table_img->horizontalHeader()->setStyleSheet("QHeaderView::section{background:#b0b0b0}");

    // 设置选择单个单元格
    ui->table_img->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->table_img->setSelectionBehavior(QAbstractItemView::SelectItems);

    // 设置列宽度
    ui->table_img->horizontalHeader()->resizeSection(0, PIC_WIDTH);
    ui->table_img->horizontalHeader()->resizeSection(1, 50);
    ui->table_img->horizontalHeader()->resizeSection(2, 70);

    // 设置3列
    ui->table_img->setColumnCount(3);
    QStringList str_list = {"图片", "结果", "置信度"};
    ui->table_img->setHorizontalHeaderLabels(str_list);

    // 禁止点表头的列
    ui->table_img->horizontalHeader()->setSectionsClickable(false);

    // 设置表格不可编辑
    ui->table_img->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 禁止表头拖动
    ui->table_img->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    // 设置滑杆滑动步数变小，像素移动
    ui->table_img->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    // 设置越小下滑越慢
    ui->table_img->verticalScrollBar()->setSingleStep(10);
}

void MicroSlideWindow::setBtnsNonExclusive()
{
    QButtonGroup* buttonGroup = new QButtonGroup(this);
    buttonGroup->addButton(ui->rBtn_C);
    buttonGroup->addButton(ui->rBtn_HP);
    buttonGroup->addButton(ui->rBtn_IP);
    buttonGroup->addButton(ui->rBtn_NM);
    buttonGroup->addButton(ui->rBtn_SSA);
    buttonGroup->addButton(ui->rBtn_TA);
    buttonGroup->addButton(ui->rBtn_TSA);
    buttonGroup->addButton(ui->rBtn_HGD);
    buttonGroup->addButton(ui->rBtn_other);

    // add stomach btn
    buttonGroup->addButton(ui->rBtn_stomach_1);
    buttonGroup->addButton(ui->rBtn_stomach_2);
    buttonGroup->addButton(ui->rBtn_stomach_3);
    buttonGroup->addButton(ui->rBtn_stomach_4);
    buttonGroup->addButton(ui->rBtn_stomach_5);
    buttonGroup->addButton(ui->rBtn_stomach_6);
    buttonGroup->addButton(ui->rBtn_stomach_7);
    buttonGroup->addButton(ui->rBtn_stomach_8);
    buttonGroup->addButton(ui->rBtn_stomach_9);

    // add prostate btn
    buttonGroup->addButton(ui->rBtn_prostate_1);
    buttonGroup->addButton(ui->rBtn_prostate_2);
    buttonGroup->addButton(ui->rBtn_prostate_3);
    buttonGroup->addButton(ui->rBtn_prostate_4);
    buttonGroup->addButton(ui->rBtn_prostate_5);
    buttonGroup->addButton(ui->rBtn_prostate_6);
    buttonGroup->addButton(ui->rBtn_prostate_7);
    buttonGroup->addButton(ui->rBtn_prostate_8);
    buttonGroup->addButton(ui->rBtn_prostate_9);
    buttonGroup->addButton(ui->rBtn_prostate_10);
    buttonGroup->addButton(ui->rBtn_prostate_11);
    buttonGroup->addButton(ui->rBtn_prostate_12);
    buttonGroup->addButton(ui->rBtn_prostate_13);

    // add lung btn
    buttonGroup->addButton(ui->rBtn_lung_1);
    buttonGroup->addButton(ui->rBtn_lung_2);
    buttonGroup->addButton(ui->rBtn_lung_3);
    buttonGroup->addButton(ui->rBtn_lung_4);
    buttonGroup->addButton(ui->rBtn_lung_5);
    buttonGroup->addButton(ui->rBtn_lung_6);

    // add esophagus btn
    buttonGroup->addButton(ui->rBtn_esophagus_1);
    buttonGroup->addButton(ui->rBtn_esophagus_2);
    buttonGroup->addButton(ui->rBtn_esophagus_3);
    buttonGroup->addButton(ui->rBtn_esophagus_4);

    // add liver btn
    buttonGroup->addButton(ui->rBtn_liver_1);
    buttonGroup->addButton(ui->rBtn_liver_2);
    buttonGroup->addButton(ui->rBtn_liver_3);
    buttonGroup->addButton(ui->rBtn_liver_4);

    // 互斥模式设置为false，使按钮可以多选
    buttonGroup->setExclusive(false);
}

void MicroSlideWindow::slot_addPreviewImgToTable(QPixmap _preview_img, int _best_img_num)
{
    ui->table_img->setRowCount(_best_img_num); // 设置行数
    int row = _best_img_num - 1;
    int col = 0;

    QLabel* item1 = new QLabel(this);

    double ratio = _preview_img.height() * 1.0 / _preview_img.width();
    item1->setAlignment(Qt::AlignmentFlag::AlignCenter);
    item1->setFixedWidth(PIC_WIDTH);
    item1->setFixedHeight(PIC_WIDTH * ratio);

    // 将QPixmap 对象 pix 按宽度缩放到 PIC_WIDTH，并设置为 QLabel 的图片
    item1->setPixmap(_preview_img.scaledToWidth(PIC_WIDTH));
    item1->setScaledContents(true); // 设置 QLabel 的内容（图片）在必要时可以缩放以适应标签的大小

    // 添加图片
    ui->table_img->setCellWidget(row, col, item1);

    // 更改行高
    ui->table_img->setRowHeight(row, item1->height());

    // 添加文字
    QLabel* item2 = new QLabel("Loading", this);
    item2->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter); // 设置为水平且垂直居中
    ui->table_img->setCellWidget(row, ++col, item2);

    QLabel* item3 = new QLabel("Loading", this);
    item3->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter); // 设置为水平且垂直居中
    ui->table_img->setCellWidget(row, ++col, item3);

    ui->table_img->scrollToBottom(); // 将 table_img 的视图滚动到底部，以便用户可以看到新添加的图片
}

void MicroSlideWindow::slot_addImageToTable(const std::shared_ptr<BestImage>& img,
                                            const std::shared_ptr<AIResult>& img_inference_res,
                                            const QString& _video_res)
{
    QString row_number = QString::number(img->image_num);
    QString mag = "×" + QString::number(img->magnification);
    QImage image = img->image;

    cv::Point key_patch = img_inference_res->ROI;

    QPainter painter(&image);
    QFont font = painter.font();
    font.setPointSize(60);
    painter.setFont(font);
    painter.drawText(60, 100, mag);
    painter.setPen(QPen(Qt::red, 8));
    QRect rect(key_patch.x, key_patch.y, WINDOW_SIZE, WINDOW_SIZE);
    QRect imageRect(0, 0, image.width(), image.height());
    QRect intersectedRect = rect.intersected(imageRect);
    painter.drawRect(intersectedRect);
    painter.end();

    int col = 0; // 病理结果读文件名得知
    QLabel* item1 = new QLabel(this);
    double ratio = image.height() * 1.0 / image.width();

    item1->setAlignment(Qt::AlignmentFlag::AlignCenter);
    item1->setFixedWidth(PIC_WIDTH);
    item1->setFixedHeight(PIC_WIDTH * ratio);
    // 调整图片大小和格式
    QPixmap pix = QPixmap::fromImage(image);        // 将QImage对象转换为QPixmap对象
    item1->setPixmap(pix.scaledToWidth(PIC_WIDTH)); // 将 QPixmap 对象 pix 按宽度缩放到 PIC_WIDTH，并设置为 QLabel
                                                    // 的图片
    item1->setScaledContents(true);                 // 设置 QLabel 的内容（图片）在必要时可以缩放以适应标签的大小

    // 添加图片
    ui->table_img->setCellWidget(row_number.toInt() - 1, col, item1);

    // 更改行高
    ui->table_img->setRowHeight(row_number.toInt() - 1, item1->height());

    // 添加文字
    QString _num = QString::number(std::round(img_inference_res->diagnosis_prob * 1000.f) / 1000.f);
    QString _res = img_inference_res->diagnosis_res + "\n" + _num;

    QLabel* item = new QLabel(_res, this);

    item->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter); // 设置为水平且垂直居中
    ui->table_img->setCellWidget(row_number.toInt() - 1, ++col, item);

    // 在第三行添加可视化的概率分布
    barChartWidget* prob_visualizer = new barChartWidget(this);

    std::vector<std::string> cls_name;
    auto it = bgCamera::slicePartToClassNamesMap.find(this->ui->widget_camera->m_slideInfo->slicesource);
    if (it != bgCamera::slicePartToClassNamesMap.end())
    {
        cls_name = it->second;
    }
    else
    {
        cls_name = bgCamera::slicePartToClassNamesMap.find(SLICESOURCE_DEFAULT)->second;
    }

    prob_visualizer->setChartData(img_inference_res->cls_prob, cls_name);
    ui->table_img->setCellWidget(row_number.toInt() - 1, ++col, prob_visualizer);

    // 更新video诊断结果
    ui->label_video_res->setText(_video_res);

    // 更新到最底下，滑动的效果
    ui->table_img->scrollToBottom(); // 将 table_img 的视图滚动到底部，以便用户可以看到新添加的图片
}

void MicroSlideWindow::clearImagesInTable()
{
    ui->table_img->clearContents();
    ui->table_img->setRowCount(0);
}

void MicroSlideWindow::slot_scannerPIDCaptured(const std::string& input)
{
    std::string PID = input;
    // 处理解码的扫描数据包含换行符'\r'的情况
    if (!input.empty() && input.back() == '\r')
    {
        PID = PID.substr(0, PID.length() - 1);
    }

    // 如果当前已经有PID，则阻塞主线程，等待AI推理结束
    if (ui->widget_camera->m_saveDir != "")
    {
        QEventLoop loop;
        connect(this, &MicroSlideWindow::signal_unblockThread, &loop, &QEventLoop::quit);

        // 病理医生点击确认按钮，准备开始新的诊断流程
        on_btn_confirm_doc_clicked();

        // 阻塞主线程，直到AI推理完所有当前PID的图片
        loop.exec();

        disconnect(this, &MicroSlideWindow::signal_unblockThread, nullptr, nullptr);
    }

    // 若是第一次扫描，则直接将PID显示在输入框中
    ui->lineEdit_id->setText(QString::fromStdString(PID));
    // 确定输入新的PID
    on_btn_id_clicked();
}

void MicroSlideWindow::initializeScanner()
{
    bool result = ScannerProcessor::initialize();
    if (result)
    {
        LOGGER_INFO("Scanner device initialized successfully.");
        QMessageBox::information(this, "提示", tr("扫描仪设备已连接"));
    }
    else
    {
        LOGGER_WARN("Scanner device not found.");
        QMessageBox::warning(this, "警告", tr("未发现扫描仪设备"));
    }
}

void MicroSlideWindow::releaseScanner()
{
    ScannerProcessor::release();
    LOGGER_INFO("Scanner device released.");
}

void MicroSlideWindow::setupScannerConnections()
{
    connect(ScannerProcessor::get(), &ScannerProcessor::decodeDataReceived, this,
            &MicroSlideWindow::slot_scannerPIDCaptured);
}

void MicroSlideWindow::on_btn_id_clicked()
{
    // 清空图片栏
    clearImagesInTable();

    // 获取通过扫描枪得到的PID
    QString PID = ui->lineEdit_id->text();
    if (PID == "")
    {
        // 未输入PID则默认为当前系统时间
        PID = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    }

    // 检查PID中是否包含禁用字符。禁用字符包括：“_”
    if (PID.contains("_"))
    {
        QMessageBox::warning(this, tr("警告"), tr("PID中不能包含下划线"));
        return;
    }

    // 将PID输入框与确认按钮设定为禁用状态
    ui->lineEdit_id->setEnabled(false);
    ui->btn_id->setEnabled(false);

#ifdef DB_SEND
    // 向数据库发送病理号、切片号等信息，从PIS系统中获取切片部位、镜下描述等医学信息
    WSAInit();
    SOCKET dbClientSocket;
    unsigned short port = static_cast<unsigned short>(ui->widget_camera->m_dbPort);
    int dbConnectRes = dbSockInit(dbClientSocket, ui->widget_camera->m_dbAddr, port, 5);

    // 检查是否成功初始化了db套接字
    if (!dbConnectRes)
    {
        GET_PID query = {
            PID.toStdString(),                                         // 病理号，这里直接发送扫描枪扫描结果
            PID.toStdString().substr(PID.size() - 1, 1),               // 切片号，这里为病理号的最后一个字符
            utils::wcharToString(ui->widget_camera->m_microDevice.id), // 显微镜唯一ID
            "A01"                                                      // 数据包类型
        };
        dbGetPathinfo(&query, dbClientSocket, ui->widget_camera->m_slideInfo.get(), ui->widget_camera->m_dbAddr, port,
                      5);
        dbSockClose(dbClientSocket);
        WSAClose();

        // 判断从数据库拿到的数据是否有效
        if (ui->widget_camera->m_slideInfo->pathological_id == "")
        {
            QMessageBox::warning(this, tr("警告"), tr("数据库未查询到有效信息！"));

            // 将PID输入框与确认按钮设定为可用状态
            ui->lineEdit_id->setEnabled(true);
            ui->btn_id->setEnabled(true);
            return;
        }

        // 根据数据库返回的信息，确定文件保存目录。保存目录命名格式为：病理号_切片号_诊断序号
        LOGGER_INFO("Successfully get pathological information from database, pathological_id: {}, slice_id: {}, "
                    "pathological_order: {}, ",
                    ui->widget_camera->m_slideInfo->pathological_id, ui->widget_camera->m_slideInfo->slice_id,
                    ui->widget_camera->m_slideInfo->pathological_order);
        QString pathological_id = QString::fromStdString(ui->widget_camera->m_slideInfo->pathological_id);
        QString slice_id = QString::fromStdString(ui->widget_camera->m_slideInfo->slice_id);

        QString pathological_order = QString::number(ui->widget_camera->m_slideInfo->pathological_order);
        ui->widget_camera->m_saveDir =
            ui->widget_camera->m_saveRoot + "/" + pathological_id + "_" + slice_id + "_" + pathological_order;

        // 切换切片，清空图片特征
        ui->widget_camera->m_imagesMetrics.clear();
    }
    else
    {
        QMessageBox::warning(this, tr("警告"), tr("未连接数据库，无法获取切片信息!"));
        // 若未成功连接数据库，则保存路径与不开启数据库功能时的命名相同
        ui->widget_camera->m_saveDir = ui->widget_camera->m_saveRoot + "/" + PID + "_NA_0000_NA";
    }

#else
    // 若未启用数据库功能，则文件保存目录默认为：PID_NA_0000_NA
    ui->widget_camera->m_saveDir = ui->widget_camera->m_saveRoot + "/" + PID + "_NA_0000_NA";

#endif // DB_SEND

    // 为当前病理切片创建保存目录
    QDir().mkpath(ui->widget_camera->m_saveDir);

    // 更改病理号后，清除bgcamera类中video_features中记录的视频特征
    ui->widget_camera->m_video_features.clear();
    ui->label_video_res->setText(".........");

    // 初始化与bgcamera筛选图片相关的变量
    ui->widget_camera->m_staticViewImgNum = 0;                    // 用于判定连续给出相同图片的数量，若大于等于4则不出图
    ui->widget_camera->m_similarImgNum = 0;                       // 记录相似的图片的数目，每5张图片输出一次，或移动输出
    ui->widget_camera->m_validImgNum = 0;                         // 记录输出的图片的数量
    ui->widget_camera->m_lastSimilarity = SIMILARITY_RESET_VALUE; // 上一张图的相似度的初始值
    ui->widget_camera->m_validImg->clear();                       // 记录挑选的图片的清晰度

    // 开始传输
    ui->widget_camera->m_isTrans = false;
    if (ui->widget_camera->onBtnTransClicked() == false)
    {
        ui->lineEdit_id->setEnabled(true);
        ui->btn_id->setEnabled(true);
    }
}

void MicroSlideWindow::on_btn_confirm_doc_clicked()
{
    // 若PID为空，则提示未输入患者信息
    if (ui->widget_camera->m_saveDir == "")
    {
        QMessageBox::warning(this, tr("警告"), tr("未检测到有效的患者数据"));
        return;
    }

    // 获取医生诊断结果。若医生未诊断，则默认为AI的诊断结果
    QString doc_diagnose = getDoctorDiagnosisResultFromBtn();
    if (doc_diagnose == "")
    {
        QString AI_diagnose = ui->label_video_res->text();
        QStringList parts = AI_diagnose.split(",");
        doc_diagnose = parts[0];
    }

    ui->btn_confirm_doc->setEnabled(false);
    ui->widget_camera->m_isTrans = true;

    ui->widget_camera->onBtnTransClicked();

    // 创建子线程，在子线程中轮询AI处理线程数以判断是否处理完了所有图片数据，在等待AI线程处理完所有数据后发出AI推理完成的信号并detach该子线程
    std::thread node_check(
        [this, doc_diagnose]()
        {
            while (true)
            {
                QThread::msleep(500);
                if (this->ui->widget_camera->m_numFreeThread != DEAL_THREAD_MAX_NUMS)
                {
                    continue;
                }
                QThread::msleep(500);
                if (this->ui->widget_camera->m_numFreeThread != DEAL_THREAD_MAX_NUMS)
                {
                    continue;
                }

                // 保存切片对应的图片特征包至保存目录
                savejson(&ui->widget_camera->m_imagesMetrics, ui->widget_camera->m_saveDir.toStdString(),
                         "images_features.json");

                emit this->signal_threadPoolFinished(doc_diagnose);

#ifdef DB_SEND
                // 医生点击确认后，再次向数据库发送视频诊断结果并补充医生诊断结果（AI处理线程中发送的信息是不包括医生诊断结果的）
                WSAInit();
                SOCKET dbClientSock;
                unsigned short port = static_cast<unsigned short>(ui->widget_camera->m_dbPort);
                int dbConnectRet = dbSockInit(dbClientSock, ui->widget_camera->m_dbAddr, port, 5);

                // 连接数据库成功，发送最后的视频诊断结果包
                if (!dbConnectRet)
                {
                    std::vector<double> video_res_prob(ui->widget_camera->m_video_res.second.begin(),
                                                       ui->widget_camera->m_video_res.second.end());

                    PATHO_RES video_res = {
                        ui->widget_camera->m_slideInfo->pathological_id,           // 病理号
                        ui->widget_camera->m_slideInfo->slice_id,                  // 切片号
                        utils::wcharToString(ui->widget_camera->m_microDevice.id), // 传感器编号
                        "1.0",                                                     // 版本号
                        static_cast<long int>(time(nullptr)),                      // 诊断时间
                        ui->widget_camera->m_slideInfo->pathological_order,        // 诊断次数
                        ui->widget_camera->m_validImg->total_image_count,          // 一个切片号对应图片
                        9,                                                         // 图片特征参数个数
                        ui->widget_camera->m_slideInfo->pathological_order,        // 诊断次序
                        ui->widget_camera->m_video_res.first,                      // AI诊断结果
                        video_res_prob,                                            // AI诊断概率
                        doc_diagnose.toStdString(),                                // 医生诊断结果
                        "A02",                                                     // 包的类型
                    };
                    dbResultUpload(&video_res, dbClientSock, ui->widget_camera->m_dbAddr, port, 5);
                    dbSockClose(dbClientSock);
                }
                else
                {
                    LOGGER_ERROR("Failed to connect to database to upload final diagnosis result.");
                }

                // 若开启数据库功能，则将图片和图片特征相关的保存信息放入数据队列，发送给数据库
                std::string queue_dir = "C:\\hpec_temp";
                // 创建队列目录
                if (!std::filesystem::exists(queue_dir) || !std::filesystem::is_directory(queue_dir))
                {
                    std::filesystem::create_directory(queue_dir);
                };

                IF_QUE queueData = {ui->widget_camera->m_saveDir.toStdString(),
                                    ui->widget_camera->m_saveDir.toStdString() + "/images_features.json",
                                    ui->widget_camera->m_slideInfo->pathological_id,
                                    ui->widget_camera->m_slideInfo->slice_id,
                                    ui->widget_camera->m_slideInfo->pathological_order,
                                    ui->widget_camera->m_slideInfo->storage_node,
                                    ui->widget_camera->m_slideInfo->port,
                                    ui->widget_camera->m_slideInfo->ftpusr,
                                    ui->widget_camera->m_slideInfo->ftppwd};
                json queueDataJson = convert2json(&queueData);
                savejson(&queueDataJson, queue_dir,
                         ui->widget_camera->m_slideInfo->pathological_id + "_" +
                             ui->widget_camera->m_slideInfo->slice_id + "_" +
                             std::to_string(ui->widget_camera->m_slideInfo->pathological_order) + ".json");
#endif // DB_SEND
                return;
            }
        });
    node_check.detach();
}

QString MicroSlideWindow::getDoctorDiagnosisResultFromBtn()
{
    QVector<QString> vec;
    if (ui->rBtn_C->isChecked())
    {
        vec.push_back("C");
        ui->rBtn_C->setChecked(false);
    }
    if (ui->rBtn_SSA->isChecked())
    {
        vec.push_back("SSA");
        ui->rBtn_SSA->setChecked(false);
    }
    if (ui->rBtn_TA->isChecked())
    {
        vec.push_back("TA");
        ui->rBtn_TA->setChecked(false);
    }
    if (ui->rBtn_HP->isChecked())
    {
        vec.push_back("HP");
        ui->rBtn_HP->setChecked(false);
    }
    if (ui->rBtn_IP->isChecked())
    {
        vec.push_back("IP");
        ui->rBtn_IP->setChecked(false);
    }
    if (ui->rBtn_NM->isChecked())
    {
        vec.push_back("NM");
        ui->rBtn_NM->setChecked(false);
    }
    if (ui->rBtn_TSA->isChecked())
    {
        vec.push_back("TSA");
        ui->rBtn_TSA->setChecked(false);
    }
    if (ui->rBtn_HGD->isChecked())
    {
        vec.push_back("HGD");
        ui->rBtn_HGD->setChecked(false);
    }
    if (ui->rBtn_other->isChecked())
    {
        vec.push_back(ui->line_other->text());
        ui->rBtn_other->setChecked(false);
    }
    if (ui->rBtn_other->isChecked())
    {
        vec.push_back(ui->line_other->text());
        ui->rBtn_other->setChecked(false);
    }
    if (ui->rBtn_other->isChecked())
    {
        vec.push_back(ui->line_other->text());
        ui->rBtn_other->setChecked(false);
    }
    // 补充
    // 胃
    if (ui->rBtn_stomach_1->isChecked())
    {
        vec.push_back(ui->rBtn_stomach_1->text());
        ui->rBtn_stomach_1->setChecked(false);
    }
    if (ui->rBtn_stomach_2->isChecked())
    {
        vec.push_back(ui->rBtn_stomach_2->text());
        ui->rBtn_stomach_2->setChecked(false);
    }
    if (ui->rBtn_stomach_3->isChecked())
    {
        vec.push_back(ui->rBtn_stomach_3->text());
        ui->rBtn_stomach_3->setChecked(false);
    }
    if (ui->rBtn_stomach_4->isChecked())
    {
        vec.push_back(ui->rBtn_stomach_4->text());
        ui->rBtn_stomach_4->setChecked(false);
    }
    if (ui->rBtn_stomach_5->isChecked())
    {
        vec.push_back(ui->rBtn_stomach_5->text());
        ui->rBtn_stomach_5->setChecked(false);
    }
    if (ui->rBtn_stomach_6->isChecked())
    {
        vec.push_back(ui->rBtn_stomach_6->text());
        ui->rBtn_stomach_6->setChecked(false);
    }
    if (ui->rBtn_stomach_7->isChecked())
    {
        vec.push_back(ui->rBtn_stomach_7->text());
        ui->rBtn_stomach_7->setChecked(false);
    }
    if (ui->rBtn_stomach_8->isChecked())
    {
        vec.push_back(ui->rBtn_stomach_8->text());
        ui->rBtn_stomach_8->setChecked(false);
    }
    if (ui->rBtn_stomach_9->isChecked())
    {
        vec.push_back(ui->rBtn_stomach_9->text());
        ui->rBtn_stomach_9->setChecked(false);
    }
    // 前列腺
    if (ui->rBtn_prostate_1->isChecked())
    {
        vec.push_back(ui->rBtn_prostate_1->text());
        ui->rBtn_prostate_1->setChecked(false);
    }
    if (ui->rBtn_prostate_2->isChecked())
    {
        vec.push_back(ui->rBtn_prostate_2->text());
        ui->rBtn_prostate_2->setChecked(false);
    }
    if (ui->rBtn_prostate_3->isChecked())
    {
        vec.push_back(ui->rBtn_prostate_3->text());
        ui->rBtn_prostate_3->setChecked(false);
    }
    if (ui->rBtn_prostate_4->isChecked())
    {
        vec.push_back(ui->rBtn_prostate_4->text());
        ui->rBtn_prostate_4->setChecked(false);
    }
    if (ui->rBtn_prostate_5->isChecked())
    {
        vec.push_back(ui->rBtn_prostate_5->text());
        ui->rBtn_prostate_5->setChecked(false);
    }
    if (ui->rBtn_prostate_6->isChecked())
    {
        vec.push_back(ui->rBtn_prostate_6->text());
        ui->rBtn_prostate_6->setChecked(false);
    }
    if (ui->rBtn_prostate_7->isChecked())
    {
        vec.push_back(ui->rBtn_prostate_7->text());
        ui->rBtn_prostate_7->setChecked(false);
    }
    if (ui->rBtn_prostate_8->isChecked())
    {
        vec.push_back(ui->rBtn_prostate_8->text());
        ui->rBtn_prostate_8->setChecked(false);
    }
    if (ui->rBtn_prostate_9->isChecked())
    {
        vec.push_back(ui->rBtn_prostate_9->text());
        ui->rBtn_prostate_9->setChecked(false);
    }
    if (ui->rBtn_prostate_10->isChecked())
    {
        vec.push_back(ui->rBtn_prostate_10->text());
        ui->rBtn_prostate_10->setChecked(false);
    }
    if (ui->rBtn_prostate_11->isChecked())
    {
        vec.push_back(ui->rBtn_prostate_11->text());
        ui->rBtn_prostate_11->setChecked(false);
    }
    if (ui->rBtn_prostate_12->isChecked())
    {
        vec.push_back(ui->rBtn_prostate_12->text());
        ui->rBtn_prostate_12->setChecked(false);
    }
    if (ui->rBtn_prostate_13->isChecked())
    {
        vec.push_back(ui->rBtn_prostate_13->text());
        ui->rBtn_prostate_13->setChecked(false);
    }
    // 肺
    if (ui->rBtn_lung_1->isChecked())
    {
        vec.push_back(ui->rBtn_lung_1->text());
        ui->rBtn_lung_1->setChecked(false);
    }
    if (ui->rBtn_lung_2->isChecked())
    {
        vec.push_back(ui->rBtn_lung_2->text());
        ui->rBtn_lung_2->setChecked(false);
    }
    if (ui->rBtn_lung_3->isChecked())
    {
        vec.push_back(ui->rBtn_lung_3->text());
        ui->rBtn_lung_3->setChecked(false);
    }
    if (ui->rBtn_lung_4->isChecked())
    {
        vec.push_back(ui->rBtn_lung_4->text());
        ui->rBtn_lung_4->setChecked(false);
    }
    if (ui->rBtn_lung_5->isChecked())
    {
        vec.push_back(ui->rBtn_lung_5->text());
        ui->rBtn_lung_5->setChecked(false);
    }
    if (ui->rBtn_lung_6->isChecked())
    {
        vec.push_back(ui->rBtn_lung_6->text());
        ui->rBtn_lung_6->setChecked(false);
    }
    // 食管
    if (ui->rBtn_esophagus_1->isChecked())
    {
        vec.push_back(ui->rBtn_esophagus_1->text());
        ui->rBtn_esophagus_1->setChecked(false);
    }
    if (ui->rBtn_esophagus_2->isChecked())
    {
        vec.push_back(ui->rBtn_esophagus_2->text());
        ui->rBtn_esophagus_2->setChecked(false);
    }
    if (ui->rBtn_esophagus_3->isChecked())
    {
        vec.push_back(ui->rBtn_esophagus_3->text());
        ui->rBtn_esophagus_3->setChecked(false);
    }
    if (ui->rBtn_esophagus_4->isChecked())
    {
        vec.push_back(ui->rBtn_esophagus_4->text());
        ui->rBtn_esophagus_4->setChecked(false);
    }
    // 肝
    if (ui->rBtn_liver_1->isChecked())
    {
        vec.push_back(ui->rBtn_liver_1->text());
        ui->rBtn_liver_1->setChecked(false);
    }
    if (ui->rBtn_liver_2->isChecked())
    {
        vec.push_back(ui->rBtn_liver_2->text());
        ui->rBtn_liver_2->setChecked(false);
    }
    if (ui->rBtn_liver_3->isChecked())
    {
        vec.push_back(ui->rBtn_liver_3->text());
        ui->rBtn_liver_3->setChecked(false);
    }
    if (ui->rBtn_liver_4->isChecked())
    {
        vec.push_back(ui->rBtn_liver_4->text());
        ui->rBtn_liver_4->setChecked(false);
    }
    QString res = vec.join("_");

    return res;
}

void MicroSlideWindow::on_act_sysConfig_triggered()
{
    if (systenCfg == nullptr)
    {
        systenCfg = new SysConfigForm();
        systenCfg->setAttribute(Qt::WA_DeleteOnClose);
        connect(systenCfg, &SysConfigForm::destroyed, [this] { this->systenCfg = nullptr; });
        connect(systenCfg, &SysConfigForm::signal_syscfgChanged, this, &MicroSlideWindow::slot_on_syscfgChanged);
        systenCfg->show();
    }
    else
    {
        systenCfg->show();
    }
}

void MicroSlideWindow::slot_on_syscfgChanged(int _sharp, int _similarity, int _area, QString _filepath,
                                             std::string db_addr, int db_port)
{
    ui->widget_camera->m_sharpThres = _sharp;
    ui->widget_camera->m_similarityThres = _similarity;
    ui->widget_camera->m_saveRoot = _filepath;
    ui->widget_camera->m_areaThres = _area;
    ui->widget_camera->m_dbAddr = db_addr;
    ui->widget_camera->m_dbPort = db_port;
}

void MicroSlideWindow::slot_threadPoolFinished(QString diagnose)
{
    ui->lineEdit_id->clear();
    ui->lineEdit_id->setEnabled(true);
    ui->btn_confirm_doc->setEnabled(true);
    ui->btn_id->setEnabled(true);

    // 点击确诊按钮后，将病理号输入框设置为focus，方便扫描枪继续扫描病理号
    ui->lineEdit_id->setFocus();

#ifndef DB_SEND
    // 根据诊断结果得到新的文件夹名
    QStringList parts = ui->widget_camera->m_saveDir.split("/"); // 分离保存路径
    QString folderName = parts[parts.length() - 1];
    QStringList parts2 = folderName.split("_"); // 分离图片上级文件夹名
    QString AI_diagnose = ui->label_video_res->text();

    if (AI_diagnose != ".........")
    {
        if (AI_diagnose.length() <= 40)
        {
            QStringList parts3 = AI_diagnose.split(","); // 分离AI诊断结果
            parts2[1] = parts3[0];
            parts2[2] = parts3[1].split(".").join("");
        }
        else
        {
            AI_diagnose.replace(": ", "-");
            QStringList parts3 = AI_diagnose.split("(");
            QStringList parts4 = parts3[0].split(",");
            parts2[1] = parts4[0] + "(" + parts3[1];
            parts2[2] = parts4[1].split(".").join("");
        }
    }
    parts2[3] = diagnose;
    folderName = parts2.join("_");
    parts[parts.length() - 1] = folderName;
    QString new_save_dir = parts.join("/");

    // 重命名文件夹
    // 若没有修改则直接返回true，否则返回修改结果（true表示修改成功，false表示修改失败）
    QDir dir;
    bool success;
    if (ui->widget_camera->m_saveDir == new_save_dir)
    {
        success = true;
    }
    else
    {
        success = dir.rename(ui->widget_camera->m_saveDir, new_save_dir);
    }

    if (success)
    {
        ui->widget_camera->m_saveDir = new_save_dir;
        QMessageBox* box = new QMessageBox(QMessageBox::Information, tr("提示"), tr("已完成诊断!"));
        box->setAttribute(Qt::WA_DeleteOnClose);

        QTimer::singleShot(1000, box, SLOT(close()));
        box->show();
    }
    else
    {
        QMessageBox::warning(this, "警告", "诊断失败：未能重命名文件夹！\n 请重试!");
    }

#else
    QMessageBox* box = new QMessageBox(QMessageBox::Information, tr("提示"), tr("已完成诊断!"));
    box->setAttribute(Qt::WA_DeleteOnClose);

    QTimer::singleShot(1000, box, SLOT(close()));
    box->show();
#endif // DB_SEND

    ui->line_other->clear();

    emit signal_unblockThread();
}

void MicroSlideWindow::on_action_filetest_triggered()
{
    if (ui->widget_camera->m_hDevice < 0)
    {
        QMessageBox::warning(this, "警告", "未连接FPGA设备");
        return;
    }
    if (ft == nullptr)
    {
        ft = new FileTest();
        ft->setAttribute(Qt::WA_DeleteOnClose);
        connect(ft, &FileTest::destroyed, [this] { ft = nullptr; });
        connect(ft, &FileTest::signal_testSingleImage, this->ui->widget_camera, &bgCamera::slot_handleFileTest);
        ft->show();
    }
    else
    {
        ft->show();
    }
}
