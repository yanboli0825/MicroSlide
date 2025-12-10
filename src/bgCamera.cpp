#include "BgCamera.h"

#include "CamImgPool.h"
#include "IniParser.h"
#include "Logger.h"
#include "OnnxDeployer.h"
#include "hpec_lib.h"
#include "tmagnifydetect.h"
#include "utils.h"

#include <QCamera>
#include <QCheckBox>
#include <QDir>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QMenu>
#include <QMessageBox>
#include <QMetaType>
#include <QPushButton>
#include <QSlider>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QVideoFrame>
#include <QVideoSink>
#include <chrono>
#include <iostream>
#include <memory>
#include <time.h>

std::mutex send_lock;
std::mutex g_featureLock;

Q_DECLARE_METATYPE(std::shared_ptr<BestImage>);
Q_DECLARE_METATYPE(std::shared_ptr<AIResult>);

const std::map<std::string, std::vector<std::string>> bgCamera::slicePartToClassNamesMap = {
    {SLICESOURCE_STOMACH, CLSNAME_STOMACH},
    {SLICESOURCE_GUT, CLSNAME_GUT},
    {SLICESOURCE_PROSTATE, CLSNAME_PROSTATE},
    {SLICESOURCE_UNKNOWN, CLSNAME_KNOWN},
    {SLICESOURCE_DEFAULT, CLSNAME_DEFAULT}};

bgCamera::bgCamera(QWidget* parent)
    : QWidget(parent)
    , m_hMicro(nullptr)
    , m_fpsTimer(new QTimer(this))
    , m_imgWidth(DEFUALT_WIDTH)
    , m_imgHeight(DEFUALT_HEIGHT)
    , m_pData(nullptr)
    , m_temp(BGCAM_TEMP_DEF)
    , m_tint(BGCAM_TINT_DEF)
    , m_saveImg(true)
    , m_busNum(0)
    , m_hDevice(-1)
    , m_numFreeThread(DEAL_THREAD_MAX_NUMS)
    , m_camera(nullptr)
    , m_imgNumToFPGA(0)
    , m_cameraFrameCnt(0)
    , m_microMagnification(0)
    , m_isTrans(false)
    , m_slideInfo(std::make_unique<PIS_RES>())
    , m_saveDir("")
    , m_validImg(new BestImage())
    , m_FPGADNA(0)
    , m_img_pool(new CImgPool())
    , m_lastSimilarity(SIMILARITY_RESET_VALUE)
{
    qRegisterMetaType<std::shared_ptr<BestImage>>("std::shared_ptr<BestImage>");
    qRegisterMetaType<std::shared_ptr<AIResult>>("std::shared_ptr<AIResult>");

    initUI();
    readIniSettings();
    setupConnections();

    initMagDetectThread();
    initThreadPool(DEAL_THREAD_MAX_NUMS);

    m_cbox_mag->setChecked(true);
}

bgCamera::~bgCamera()
{
    // freeSaveImgThread();
    if (m_img_pool)
    {
        delete m_img_pool;
        m_img_pool = nullptr;
    }
    freeMagDetectThread();

    if (m_hMicro)
    {
        Bgcam_Close(m_hMicro);
        m_hMicro = nullptr;
    }

    releaseThreadPool();

    // if (m_slideInfo)
    // {
    //     delete m_slideInfo;
    //     m_slideInfo = nullptr;
    // }

    if (m_fpsTimer)
    {
        delete m_fpsTimer;
        m_fpsTimer = nullptr;
    }
}

void bgCamera::initUI()
{
    QHBoxLayout* vertical_main = new QHBoxLayout(this);
    // 右半边网格布局
    QGridLayout* gmain_right = new QGridLayout(this);

    QGroupBox* gboxexp = new QGroupBox("曝光");
    {
        m_cbox_auto = new QCheckBox("自动曝光");
        m_cbox_auto->setEnabled(false);
        m_lbl_expoTarget = new QLabel("0");
        m_lbl_expoTime = new QLabel("0");
        m_lbl_expoGain = new QLabel("0");
        m_slider_expoTarget = new QSlider(Qt::Horizontal);
        m_slider_expoTime = new QSlider(Qt::Horizontal);
        m_slider_expoGain = new QSlider(Qt::Horizontal);
        m_slider_expoTarget->setEnabled(false);
        m_slider_expoTime->setEnabled(false);
        m_slider_expoGain->setEnabled(false);
        connect(m_cbox_auto, &QCheckBox::stateChanged, this,
                [this](bool state)
                {
                    if (m_hMicro)
                    {
                        Bgcam_put_AutoExpoEnable(m_hMicro, state ? 1 : 0);
                        m_slider_expoTarget->setEnabled(state);
                        m_slider_expoTime->setEnabled(!state);
                        m_slider_expoGain->setEnabled(!state);
                        // unsigned short get_target;
                        // Bgcam_get_AutoExpoTarget(m_hMicro, &get_target);
                        // cout << "get_target: " << get_target << endl;
                        /*unsigned short _target = 120;
                        Bgcam_put_AutoExpoTarget(m_hMicro, _target);*/
                    }
                });

        connect(m_slider_expoTarget, &QSlider::valueChanged, this,
                [this](int value)
                {
                    if (m_hMicro)
                    {
                        m_lbl_expoTarget->setText(QString::number(value));
                        if (m_cbox_auto->isChecked())
                        {
                            Bgcam_put_AutoExpoTarget(m_hMicro, value);
                        }
                    }
                });
        connect(m_slider_expoTime, &QSlider::valueChanged, this,
                [this](int value)
                {
                    if (m_hMicro)
                    {
                        m_lbl_expoTime->setText(QString::number(value));
                        if (!m_cbox_auto->isChecked())
                            Bgcam_put_ExpoTime(m_hMicro, value * 1000);
                    }
                });
        connect(m_slider_expoGain, &QSlider::valueChanged, this,
                [this](int value)
                {
                    if (m_hMicro)
                    {
                        m_lbl_expoGain->setText(QString::number(value));
                        if (!m_cbox_auto->isChecked())
                            Bgcam_put_ExpoAGain(m_hMicro, value);
                    }
                });

        QVBoxLayout* v = new QVBoxLayout(gboxexp);
        v->addWidget(m_cbox_auto);
        v->addLayout(makeLayout3(new QLabel("曝光目标:"), m_slider_expoTarget, m_lbl_expoTarget,
                                 new QLabel("曝光时间(ms):"), m_slider_expoTime, m_lbl_expoTime, new QLabel("增益(%):"),
                                 m_slider_expoGain, m_lbl_expoGain));
        // gboxexp->setLayout(v);
    } // 曝光box

    QGroupBox* gboxwb = new QGroupBox("白平衡");
    {
        m_btn_defaultWB = new QPushButton("默认值");
        m_btn_defaultWB->setEnabled(false);
        connect(m_btn_defaultWB, &QPushButton::clicked, this,
                [this]()
                {
                    Bgcam_put_TempTint(m_hMicro, BG_TEMP, BG_TINT);
                    // 设为默认值
                    m_slider_temp->setValue(BG_TEMP);
                    m_slider_tint->setValue(BG_TINT);
                });
        m_btn_autoWB = new QPushButton("白平衡");
        m_btn_autoWB->setEnabled(false);
        connect(m_btn_autoWB, &QPushButton::clicked, this,
                [this]()
                {
                    // 自动白平衡函数
                    Bgcam_AwbOnce(m_hMicro, nullptr, nullptr);
                    // 读取设置的白平衡参数
                    int _get_int, _get_temp;
                    Bgcam_get_TempTint(m_hMicro, &_get_temp, &_get_int);
                    // 更改参数到滑动条
                    m_slider_temp->setValue(_get_temp);
                    m_slider_tint->setValue(_get_int);
                });
        m_lbl_temp = new QLabel(QString::number(BGCAM_TEMP_DEF));
        m_lbl_tint = new QLabel(QString::number(BGCAM_TINT_DEF));
        m_slider_temp = new QSlider(Qt::Horizontal);
        m_slider_tint = new QSlider(Qt::Horizontal);
        m_slider_temp->setRange(BGCAM_TEMP_MIN, BGCAM_TEMP_MAX);
        m_slider_temp->setValue(BGCAM_TEMP_DEF);
        m_slider_tint->setRange(BGCAM_TINT_MIN, BGCAM_TINT_MAX);
        m_slider_tint->setValue(BGCAM_TINT_DEF);
        m_slider_temp->setEnabled(false);
        m_slider_tint->setEnabled(false);
        connect(m_slider_temp, &QSlider::valueChanged, this,
                [this](int value)
                {
                    m_temp = value;
                    if (m_hMicro)
                        Bgcam_put_TempTint(m_hMicro, m_temp, m_tint);
                    m_lbl_temp->setText(QString::number(value));
                });
        connect(m_slider_tint, &QSlider::valueChanged, this,
                [this](int value)
                {
                    m_tint = value;
                    if (m_hMicro)
                        Bgcam_put_TempTint(m_hMicro, m_temp, m_tint); // 设置白平衡的函数
                    m_lbl_tint->setText(QString::number(value));
                });

        QVBoxLayout* v = new QVBoxLayout(gboxwb);
        QHBoxLayout* v2 = new QHBoxLayout(gboxwb);
        m_btn_defaultWB->setMaximumWidth(100);
        m_btn_autoWB->setMaximumWidth(100);
        v2->addWidget(m_btn_defaultWB);
        v2->addWidget(m_btn_autoWB);
        v2->addStretch();
        v->addLayout(
            makeLayout(new QLabel("色温:"), m_slider_temp, m_lbl_temp, new QLabel("色调:"), m_slider_tint, m_lbl_tint));
        v->addLayout(v2);
        // gboxwb->setLayout(v);
    } // 白平衡box

    // 按钮布局
    {
        m_btn_open = new QPushButton("打开显微镜");
        connect(m_btn_open, &QPushButton::clicked, this, &bgCamera::onBtnOpenClicked);

        m_btn_connect = new QPushButton("连接FPGA");
        connect(m_btn_connect, &QPushButton::clicked, this, &bgCamera::onBtnConnectClicked);

        m_btn_trans = new QPushButton("开始传输");
        connect(m_btn_trans, &QPushButton::clicked, this, &bgCamera::onBtnTransClicked);

        m_cbox_mag = new QCheckBox("倍率检测");
        connect(m_cbox_mag, &QCheckBox::checkStateChanged, this, &bgCamera::slot_cbox_magDetectStateChanged);

        m_cbox_save = new QCheckBox("存储入库");
        m_cbox_save->setChecked(true);
        connect(m_cbox_save, &QCheckBox::checkStateChanged, this, &bgCamera::slot_cbox_saveStateChanged);
        {
            QVBoxLayout* v = new QVBoxLayout(this);
            v->addWidget(gboxexp);
            v->addWidget(gboxwb);
            m_btn_open->setMaximumWidth(100);
            m_btn_connect->setMaximumWidth(100);
            m_btn_trans->setMaximumWidth(100);
            QWidget* btn_widget = new QWidget(this);
            QHBoxLayout* hlyt = new QHBoxLayout(btn_widget);
            hlyt->addWidget(m_btn_open);
            hlyt->addWidget(m_btn_connect);
            hlyt->addWidget(m_btn_trans);
            hlyt->addStretch();
            QWidget* cbox_widget = new QWidget(this);
            QHBoxLayout* hlyt2 = new QHBoxLayout(cbox_widget);
            hlyt2->addWidget(m_cbox_mag);
            hlyt2->addWidget(m_cbox_save);
            hlyt2->addStretch();
            v->addWidget(btn_widget);
            v->addWidget(cbox_widget);
            v->addStretch();
            gmain_right->addLayout(v, 0, 0);
            v->setAlignment(Qt::AlignHCenter);
            QSpacerItem* horizontalSpacer = new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);
            gmain_right->addItem(horizontalSpacer, 1, 0);
            gmain_right->setAlignment(Qt::AlignHCenter);
        }
    }

    {
        m_lbl_fps = new QLabel();
        m_lbl_debug = new QLabel();
        // 显示视频帧
        m_lbl_video = new QLabel("Video");
        m_lbl_video->setObjectName("lbl_video");
        m_lbl_video->setStyleSheet("#lbl_video{  border: 1px solid black; \
                                    border-radius: 2px;\
                                    font:bold 82px;\
                                    color:#bdbebd;}");
        m_lbl_video->setMinimumHeight(540);
        m_lbl_video->setMaximumHeight(660);
        m_lbl_video->setMinimumWidth(800); // wll--height/width
        m_lbl_video->setMaximumWidth(1000);
        QVBoxLayout* v = new QVBoxLayout(this);
        v->addWidget(m_lbl_video, 2);
        QHBoxLayout* v2 = new QHBoxLayout(this);
        v2->addWidget(m_lbl_fps);
        v2->addWidget(m_lbl_debug);
        v2->addStretch();
        v->addLayout(v2);
        v->addStretch();
        vertical_main->addLayout(v, 5);
    }

    vertical_main->addLayout(gmain_right, 1);
    vertical_main->setSpacing(15);
    // 最终布局
    setLayout(vertical_main);
}

void bgCamera::readIniSettings()
{
    QString iniPath = INIT_FILE_PATH;
    QString groupName = GROUP_NAME;

    // 读取系统设置
    m_saveRoot = IniParser::readIniSettings(iniPath, groupName, "FilePath");

    m_sharpThres = IniParser::readIniSettings(iniPath, groupName, "Clarity").toInt();
    m_similarityThres = IniParser::readIniSettings(iniPath, groupName, "Similarity").toInt();
    m_areaThres = IniParser::readIniSettings(iniPath, groupName, "Area").toInt();

    m_dbAddr = IniParser::readIniSettings(iniPath, groupName, "DBAddr").toStdString();
    m_dbPort = IniParser::readIniSettings(iniPath, groupName, "DBPort").toInt();

    m_FPGADNA = IniParser::readIniSettings(iniPath, groupName, "FPGADNA").toLower().remove("0x").toUInt(nullptr, 16);
}

void bgCamera::setupConnections()
{
    // 回调函数
    connect(this, &bgCamera::evtCallback, this,
            [this](unsigned nEvent)
            {
                /* this run in the UI thread */
                if (m_hMicro)
                {
                    if (nEvent == BGCAM_EVENT_IMAGE)
                    {
                        handleVideoStreamEvent();
                    }
                    else if (nEvent == BGCAM_EVENT_EXPOSURE)
                        handleExpoEvent();
                    else if (nEvent == BGCAM_EVENT_TEMPTINT)
                        handleTempTintEvent();
                    else if (nEvent == BGCAM_EVENT_ERROR)
                    {
                        releaseMicro();
                        LOGGER_ERROR("Microscope generic error");
                        QMessageBox::critical(this, "错误", tr("图形错误."));
                    }
                    else if (nEvent == BGCAM_EVENT_DISCONNECTED)
                    {
                        releaseMicro();
                        LOGGER_ERROR("Microscope disconnected");
                        QMessageBox::critical(this, "错误", tr("显微镜断开连接"));
                    }
                }
            });

    // 计帧数
    connect(m_fpsTimer, &QTimer::timeout, this,
            [this]()
            {
                unsigned nFrame = 0, nTime = 0, nTotalFrame = 0;
                if (m_hMicro && SUCCEEDED(Bgcam_get_FrameRate(m_hMicro, &nFrame, &nTime, &nTotalFrame)) && (nTime > 0))
                {
                    double fps = nFrame * 1000.0 / nTime;
                    m_lbl_fps->setText(QString::asprintf("总帧数 = %u, 帧率 = %.1f", nTotalFrame, fps));
                }
                else
                {
                    LOGGER_WARN("Failed to get frame rate");
                }
            });
}

void bgCamera::initMagDetectThread()
{
    LOGGER_INFO("Initializing magnification detection thread...");

    m_mySink = new QVideoSink(this);
    m_captureSession = new QMediaCaptureSession(this);

    m_magDetThread = new QThread;
    m_magDetImage = new TMagDetImage();
    m_magDetWorker = new TMagnifyDetect(m_magDetImage);
    m_magDetWorker->moveToThread(m_magDetThread);
    connect(m_magDetThread, &QThread::finished, m_magDetWorker, &TMagnifyDetect::deleteLater);
    connect(m_magDetThread, &QThread::started, m_magDetWorker, &TMagnifyDetect::working);

    m_magDetThread->start();

    connect(m_mySink, &QVideoSink::videoFrameChanged, this, &bgCamera::slot_cameraFrameChanged);
}

void bgCamera::freeMagDetectThread()
{
    m_magDetWorker->stop();
    m_magDetWorker->close();
    m_magDetThread->quit();
    m_magDetThread->wait();

    delete m_magDetThread;
}

void bgCamera::setBusNum(const int busNum)
{
    m_busNum = busNum;
}

void bgCamera::releaseMicro()
{
    if (m_hMicro)
    {
        Bgcam_Close(m_hMicro);
        m_hMicro = nullptr;
    }
    delete[] m_pData;
    m_pData = nullptr;

    m_btn_open->setText("打开");
    m_fpsTimer->stop();
    m_lbl_fps->clear();
    m_lbl_debug->clear();
    m_lbl_video->setText("video");
    m_cbox_auto->setEnabled(false);
    m_slider_expoTarget->setEnabled(false);
    m_slider_expoGain->setEnabled(false);
    m_slider_expoTime->setEnabled(false);
    m_btn_autoWB->setEnabled(false);
    m_btn_defaultWB->setEnabled(false);
    m_slider_temp->setEnabled(false);
    m_slider_tint->setEnabled(false);
}

void bgCamera::closeEvent(QCloseEvent*)
{
    releaseMicro();
}

void bgCamera::initMicro()
{
    // 通过设备id号打开显微镜，获得显微镜句柄
    m_hMicro = Bgcam_Open(m_microDevice.id);

    // 设置默认分辨率
    Bgcam_put_Size(m_hMicro, DEFUALT_WIDTH, DEFUALT_HEIGHT); // 设置分辨率
    Bgcam_put_Option(m_hMicro, BGCAM_OPTION_BYTEORDER, 0);   // 设置字节序为RGB
    Bgcam_put_AutoExpoEnable(m_hMicro, 1);                   // 自动曝光使能

    // 设置锐化：0-500
    int threshold = 0;
    int radius = 2;
    int strength = 350;
    int iValue = (threshold << 24) | (radius << 16) | (strength);
    Bgcam_put_Option(m_hMicro, BGCAM_OPTION_SHARPENING, iValue); // 锐化

    if (m_pData)
    {
        delete[] m_pData;
        m_pData = nullptr;
    }

    m_pData = new uchar[TDIBWIDTHBYTES(m_imgWidth * 24) * m_imgHeight];

    unsigned uimax = 0, uimin = 0, uidef = 0;
    unsigned short usmax = 0, usmin = 0, usdef = 0;
    Bgcam_get_ExpTimeRange(m_hMicro, &uimin, &uimax, &uidef); // 获取相机所能使用的最大、最小以及默认的曝光时间

    m_slider_expoTarget->setRange(BGCAM_AETARGET_MIN, BGCAM_AETARGET_MAX);
    m_slider_expoTime->setRange(uimin / 1000, uimax / 1000); // 这里获取的单位是ns，而显示的是ms

    Bgcam_get_ExpoAGainRange(m_hMicro, &usmin, &usmax, &usdef); // 获取相机所能使用的最大、最小以及默认的增益
    m_slider_expoGain->setRange(usmin, usmax);

    if (0 == (m_microDevice.model->flag & BGCAM_FLAG_MONO))
    {
        handleTempTintEvent();
    }

    handleExpoEvent();

    if (SUCCEEDED(Bgcam_StartPullModeWithCallback(m_hMicro, eventCallBack, this)))
    {
        m_cbox_auto->setEnabled(true);
        m_btn_autoWB->setEnabled(0 == (m_microDevice.model->flag & BGCAM_FLAG_MONO));
        m_btn_defaultWB->setEnabled(0 == (m_microDevice.model->flag & BGCAM_FLAG_MONO));
        m_slider_temp->setEnabled(0 == (m_microDevice.model->flag & BGCAM_FLAG_MONO));
        m_slider_tint->setEnabled(0 == (m_microDevice.model->flag & BGCAM_FLAG_MONO));

        int bAuto = 0;
        Bgcam_get_AutoExpoEnable(m_hMicro, &bAuto); // 将自动曝光设置为False
        m_cbox_auto->setChecked(bAuto == 1);

        m_fpsTimer->start(1000);

        m_btn_open->setText("关闭显微镜");

        LOGGER_INFO("Microscope initialized successfully");
    }
    else
    {
        releaseMicro();
        LOGGER_ERROR("Failed to initialize microscope");
        QMessageBox::warning(this, tr("错误"), tr("初始化显微镜失败"));
    }
}

void bgCamera::onBtnOpenClicked()
{
    if (m_hMicro)
    {
        // 当前已打开显微镜，关闭显微镜
        releaseMicro();
        // 断开传输
        if (m_isTrans)
        {
            m_isTrans = false;
            m_btn_trans->setText("开始传输");
            // m_saveImgWorkwer->stop();
        }
    }
    else
    {
        // 枚举显微镜设备
        BgcamDeviceV2 arr[BGCAM_MAX] = {{0}};
        unsigned count = Bgcam_EnumV2(arr);
        if (count == 0)
        {
            // 未发现显微镜
            LOGGER_ERROR("No microscope found, please connect device or check driver");
            QMessageBox::critical(this, tr("错误"), tr("未发现显微镜，请连接设备或检查驱动"));
        }
        else if (count == 1)
        {
            // 只有一台显微镜
            m_microDevice = arr[0];

            initMicro();
        }
        else
        {
            // 发现多台显微镜
            QMenu menu;
            for (unsigned i = 0; i < count; ++i)
            {
                menu.addAction(
#if defined(_WIN32)
                    QString::fromWCharArray(arr[i].displayname)
#else
                    arr[i].displayname
#endif
                        ,
                    this,
                    [this, i, arr](bool)
                    {
                        m_microDevice = arr[i];
                        initMicro();
                    });
            }
            // menu.exec(mapToGlobal(m_btn_snap->pos()));
        }
    }
}

void bgCamera::onBtnConnectClicked()
{
    if (m_hDevice == -1)
    {
        // 连接设备
        m_hDevice = OpenDevice(m_busNum);
        LOGGER_DEBUG("Opened device handle: ", m_hDevice);

        if (m_hDevice < 0)
        {
            // 连接失败
            m_hDevice = -1;
            LOGGER_ERROR("Failed to connect FPGA, please check the bus or driver");
            QMessageBox::critical(this, "错误", "驱动异常，请检查通路！");
            return;
        }
        else
        {
            // 连接成功
            m_btn_connect->setText("断开设备");

            // 初始化设备
            InitDevice(m_hDevice);
            LOGGER_INFO("Device connected successfully!");

            // 软件清零FPGA，防止硬件出现传输错误
            unsigned int reg_base = 0x6000;    // 寄存器基址
            unsigned int reg_offset3 = 21 * 4; // 软件清零
            unsigned int reg_offset7 = 23 * 4; // 软件验证码

            int ret = 0;
            ret = write_19eg_reg(m_hDevice, reg_base + reg_offset3, 0x00000001);
            ret = write_19eg_reg(m_hDevice, reg_base + reg_offset3, 0x00000000);

            if (ret == W_REG_OK)
            {
                LOGGER_DEBUG("Succeed to clear FPGA when connecting");
            }
            else
            {
                QMessageBox::critical(this, "错误", "FPGA清零失败");
                LOGGER_ERROR("Failed to clear FPGA when connecting");
                releaseFPGA();
                return;
            }

            ret = write_19eg_reg(m_hDevice, reg_base + reg_offset7, m_FPGADNA); // FPGA DNA, e.g. 40070524 | 90042762

            if (ret == W_REG_OK)
            {
                LOGGER_DEBUG("Succeed to write FPGA DNA when connecting");
            }
            else
            {
                QMessageBox::critical(this, "错误", "写入FPGA DNA失败");
                LOGGER_ERROR("Failed to write FPGA DNA when connecting");
                releaseFPGA();
                return;
            }
        }
    }
    else
    {
        releaseFPGA();

        // 断开传输
        m_isTrans = false;
        m_btn_trans->setText("开始传输");
        // m_saveImgWorkwer->stop();
    }
}

bool bgCamera::onBtnTransClicked()
{
    if (m_isTrans == false)
    {
        // 开始传输
        if (m_hMicro && m_hDevice != -1)
        {
            if (m_saveDir == "")
            {
                emit directly_click_btn_trans();
            }
            m_isTrans = true;
            m_btn_trans->setText("停止传输");

            LOGGER_INFO("Start image transfer.");
            return true;
        }
        else
        {
            LOGGER_WARN("Please open the microscope and connect the FPGA device first.");
            QMessageBox::warning(this, "警告", "请先打开显微镜并连接FPGA设备");
            return false;
        }
    }
    else
    {
        m_isTrans = false;
        m_btn_trans->setText("开始传输");
        LOGGER_INFO("Stop image transfer.");
        return true;
    }
}

void bgCamera::releaseFPGA()
{
    if (m_hDevice < 0)
    {
        // 未获取句柄，直接返回
        return;
    }
    else
    {
        // 释放设备资源
        CloseDevice(m_hDevice);
        m_hDevice = -1;

        m_btn_connect->setText("连接FPGA");
        LOGGER_INFO("Disconnect FPGA device.");
    }
}

void bgCamera::slot_cbox_saveStateChanged(const Qt::CheckState& arg1)
{
    if (arg1 == Qt::CheckState::Checked)
    {
        m_saveImg = true;
    }
    else
    {
        m_saveImg = false;
    }
}

void bgCamera::slot_cbox_magDetectStateChanged(const Qt::CheckState& arg1)
{
    if (arg1 == Qt::CheckState::Checked)
    {
        // 检测设备
        const QList<QCameraDevice> videoDevices = QMediaDevices::videoInputs();
        for (const QCameraDevice& device : videoDevices)
        {
            if (device.description() == MAGNIFICATION_CAMERA_ID)
            {
                if (m_camera == nullptr)
                {
                    m_camera = new QCamera(device);
                    LOGGER_INFO("bind camera: {}", device.description().toStdString());
                }
                else
                {
                    LOGGER_WARN("Camera has been bound already.");
                }
            }
        }
        if (m_camera)
        {
            m_captureSession->setCamera(m_camera);
            m_captureSession->setVideoSink(m_mySink);
            m_camera->start();
            // working start
            m_magDetWorker->start();
        }
        else
        {
            LOGGER_WARN("Not find camera.");
        }
    }
    else
    {
        // destroy thread
        delete m_camera;
        m_camera = nullptr;
        m_magDetWorker->stop();
    }
}

void bgCamera::eventCallBack(unsigned nEvent, void* pCallbackCtx)
{
    bgCamera* pThis = reinterpret_cast<bgCamera*>(pCallbackCtx);
    // 发出事件回调的Signal
    emit pThis->evtCallback(nEvent);
}

bool bgCamera::getMetricsFromFPGA(const uchar* _pData, unsigned int& rsharp, unsigned int& rframe,
                                  unsigned int& rsimilarity, unsigned int& rarea)
{
    // pcie最小数据传输单位是*8M，适应图像尺寸以及包头512位(64字节)
    int imgSize = TDIBWIDTHBYTES(DEFUALT_WIDTH * 24 * DEFUALT_HEIGHT);
    const int PCIE_SIZE = 64 + imgSize;

    char* down_buf = static_cast<char*>(malloc(PCIE_SIZE));
    if (down_buf == nullptr)
    {
        LOGGER_ERROR("malloc() down_buf failed, out of memory.");
        exit(1);
    }

    memset(down_buf, 0, PCIE_SIZE);

    // 定义包头
    unsigned int frame_flag = 0x12345678;
    unsigned int frame_length = imgSize / 3;
    unsigned int frame_num = m_imgNumToFPGA; // 这是传给FPGA的总图片数，不是每张图在slide中的帧号
    down_buf[12] = frame_flag & 0xff;
    down_buf[13] = (frame_flag >> 8) & 0xff;
    down_buf[14] = (frame_flag >> 16) & 0xff;
    down_buf[15] = (frame_flag >> 24) & 0xff;
    down_buf[8] = frame_length & 0xff;
    down_buf[9] = (frame_length >> 8) & 0xff;
    down_buf[10] = (frame_length >> 16) & 0xff;
    down_buf[11] = (frame_length >> 24) & 0xff;
    down_buf[0] = frame_num & 0xff;
    down_buf[1] = (frame_num >> 8) & 0xff;
    down_buf[2] = (frame_num >> 16) & 0xff;
    down_buf[3] = (frame_num >> 24) & 0xff;

    // 倍率包头
    unsigned int mag = m_microMagnification;
    down_buf[16] = mag & 0xff;
    down_buf[17] = (mag >> 8) & 0xff;
    down_buf[18] = (mag >> 16) & 0xff;
    down_buf[19] = (mag >> 24) & 0xff;

    // 切片种类包头
    // TODO: 这里后续根据数据库传来的信息修改
    // unsigned int slide_class = m_slideInfo->staining.toUInt();
    unsigned int slide_class = 1;
    down_buf[20] = slide_class & 0xff;
    down_buf[21] = (slide_class >> 8) & 0xff;
    down_buf[22] = (slide_class >> 16) & 0xff;
    down_buf[23] = (slide_class >> 24) & 0xff;

    // 后512位都是像素信息
    memcpy(down_buf + 64, _pData, imgSize);

    ULONG n_send = 0; // 发送数据有效长度

    unsigned long long total_size = 0; // 一帧传送的字节数
    unsigned int reg_offset = 11 * 4;  // 寄存器地址偏移量，清晰度
    unsigned int reg_offset1 = 13 * 4; // 相似度
    unsigned int reg_offset2 = 14 * 4; // 帧号
    unsigned int reg_offset3 = 21 * 4; // 软件清零
    unsigned int reg_offset4 = 12 * 4; // 有效面积
    unsigned int reg_offset5 = 15 * 4; // 硬件接收图片报错

    // 红绿色差阈值，和有效面积成反比，默认是40，区间1~255；若要使用这个寄存器，注意首位得是1。//=0x8000_0000
    unsigned int reg_offset6 = 22 * 4;
    // | 红绿色差阈值；
    unsigned int reg_offset7 = 23 * 4; // 软件端验证码，90042762
    unsigned int reg_base = 0x6000;    // 寄存器基址

    unsigned int ret = WriteDataToFpga(m_hDevice, down_buf, PCIE_SIZE, FPGA_CHANNEL, 0, n_send, 100);

    // 若写超时则返回失败
    if (ret == -1)
    {
        LOGGER_ERROR("WriteDataToFpga: 等待超时.");
        return false;
    }

    if (n_send != 6654016)
    {
        // 将FPGA清零，防止硬件出现传输错误
        unsigned int reg_base = 0x6000;    // 寄存器基址
        unsigned int reg_offset3 = 21 * 4; // 软件清零

        if (write_19eg_reg(m_hDevice, reg_base + reg_offset3, 0x00000001) != W_REG_OK)
        {
            LOGGER_ERROR("Failed to write FPGA clear register.");
        }
        if (write_19eg_reg(m_hDevice, reg_base + reg_offset3, 0x00000000) != W_REG_OK)
        {
            LOGGER_ERROR("Failed to write FPGA clear register.");
        }

        LOGGER_ERROR("Abnormal number of valid bytes sent to FPGA: {}", n_send);
        return false;
    }

    free(down_buf);

    // 等待FPGA处理完成
    Sleep(5);

    // 读返回结果
    if (read_19eg_reg(m_hDevice, reg_base + reg_offset, rsharp) != R_REG_OK)
    {
        return false;
    }
    if (read_19eg_reg(m_hDevice, reg_base + reg_offset1, rsimilarity) != R_REG_OK)
    {
        return false;
    }
    if (read_19eg_reg(m_hDevice, reg_base + reg_offset2, rframe) != R_REG_OK)
    {
        return false;
    }
    if (read_19eg_reg(m_hDevice, reg_base + reg_offset4, rarea) != R_REG_OK)
    {
        return false;
    }
    return true;
}

void bgCamera::getValidImage(const QImage& image, const unsigned int frame, const unsigned int clear,
                             const unsigned int normClear, const unsigned int similarity, const unsigned int area)
{
    // 预定义筛选条件
    unsigned int _similarity = m_similarityThres; // 相似度阈值
    unsigned int _sharp = m_sharpThres;           // 清晰度阈值
    unsigned int _area = m_areaThres;             // 愉快面积阈值

    // 对图片进行条件筛选，若符合筛选条件且有空闲的节点时，将图片挂载到deal链表上进行AI处理；否则丢图，直接返回
    // 场景1：视野静止（相似度 <= 阈值）
    if ((m_lastSimilarity > _similarity && similarity <= _similarity) ||
        (m_lastSimilarity <= _similarity && similarity <= _similarity))
    {
        m_lastSimilarity = similarity;

        // 同一静止位置最多保存 MAX_STATIC_VIEW_IMAGES 张图片
        if (m_staticViewImgNum < MAX_STATIC_VIEW_IMAGES)
        {
            if (m_similarImgNum < SIMILAR_FRAME_COUNT)
            {
                m_similarImgNum++;
                // 筛选最优图片
                if (normClear > _sharp && area > _area && normClear > m_validImg->sharpness)
                {
                    // 预览图片
                    emit signal_showPreviewImg(QPixmap::fromImage(image), m_validImgNum + 1);

                    QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");

                    // 确保释放之前记录图片所用的内存
                    if (m_validImg)
                    {
                        delete m_validImg;
                        m_validImg = nullptr;
                    }
                    m_validImg = new BestImage(image.copy(), timeStamp, m_validImgNum, frame, clear, similarity, area,
                                               normClear, m_microMagnification, m_saveImg);
                }
            }

            // 累积到SIMILAR_FRAME_COUNT帧后，保存最优图片
            if (m_similarImgNum == 5)
            {
                m_similarImgNum = 0;
                // 检查图片有效性
                if (m_validImg->sharpness != 0)
                {
                    HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();
                    if (img_node == NULL)
                    {
                        LOGGER_DEBUG("No free node in free list, lost image: {}", m_validImg->total_image_count);

                        m_validImg->clear();
                        return;
                    }
                    m_validImgNum++;
                    m_staticViewImgNum++;
                    // signal_showPreviewImg(QPixmap::fromImage(m_validImg->image.copy()), m_validImgNum);
                    signal_showPreviewImg(QPixmap::fromImage(m_validImg->image), m_validImgNum);

                    img_node->best_image = BestImage(m_validImg);

                    img_node->best_image.image_num = m_validImgNum;

                    // 填充好内容后，将结点挂载到used_list链表上，由子线程处理
                    m_img_pool->fill_deal_mem_pool(img_node);
                    img_node = NULL;
                    m_validImg->clear();
                }
            }
        }
        else
        {
            m_validImg->clear();
        }
    }
    // 场景2：从静止到移动（相似度从 <= 阈值变为 > 阈值）
    else if (m_lastSimilarity <= _similarity && similarity > _similarity)
    {
        m_lastSimilarity = similarity;

        // 先保存静止时累积的最优图片

        if (m_validImg->sharpness != 0)
        {
            m_similarImgNum = 0;

            if (m_staticViewImgNum < MAX_STATIC_VIEW_IMAGES)
            {
                m_staticViewImgNum = 0;
                HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();

                if (img_node == NULL)
                {
                    LOGGER_DEBUG("No free node in free list, lost image: {}", m_validImg->total_image_count);
                    m_validImg->clear();
                }
                else
                {
                    m_validImgNum++;
                    signal_showPreviewImg(QPixmap::fromImage(m_validImg->image.copy()), m_validImgNum);

                    img_node->best_image = BestImage(m_validImg);
                    img_node->best_image.image_num = m_validImgNum;

                    // 填充好内容后，将结点挂载到used_list链表上，由子线程处理
                    m_img_pool->fill_deal_mem_pool(img_node);
                    img_node = NULL;
                    m_validImg->clear();
                }
            }
            else
            {
                m_validImg->clear();
            }
        }
        else
        {
            m_similarImgNum = 0;
            m_staticViewImgNum = 0;
        }

        // 检查当前帧是否也符合质量要求
        if (normClear > _sharp && area > _area)
        {
            HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();
            if (img_node == NULL)
            {
                LOGGER_DEBUG("No free node in free list, lost image: {}", m_validImg->total_image_count);
                m_validImg->clear();
                return;
            }
            m_validImgNum++;
            signal_showPreviewImg(QPixmap::fromImage(image.copy()), m_validImgNum);

            // 确保释放之前记录图片所用的内存
            if (m_validImg)
            {
                delete m_validImg;
                m_validImg = nullptr;
            }
            QString time_stamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
            m_validImg = new BestImage(image.copy(), time_stamp, m_validImgNum, frame, clear, similarity, area,
                                       normClear, m_microMagnification, m_saveImg);

            img_node->best_image = BestImage(m_validImg);

            // 填充好内容后，将结点挂载到used_list链表上，由子线程处理
            m_img_pool->fill_deal_mem_pool(img_node);
            img_node = NULL;
            m_validImg->clear();
        }
    }
    // 场景3：视野快速移动（相似度持续 > 阈值）
    else if (m_lastSimilarity > _similarity && similarity > _similarity)
    {
        m_lastSimilarity = similarity;

        if (normClear > _sharp && area > _area)
        {
            HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();
            if (img_node == NULL)
            {
                LOGGER_DEBUG("No free node in free list, lost image: {}", m_validImg->total_image_count);
                m_validImg->clear();

                return;
            }
            m_validImgNum++;
            signal_showPreviewImg(QPixmap::fromImage(image.copy()), m_validImgNum);

            // 确保释放之前记录图片所用的内存
            if (m_validImg)
            {
                delete m_validImg;
                m_validImg = nullptr;
            }
            QString time_stamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
            m_validImg = new BestImage(image.copy(), time_stamp, m_validImgNum, frame, clear, similarity, area,
                                       normClear, m_microMagnification, m_saveImg);

            img_node->best_image = BestImage(m_validImg);

            // 填充好内容后，将结点挂载到used_list链表上，由子线程处理
            m_img_pool->fill_deal_mem_pool(img_node);
            img_node = NULL;
            m_validImg->clear();
        }
    }
}

void bgCamera::handleVideoStreamEvent()
{
    // 处理从相机过来的图片
    unsigned width = 0, height = 0;
    if (SUCCEEDED(Bgcam_PullImage(m_hMicro, m_pData, 24, &width, &height))) // 拉取图片，RGB格式
    {
        // 拷贝数据，防止竞态
        QImage image = QImage(m_pData, width, height, QImage::Format_RGB888).copy();

        // 将显微镜传来的图片缩放后显示
        QImage displayImg =
            image.scaled(m_lbl_video->width(), m_lbl_video->height(), Qt::KeepAspectRatio, Qt::FastTransformation);
        m_lbl_video->setPixmap(QPixmap::fromImage(displayImg));

        // 将视频流数据传输至FPGA
        if (m_isTrans)
        {
            // 倍率检测失败则直接返回，不进行后续处理
            if (m_microMagnification == 0)
            {
                return;
            }

            m_imgNumToFPGA++;
            handleSingleImage(image);
        }
        else
        {
            m_lastSimilarity = SIMILARITY_RESET_VALUE;
        }
    } // pull img success
}

void bgCamera::handleExpoEvent()
{
    unsigned time = 0;
    unsigned short gain = 0;
    unsigned short target = 0;
    Bgcam_get_AutoExpoTarget(m_hMicro, &target);
    Bgcam_get_ExpoTime(m_hMicro, &time);
    Bgcam_get_ExpoAGain(m_hMicro, &gain);
    {
        const QSignalBlocker blocker(m_slider_expoTarget);
        m_slider_expoTarget->setValue(int(target));
    }
    {
        const QSignalBlocker blocker(m_slider_expoTime);
        m_slider_expoTime->setValue(int(time) / 1000);
    }
    {
        const QSignalBlocker blocker(m_slider_expoGain);
        m_slider_expoGain->setValue(int(gain));
    }
    m_lbl_expoTarget->setText(QString::number(target));
    m_lbl_expoTime->setText(QString::number(time / 1000));
    m_lbl_expoGain->setText(QString::number(gain));
}

void bgCamera::handleTempTintEvent()
{
    int nTemp = 0, nTint = 0;
    if (SUCCEEDED(Bgcam_get_TempTint(m_hMicro, &nTemp, &nTint)))
    {
        {
            const QSignalBlocker blocker(m_slider_temp);
            m_slider_temp->setValue(nTemp);
        }
        {
            const QSignalBlocker blocker(m_slider_tint);
            m_slider_tint->setValue(nTint);
        }
        m_lbl_temp->setText(QString::number(nTemp));
        m_lbl_tint->setText(QString::number(nTint));
    }
}

QVBoxLayout* bgCamera::makeLayout(QLabel* lbl1, QSlider* sli1, QLabel* val1, QLabel* lbl2, QSlider* sli2, QLabel* val2)
{
    QHBoxLayout* hlyt1 = new QHBoxLayout();
    hlyt1->addWidget(lbl1);
    hlyt1->addStretch();
    hlyt1->addWidget(val1);
    QHBoxLayout* hlyt2 = new QHBoxLayout();
    hlyt2->addWidget(lbl2);
    hlyt2->addStretch();
    hlyt2->addWidget(val2);
    QVBoxLayout* vlyt = new QVBoxLayout();
    vlyt->addLayout(hlyt1);
    vlyt->addWidget(sli1);
    vlyt->addLayout(hlyt2);
    vlyt->addWidget(sli2);
    return vlyt;
}

QVBoxLayout* bgCamera::makeLayout3(QLabel* lbl1, QSlider* sli1, QLabel* val1, QLabel* lbl2, QSlider* sli2, QLabel* val2,
                                   QLabel* lbl3, QSlider* sli3, QLabel* val3)
{
    QHBoxLayout* hlyt1 = new QHBoxLayout();
    hlyt1->addWidget(lbl1);
    hlyt1->addStretch();
    hlyt1->addWidget(val1);
    QHBoxLayout* hlyt2 = new QHBoxLayout();
    hlyt2->addWidget(lbl2);
    hlyt2->addStretch();
    hlyt2->addWidget(val2);
    QHBoxLayout* hlyt3 = new QHBoxLayout();
    hlyt3->addWidget(lbl3);
    hlyt3->addStretch();
    hlyt3->addWidget(val3);
    QVBoxLayout* vlyt = new QVBoxLayout();
    vlyt->addLayout(hlyt1);
    vlyt->addWidget(sli1);
    vlyt->addLayout(hlyt2);
    vlyt->addWidget(sli2);
    vlyt->addLayout(hlyt3);
    vlyt->addWidget(sli3);
    return vlyt;
}

void bgCamera::slot_cameraFrameChanged(const QVideoFrame& frame)
{
    m_cameraFrameCnt++;

    if (m_cameraFrameCnt == 3)
    {
        m_cameraFrameCnt = 0;
        if (frame.isValid())
        {
            unique_lock<mutex> lk(m_magDetImage->m_lock, std::defer_lock);
            if (lk.try_lock())
            {
                m_magDetImage->m_image = frame.toImage();
                m_microMagnification = m_magDetImage->color;
            }
        }
    }
}

void imageInference(OnnxDeployer* deployer, bgCamera* bgcamera, const BestImage* best_image, AIResult* aiResult)
{
    // 将RGB QImage转换为cv::Mat格式
    cv::Mat _image = utils::QImageToMatRGB(best_image->image);

    // 裁剪patch
    std::pair<std::vector<cv::Mat>, std::vector<cv::Point>> pairs = deployer->extractPatches(_image);

    if (pairs.first.size() == 0)
    {
        aiResult->setValue(QString::fromStdString("invalid"), -1, QVector<float>{-1, -1, -1, -1, -1, -1},
                           cv::Point(0, 0));
        return;
    }

    // 提取特征向量
    std::vector<float> features = deployer->embedding(pairs.first);

    // 根据切片部位选择对应的AI模型进行推理
    std::pair<std::vector<float>, int> output = deployer->inference(features, bgcamera->m_slideInfo->slicesource);

    {
        std::lock_guard<std::mutex> lk(g_featureLock);
        bgcamera->m_video_features.reserve(bgcamera->m_video_features.size() + features.size());
        bgcamera->m_video_features.insert(bgcamera->m_video_features.end(), features.begin(), features.end());

        // 若是肠息肉切片且为10，20或40倍率，则存储MMR特征
        if (bgcamera->m_slideInfo->slicesource == SLICESOURCE_GUT)
        {
            if (best_image->magnification == 10 || best_image->magnification == 20 || best_image->magnification == 40)
            {
                bgcamera->m_mmr_features.reserve(bgcamera->m_mmr_features.size() + features.size());
                bgcamera->m_mmr_features.insert(bgcamera->m_mmr_features.end(), features.begin(), features.end());
            }
        }
    }

    // 将关键patch的坐标保存为yaml
    cv::Point keyPatchCoords = pairs.second[output.second];

    std::string key_str = std::to_string(best_image->image_num);
    cv::FileStorage fs(bgcamera->m_saveDir.toStdString() + "/" + key_str + ".yaml", cv::FileStorage::WRITE);
    fs << "coords" << keyPatchCoords;
    fs.release();

    // 搜索最大分类概率和最大概率对应的下标
    std::vector<float> cls_prob = output.first;

    int max_index = 0;
    float max_prob = 0;

    for (int i = 0; i < cls_prob.size(); ++i)
    {
        if (cls_prob[i] > max_prob)
        {
            max_prob = cls_prob[i];
            max_index = i;
        }
    }

    // 使用ostringstream来设置精度并去除尾随的零
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << cls_prob[max_index];

    // 根据切片部位获取疾病类型名称
    std::vector<std::string> cls_name;
    auto it = bgCamera::slicePartToClassNamesMap.find(bgcamera->m_slideInfo->slicesource);
    if (it != bgCamera::slicePartToClassNamesMap.end())
    {
        cls_name = it->second;
    }
    else
    {
        cls_name = bgCamera::slicePartToClassNamesMap.find(SLICESOURCE_DEFAULT)->second;
    }

    aiResult->setValue(QString::fromStdString(cls_name[max_index]), cls_prob[max_index],
                       QVector<float>(cls_prob.begin(), cls_prob.end()), keyPatchCoords);
}

std::pair<string, std::vector<float>> video_inference(OnnxDeployer* deployer, std::vector<float>& video_features,
                                                      const std::string& slice_part)
{
    std::vector<float> cls_prob = deployer->inference(video_features, slice_part).first;

    int max_index = 0;
    float max_prob = -1;

    // 搜索最大分类概率和最大概率对应的下表
    for (int i = 0; i < cls_prob.size(); ++i)
    {
        if (cls_prob[i] > max_prob)
        {
            max_prob = cls_prob[i];
            max_index = i;
        }
    }

    // 使用ostringstream来设置精度并去除尾随的零
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << cls_prob[max_index];

    std::vector<std::string> cls_name;
    auto it = bgCamera::slicePartToClassNamesMap.find(slice_part);
    if (it != bgCamera::slicePartToClassNamesMap.end())
    {
        cls_name = it->second;
    }
    else
    {
        cls_name = bgCamera::slicePartToClassNamesMap.find(SLICESOURCE_DEFAULT)->second;
    }

    std::string result = cls_name[max_index] + ", " + oss.str();

    return {result, cls_prob};
}

void processVideoStream(bgCamera* _bgcamera)
{
    CImgPool* mem_pool = _bgcamera->m_img_pool;
    HL_IMG_POOL_NODE* mem_node = nullptr;

    std::unique_ptr<OnnxDeployer> deployer = std::make_unique<OnnxDeployer>();

#ifdef FTP_SEND
    SOCKET dbClientSock;
    int dbConnectRet = dbSockInit(dbClientSock, _bgcamera->m_dbAddr, _bgcamera->m_dbPort);

    // 连接数据库失败则弹出警告
    if (dbConnectRet)
    {
        if (!_bgcamera->warning_shown.test_and_set())
        {
            emit _bgcamera->signal_showDBDisconnectWarning();
        }
    }
#endif // FTP_SEND

    while (_bgcamera->m_dealFlag == 1)
    {
        // 等待图片信号，没有信号的话短暂休眠后重试，避免空转
        mem_node = mem_pool->malloc_used_mem_pool();
        if (mem_node == NULL)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        // 进入AI处理线程，m_free_thread_num减一，表示空闲线程减少一个
        _bgcamera->m_numFreeThread--;

        std::shared_ptr<BestImage> best_image = std::make_shared<BestImage>(mem_node->best_image);

        // 读取完数据后，及时将used_list的结点挂载到free_list中，方便其他线程使用
        mem_pool->free_back_mem_pool(mem_node);
        mem_node = NULL;

        // 图片推理
        std::shared_ptr<AIResult> imgInferResult = std::make_shared<AIResult>();
        imageInference(deployer.get(), _bgcamera, best_image.get(), imgInferResult.get());

        // 若无有效小图（即切的patch的大小为0）则退出
        if (imgInferResult->diagnosis_res == "invalid")
        {
            LOGGER_DEBUG("This image is invalid, image num: {}", best_image->image_num);

            // 无有效小图时，将预览图的Loading信息改为invalid
            emit _bgcamera->signal_changePreviewImgToInvalid(best_image->image_num);

            // 退出AI处理线程，m_free_thread_num加一，表示空闲线程增加一个
            _bgcamera->m_numFreeThread++;

            continue;
        }

        QStringList temp_list =
            QString::number(std::round(imgInferResult->diagnosis_prob * 1000.f) / 1000.f).split(".");
        QString _img_inference_result = imgInferResult->diagnosis_res + "_" + temp_list.join("");

        // 视频诊断
        std::vector<float> videoFeatures = {};
        {
            std::lock_guard<std::mutex> lk(g_featureLock);
            videoFeatures = _bgcamera->m_video_features;
        }
        std::pair<std::string, std::vector<float>> video_diagnosis_result =
            video_inference(deployer.get(), videoFeatures, _bgcamera->m_slideInfo->slicesource);

        // 保存视频诊断结果
        {
            std::lock_guard<std::mutex> lk(send_lock);
            _bgcamera->m_video_res = video_diagnosis_result;
        }

        QString video_res = QString::fromStdString(video_diagnosis_result.first);

        // 判断是否要进行MMR基因预测
        if (_bgcamera->m_slideInfo->slicesource == SLICESOURCE_GUT && video_res.split(',')[0] == "C")
        {
            std::array<float, 4> mmr_result = deployer->mmr_predict(_bgcamera->m_mmr_features);
            QString MLH1_res;
            QString MSH2_res;
            QString MSH6_res;
            QString PMS2_res;
            float mmr_thres = 0.5;
            if (mmr_result[0] > mmr_thres)
            {
                MLH1_res = "+";
            }
            else
            {
                MLH1_res = "-";
            }
            if (mmr_result[1] > mmr_thres)
            {
                MSH2_res = "+";
            }
            else
            {
                MSH2_res = "-";
            }
            if (mmr_result[2] > mmr_thres)
            {
                MSH6_res = "+";
            }
            else
            {
                MSH6_res = "-";
            }
            if (mmr_result[3] > mmr_thres)
            {
                PMS2_res = "+";
            }
            else
            {
                PMS2_res = "-";
            }
            video_res = video_res + "(MLH1: " + MLH1_res + "  MSH2: " + MSH2_res + "  MSH6: " + MSH6_res +
                        "  PMS2: " + PMS2_res + ")";
        }

#ifdef FTP_SEND
        // 数据传输
        // 判断socket是否初始化成功
        if (!dbConnectRet)
        {
            std::vector<double> video_res_prob(video_diagnosis_result.second.begin(),
                                               video_diagnosis_result.second.end());
            std::string _video_res = video_res.toStdString();

            PATHO_RES video_res = {
                _bgcamera->m_slideInfo->pathological_id,           // 病理号
                _bgcamera->m_slideInfo->slice_id,                  // 切片号
                utils::wcharToString(_bgcamera->m_microDevice.id), // 传感器编号
                "1.0",                                             // 版本号
                static_cast<long int>(time(nullptr)),              // 诊断时间
                _bgcamera->m_slideInfo->pathological_order,        // 诊断次数
                best_image->image_num,                             // 一个切片号对应图片
                9,                                                 // 图片特征参数个数
                _bgcamera->m_slideInfo->pathological_order,        // 诊断次序
                _video_res,                                        // AI诊断结果
                video_res_prob,                                    // AI诊断概率
                "",                                                // 医生诊断结果,这里必须为空
                "A02",                                             // 包的类型
            };
            dbResultUpload(&video_res, dbClientSock);
        }
#endif // FTP_SEND

        // 数据落盘
        if (best_image->save == true)
        {
            QString fileName = QString::number(best_image->image_num) + ".png";
            QString saveDir = _bgcamera->m_saveDir;
            QString filePath = saveDir + "/" + fileName;

            // 判断图片是否保存成功
            if (best_image->image.save(filePath))
            {
                emit _bgcamera->signal_show_image(best_image, imgInferResult, video_res);

                std::array<float, 6> img_ai_prob;
                std::copy(imgInferResult->cls_prob.begin(), imgInferResult->cls_prob.end(), img_ai_prob.begin());
                std::string img_ai_res = (imgInferResult->diagnosis_res + "," +
                                          QString::number(std::round(imgInferResult->diagnosis_prob * 1000.f) / 1000.f))
                                             .toStdString();

                // 将当前图片的手工特征等信息存入json对象中。键为图片名
                {
                    std::lock_guard<std::mutex> lk(send_lock);
                    _bgcamera->m_imagesMetrics[fileName.toStdString()] = {
                        {"Frame", best_image->image_num},
                        {"Magnification", best_image->magnification},
                        {"Clarity", best_image->sharpness},
                        {"Similarity", best_image->similarity},
                        {"Effective Area", best_image->effective_area},
                        {"AI_Diagnosis", img_ai_res},
                        {"Normal_Clarity", best_image->normalized_sharpness},
                        {"Medical Diagnosis", "NA"},
                        {"AI_Probability", img_ai_prob},
                    };
                }
            }
            else
            {
                LOGGER_ERROR("Fail to save image: {}", filePath.toStdString());
            }
        }
        // // 若未启用数据库，则默认将图片手工特征存放在图片名中
        // if (best_image->save == true)
        // {
        //     QString fileName = QString::number(best_image->image_num)                    // 序号
        //                        + "_" + QString::number(best_image->total_image_count)    // 帧号
        //                        + "_" + QString::number(best_image->sharpness)            // 清晰度
        //                        + "_" + QString::number(best_image->similarity)           // 相似度
        //                        + "_" + QString::number(best_image->magnification)        // 倍率
        //                        + "_" + QString::number(best_image->effective_area)       // 有效区域面积
        //                        + "_" + QString::number(best_image->normalized_sharpness) // 归一化后的清晰度
        //                        + "_" + _img_inference_result +                           // AI诊断结果+置信度
        //                        +"_" + "NA"                                               // 医生诊断结果，默认为NA
        //                        + ".png";                                                 // 保存为PNG格式

        //     QString save_dir = _bgcamera->m_saveDir;
        //     QString filePath = save_dir + "/" + fileName;

        //     // 判断图片是否保存成功
        //     if (best_image->image.save(filePath))
        //     {
        //         emit _bgcamera->signal_show_image(best_image, imgInferResult, video_res);
        //     }
        //     else
        //     {
        //         LOGGER_ERROR("Fail to save image: {}", filePath.toStdString());
        //     }
        // }

        // 保存完图片后，退出AI处理线程。退出时将变量m_free_thread_num加一，表示当前被占用的AI处理线程被释放
        _bgcamera->m_numFreeThread++;
    }

#ifdef FTP_SEND
    if (!dbConnectRet)
    {
        dbSockClose(dbClientSock);
    }
#endif // FTP_SEND
}

void bgCamera::initThreadPool(const unsigned int deal_cnt)
{
    LOGGER_INFO("Initialize thread pool with {} threads for image processing.", deal_cnt);
    if (m_dealFlag == 1)
    {
        return;
    }

    m_dealFlag = 1;
    for (unsigned int i = 0; i < deal_cnt; i++)
    {
        m_aiThreadPool[i] = std::thread(processVideoStream, this);
    }
}

void bgCamera::releaseThreadPool()
{
    LOGGER_INFO("Releasing thread pool.");
    if (m_dealFlag == 0)
    {
        return;
    }

    m_dealFlag = 0;
    for (unsigned int i = 0; i < DEAL_THREAD_MAX_NUMS; i++)
    {
        m_aiThreadPool[i].join();
    }
}

void bgCamera::handleSingleImage(QImage img)
{
    m_imgNumToFPGA++;

    // 跳过FPGA，直接模拟读取结果进行调试 ----------
    {
        if (m_tempCounter == 8)
        {
            m_tempCounter = 0;

            HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();

            if (img_node == NULL)
            {
                qDebug() << "当前free链表没有空闲节点, 丢失图片: " << m_validImg->total_image_count;
                m_validImg->clear();
            }
            else
            {
                m_validImgNum++;
                signal_showPreviewImg(QPixmap::fromImage(m_validImg->image), m_validImgNum);

                m_validImg =
                    new BestImage(img.copy(), "2501111", m_validImgNum, 0, 0, 0, 0, 0, m_microMagnification, m_saveImg);

                img_node->best_image = BestImage(m_validImg);
                img_node->best_image.image_num = m_validImgNum;

                // 填充好内容后，将结点挂载到used_list链表上，由子线程处理
                m_img_pool->fill_deal_mem_pool(img_node);
                img_node = NULL;
                m_validImg->clear();
            }

            return;
        }
        else
        {
            m_tempCounter++;
            return;
        }
        // --------------------------------------
    }

    // 定义图片质量评价指标
    unsigned int read_sharp = 16;      // 清晰度
    unsigned int read_frame = 16;      // 帧号
    unsigned int read_similarity = 16; // 相似度
    unsigned int norm_clear = 16;      // 归一化后的清晰度
    unsigned int slide_class = 1;      // 切片类型，如H&E,CD138-MUM1等
    unsigned int read_eff_area = 16;   // 有效组织区域大小
    unsigned int read_med_area = 16;
    unsigned int read_high_area = 16;

    // 将图片传输至FPGA，读取清晰度，相似度，归一化后的清晰度，有效组织区域大小，帧号
    bool ret = getMetricsFromFPGA(img.bits(), read_sharp, read_frame, read_similarity, read_eff_area);

    if (!ret)
    {
        LOGGER_ERROR("Failed to read value from FPGA.");
        return;
    }

    read_med_area = (read_eff_area >> 10) & 0x000003ff;
    read_high_area = (read_eff_area >> 20) & 0x000003ff;
    read_eff_area = read_high_area;

    if (read_eff_area != 0)
    {
        norm_clear = (read_sharp * 1.0 / static_cast<double>(read_high_area)) * 450;
    }
    else
    {
        norm_clear = 0x00000000;
    }

    // 在控制台打印FPGA读取信息
    LOGGER_DEBUG("Read from FPGA: Frame = {}, Sharpness = {}, Similarity = {}, Effective Area = {}, "
                 "Normalized Clarity = {}",
                 read_frame, read_sharp, read_similarity, read_eff_area, norm_clear);

    m_lbl_debug->setText(QString::asprintf("帧号 = %u, 清晰度 = %u, 相似度 = %u, 有效面积 = %u, 归一化清晰度 = %u",
                                           read_frame, read_sharp, read_similarity, read_eff_area, norm_clear));

    getValidImage(img, read_frame, read_sharp, norm_clear, read_similarity, read_eff_area);
}

void bgCamera::slot_handleFileTest(QImage img)
{
    handleSingleImage(img);
}

void bgCamera::slot_cameraRectValueChanged(int _rectX, int _rectY)
{
    m_magDetWorker->m_rectX = _rectX;
    m_magDetWorker->m_rectY = _rectY;
}
