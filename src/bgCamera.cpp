#include "bgCamera.h" 
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QSlider>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMediaDevices>
#include <QMessageBox>
#include <QVideoFrame>
#include <QThread>
#include <QMenu>
#include <QLabel>
#include <QTimer>
#include <QCheckBox>
#include <QDir>
#include <time.h>
#include <codecvt>
#include <iostream>
#include "IniParser.h"
#include "deployment.h"
#include "hpec_lib.h"
#include "tmagnifydetect.h"
#include "CamImgPool.h"




std::mutex send_lock;
std::mutex mem_lock;

std::string version;        // 系统版本号

const std::map<std::string, std::vector<std::string>> bgCamera::classes_map = {
    {SLICESOURCE_STOMACH, CLSNAME_STOMACH},
    {SLICESOURCE_GUT, CLSNAME_GUT},
    {SLICESOURCE_PROSTATE, CLSNAME_PROSTATE},
    {SLICESOURCE_UNKNOWN, CLSNAME_KNOWN},
    {SLICESOURCE_DEFAULT, CLSNAME_DEFAULT}
};

bgCamera::bgCamera(QWidget* parent):
    QWidget(parent),
    m_hcam(nullptr),
    m_timer(new QTimer(this)),
    m_imgWidth(DEFUALT_WIDTH), m_imgHeight(DEFUALT_HEIGHT), m_pData(nullptr),
    m_temp(BGCAM_TEMP_DEF), m_tint(BGCAM_TINT_DEF),
    m_count(0),
    m_save_img(true),
    m_cur_bus_num(0),
    m_free_thread_num(DEAL_THREAD_MAX_NUMS),
    m_camera(nullptr),
    trans_cnt(0),
    m_gap_image(0), m_color_res(0),
    m_connect(false), m_trans(false),
    m_slide_info(new PIS_RES()),
    m_save_dir(""),
    m_best_image(new BestImage())
{
    QString iniPath = M_INIT_FILE_PATH;
    QString groupName = M_GROUP_NAME;

    /*读取系统设置*/
    version = IniParser::readIniSettings(iniPath, groupName, "Version").toStdString();

    m_save_root = IniParser::readIniSettings(iniPath, groupName, "FilePath");
    m_save_all_images = IniParser::readIniSettings(iniPath, groupName, "SaveAllImages").toInt();

    m_inference_video_result = IniParser::readIniSettings(iniPath, groupName, "InferenceVideoResult").toInt();

    m_sharp = IniParser::readIniSettings(iniPath, groupName, "Clarity").toInt();
    m_similarity = IniParser::readIniSettings(iniPath, groupName, "Similarity").toInt();
    m_area = IniParser::readIniSettings(iniPath, groupName, "Area").toInt();

    m_db_addr = IniParser::readIniSettings(iniPath, groupName, "DBAddr").toStdString();
    m_db_port = IniParser::readIniSettings(iniPath, groupName, "DBPort").toInt();

    m_FPGADNA = IniParser::readIniSettings(iniPath, groupName, "FPGADNA").toLower().remove("0x").toUInt(nullptr, 16);

    m_show_fpga_debug_info = IniParser::readIniSettings(iniPath, groupName, "ShowFPGADebugInfo").toInt();
    m_show_save_image_debug_info = IniParser::readIniSettings(iniPath, groupName, "ShowSaveImageDebugInfo").toInt();
    deployment::m_show_model_debug_info = IniParser::readIniSettings(iniPath, groupName, "ShowModelDebugInfo").toInt();


    initUI();
    m_img_pool = new CImgPool();
    initMagDetThread();
    enable_deal_thd(DEAL_THREAD_MAX_NUMS);

    m_cbox_mag->setChecked(true);
}

bgCamera::~bgCamera()
{
    // freeSaveImgThread();
    if(m_img_pool){
        delete m_img_pool;
        m_img_pool = nullptr;
    }
    freeMagDetThread();
    if (m_hcam)
    {
        Bgcam_Close(m_hcam);
        m_hcam = nullptr;
    }
    disable_deal_thd();

    if (m_slide_info) {
        delete m_slide_info;
        m_slide_info = nullptr;
    }

    if (m_timer) {
        delete m_timer;
        m_timer = nullptr;
    }
    //if (m_best_image) {
    //    delete m_best_image;
    //    m_best_image = nullptr;
    //}
}

std::string bgConverter::wchar_to_string_bg(const wchar_t* wstr) {
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(wstr);
    }
    catch (const std::exception& e) {
        std::cerr << "Conversion error: " << e.what() << std::endl;
        return "";
    }
}

void bgCamera::initUI()
{
    QHBoxLayout* vertical_main = new QHBoxLayout(this);
    //右半边网格布局
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
        connect(m_cbox_auto, &QCheckBox::stateChanged, this, [this](bool state)
        {
            if (m_hcam)
            {
                Bgcam_put_AutoExpoEnable(m_hcam, state ? 1 : 0);
                m_slider_expoTarget->setEnabled(state);
                m_slider_expoTime->setEnabled(!state);
                m_slider_expoGain->setEnabled(!state);
                //unsigned short get_target;
                //Bgcam_get_AutoExpoTarget(m_hcam, &get_target);
                //cout << "get_target: " << get_target << endl;
                /*unsigned short _target = 120;
                Bgcam_put_AutoExpoTarget(m_hcam, _target);*/
            }
        });

        connect(m_slider_expoTarget, &QSlider::valueChanged, this, [this](int value) {
            if (m_hcam) {
                m_lbl_expoTarget->setText(QString::number(value));
                if (m_cbox_auto->isChecked()) {
                    Bgcam_put_AutoExpoTarget(m_hcam, value);
                }
            }
         });
        connect(m_slider_expoTime, &QSlider::valueChanged, this, [this](int value)
        {
            if (m_hcam)
            {
                m_lbl_expoTime->setText(QString::number(value));
                if (!m_cbox_auto->isChecked())
                   Bgcam_put_ExpoTime(m_hcam, value*1000);
            }
        });
        connect(m_slider_expoGain, &QSlider::valueChanged, this, [this](int value)
        {
            if (m_hcam)
            {
                m_lbl_expoGain->setText(QString::number(value));
                if (!m_cbox_auto->isChecked())
                    Bgcam_put_ExpoAGain(m_hcam, value);
            }
        });

        QVBoxLayout* v = new QVBoxLayout(gboxexp);
        v->addWidget(m_cbox_auto);
        v->addLayout(makeLayout3(new QLabel("曝光目标:"), m_slider_expoTarget, m_lbl_expoTarget,
                                new QLabel("曝光时间(ms):"), m_slider_expoTime, m_lbl_expoTime, 
                                new QLabel("增益(%):"), m_slider_expoGain, m_lbl_expoGain));
        //gboxexp->setLayout(v);
    }//曝光box

    QGroupBox* gboxwb = new QGroupBox("白平衡");
    {
        m_btn_defaultWB = new QPushButton("默认值");
        m_btn_defaultWB->setEnabled(false);
        connect(m_btn_defaultWB, &QPushButton::clicked, this, [this]()
        {
            Bgcam_put_TempTint(m_hcam, BG_TEMP, BG_TINT);
            //设为默认值
            m_slider_temp->setValue(BG_TEMP);
            m_slider_tint->setValue(BG_TINT);
        });
        m_btn_autoWB = new QPushButton("白平衡");
        m_btn_autoWB->setEnabled(false);
        connect(m_btn_autoWB, &QPushButton::clicked, this, [this]()
        {
            //自动白平衡函数
            Bgcam_AwbOnce(m_hcam, nullptr, nullptr);
            //读取设置的白平衡参数
            int _get_int, _get_temp;
            Bgcam_get_TempTint(m_hcam, &_get_temp, &_get_int);
            //更改参数到滑动条
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
        connect(m_slider_temp, &QSlider::valueChanged, this, [this](int value)
        {
            m_temp = value;
            if (m_hcam)
                Bgcam_put_TempTint(m_hcam, m_temp, m_tint);
            m_lbl_temp->setText(QString::number(value));
        });
        connect(m_slider_tint, &QSlider::valueChanged, this, [this](int value)
        {
            m_tint = value;
            if (m_hcam)
                Bgcam_put_TempTint(m_hcam, m_temp, m_tint);   //设置白平衡的函数
            m_lbl_tint->setText(QString::number(value));
        });

        QVBoxLayout* v = new QVBoxLayout(gboxwb);
        QHBoxLayout* v2 = new QHBoxLayout(gboxwb);
        m_btn_defaultWB->setMaximumWidth(100);
        m_btn_autoWB->setMaximumWidth(100);
        v2->addWidget(m_btn_defaultWB);
        v2->addWidget(m_btn_autoWB);
        v2->addStretch();
        v->addLayout(makeLayout(new QLabel("色温:"), m_slider_temp, m_lbl_temp, new QLabel("色调:"), m_slider_tint, m_lbl_tint));
        v->addLayout(v2);
        //gboxwb->setLayout(v);
    }//白平衡box

    //按钮布局
    {
        m_btn_open = new QPushButton("打开");
        connect(m_btn_open, &QPushButton::clicked, this, &bgCamera::onBtnOpen);
        m_btn_connect = new QPushButton("连接设备");
        connect(m_btn_connect, &QPushButton::clicked, this, &bgCamera::onBtnConnect);
        m_btn_trans = new QPushButton("开始传输");
        connect(m_btn_trans, &QPushButton::clicked, this, &bgCamera::onBtnTrans);
        m_cbox_mag = new QCheckBox("倍率检测");
        connect(m_cbox_mag,&QCheckBox::checkStateChanged,this,&bgCamera::on_m_cbox_magStateChanged);
        m_cbox_save = new QCheckBox("存储入库");
        m_cbox_save->setChecked(true);
        connect(m_cbox_save,&QCheckBox::checkStateChanged,this,&bgCamera::on_m_cbox_saveStateChanged);
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
        m_lbl_frame = new QLabel();
        m_lbl_debug = new QLabel();
        //显示视频帧
        m_lbl_video = new QLabel("Video");
        m_lbl_video->setObjectName("lbl_video");
        m_lbl_video->setStyleSheet("#lbl_video{  border: 1px solid black; \
                                    border-radius: 2px;\
                                    font:bold 82px;\
                                    color:#bdbebd;}");
        m_lbl_video->setMinimumHeight(540);
        m_lbl_video->setMaximumHeight(660);
        m_lbl_video->setMinimumWidth(800);  //wll--height/width
        m_lbl_video->setMaximumWidth(1000);
        QVBoxLayout* v = new QVBoxLayout(this);
        v->addWidget(m_lbl_video, 2);
        QHBoxLayout* v2 = new QHBoxLayout(this);
        v2->addWidget(m_lbl_frame);
        v2->addWidget(m_lbl_debug);
        v2->addStretch();
        v->addLayout(v2);
        v->addStretch();
        vertical_main->addLayout(v, 5);
    }

    vertical_main->addLayout(gmain_right, 1);
    vertical_main->setSpacing(15);
    //最终布局
    setLayout(vertical_main);


    //回调函数
    connect(this, &bgCamera::evtCallback, this, [this](unsigned nEvent)
    {
        /* this run in the UI thread */
        if (m_hcam)
        {
            if (BGCAM_EVENT_IMAGE == nEvent)
            {
                handleImageEvent();
            }
            else if (BGCAM_EVENT_EXPOSURE == nEvent)
                handleExpoEvent();
            else if (BGCAM_EVENT_TEMPTINT == nEvent)
                handleTempTintEvent();
            else if (BGCAM_EVENT_STILLIMAGE == nEvent)
                handleStillImageEvent();
            else if (BGCAM_EVENT_ERROR == nEvent)
            {
                closeCamera();
                QMessageBox::warning(this, "Warning", "Generic error.");
            }
            else if (BGCAM_EVENT_DISCONNECTED == nEvent)
            {
                closeCamera();
                QMessageBox::warning(this, "Warning", "Camera disconnect.");
            }
        }
    });

    //计帧数
    connect(m_timer, &QTimer::timeout, this, [this]()
    {
        unsigned nFrame = 0, nTime = 0, nTotalFrame = 0;
        if (m_hcam && SUCCEEDED(Bgcam_get_FrameRate(m_hcam, &nFrame, &nTime, &nTotalFrame)) && (nTime > 0))
            m_frame = nFrame * 1000.0 / nTime;
            m_lbl_frame->setText(QString::asprintf("total = %u, fps = %.1f", nTotalFrame, m_frame));
    });
}


void bgCamera::initMagDetThread()
{
    qDebug() << "initMagDetThread";

        
    m_mySink = new QVideoSink(this);
    m_captureSession = new QMediaCaptureSession(this);

    m_magDetThread = new QThread;
    m_magDetImage = new TMagDetImage();
    m_magDetWorkwer = new TMagnifyDetect(m_magDetImage);
    m_magDetWorkwer->moveToThread(m_magDetThread);
    connect(m_magDetThread,&QThread::finished,m_magDetWorkwer,&TMagnifyDetect::deleteLater);
    connect(m_magDetThread,&QThread::started,m_magDetWorkwer,&TMagnifyDetect::working);

    m_magDetThread->start();

    connect(m_mySink,&QVideoSink::videoFrameChanged,
            this,&bgCamera::slot_on_magnify_frame_changed);

}

void bgCamera::freeMagDetThread()
{
    m_magDetWorkwer->stop();
    m_magDetWorkwer->close();
    m_magDetThread->quit();
    m_magDetThread->wait();

    delete m_magDetThread;
}

void bgCamera::set_cur_bus_num(int _num)
{
    m_cur_bus_num = _num;
}


void bgCamera::set_trans_state(bool _trans)
{
    m_trans = _trans;
}


void bgCamera::closeCamera()
{
    if (m_hcam)
    {
        Bgcam_Close(m_hcam);
        m_hcam = nullptr;
    }
    delete[] m_pData;
    m_pData = nullptr;

    m_btn_open->setText("打开");
    m_timer->stop();
    m_lbl_frame->clear();
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
    closeCamera();
}

void bgCamera::startCamera()
{
    if (m_pData)
    {
        delete[] m_pData;
        m_pData = nullptr;
    }
    m_pData = new uchar[TDIBWIDTHBYTES(m_imgWidth * 24) * m_imgHeight];
    unsigned uimax = 0, uimin = 0, uidef = 0;
    unsigned short usmax = 0, usmin = 0, usdef = 0;
    Bgcam_get_ExpTimeRange(m_hcam, &uimin, &uimax, &uidef);                     //获取相机所能使用的最大、最小以及默认的曝光时间
    //qDebug()<<"umin: " << uimin <<";umax: " << uimax;
    m_slider_expoTarget->setRange(BGCAM_AETARGET_MIN, BGCAM_AETARGET_MAX);  
    m_slider_expoTime->setRange(uimin/1000, uimax/1000);                        //这里获取的单位是ns，而显示的是ms
    Bgcam_get_ExpoAGainRange(m_hcam, &usmin, &usmax, &usdef);
    m_slider_expoGain->setRange(usmin, usmax);
    if (0 == (m_cur.model->flag & BGCAM_FLAG_MONO))
        handleTempTintEvent();
    handleExpoEvent();

    if (SUCCEEDED(Bgcam_StartPullModeWithCallback(m_hcam, eventCallBack, this)))
    {
        m_cbox_auto->setEnabled(true);
        m_btn_autoWB->setEnabled(0 == (m_cur.model->flag & BGCAM_FLAG_MONO));
        m_btn_defaultWB->setEnabled(0 == (m_cur.model->flag & BGCAM_FLAG_MONO));
        m_slider_temp->setEnabled(0 == (m_cur.model->flag & BGCAM_FLAG_MONO));
        m_slider_tint->setEnabled(0 == (m_cur.model->flag & BGCAM_FLAG_MONO));
        m_btn_open->setText("关闭");

        int bAuto = 0;
        Bgcam_get_AutoExpoEnable(m_hcam, &bAuto);
        m_cbox_auto->setChecked(1 == bAuto);
        
        m_timer->start(1000);
    }
    else
    {
        closeCamera();
        QMessageBox::warning(this, tr("Warning"), tr("Failed to start camera."));
    }
}

void bgCamera::openCamera()
{
    m_hcam = Bgcam_Open(m_cur.id);      //通过设备id号，在打开显微镜后获得显微镜句柄
    if (m_hcam)
    {
        //设置默认分辨率
        Bgcam_put_Size(m_hcam, DEFUALT_WIDTH, DEFUALT_HEIGHT);   //第2+1种尺寸
        Bgcam_put_Option(m_hcam, BGCAM_OPTION_BYTEORDER, 0); //Qimage use RGB byte order
        Bgcam_put_AutoExpoEnable(m_hcam, 1);

        //设置锐化：0-500
        int threshold = 0;
        int radius = 2;
        int strength = 350;
        int iValue = (threshold << 24) | (radius << 16) | (strength);
        Bgcam_put_Option(m_hcam, BGCAM_OPTION_SHARPENING, iValue);
        startCamera();
    }
}

void bgCamera::onBtnOpen()
{
    if (m_hcam){
        closeCamera();
        //断开传输
        if(m_trans == true){
            m_trans = false;
            m_btn_trans->setText("开始传输");
            //m_saveImgWorkwer->stop();
        }
    }
    else
    {
        BgcamDeviceV2 arr[BGCAM_MAX] = { {0} };
        unsigned count = Bgcam_EnumV2(arr);
        if (0 == count){
            QMessageBox::warning(this, tr("警告"), tr("未找到显微镜设备，请检查驱动!"));
        }else if (1 == count){
            m_cur = arr[0];
            openCamera();
        }else{
            QMenu menu;
            for (unsigned i = 0; i < count; ++i)
            {
                menu.addAction(
#if defined(_WIN32)
                            QString::fromWCharArray(arr[i].displayname)
#else
                            arr[i].displayname
#endif
                            , this, [this, i, arr](bool)
                {
                    m_cur = arr[i];
                    openCamera();
                });
            }
            // menu.exec(mapToGlobal(m_btn_snap->pos()));
        }
    }
}


void bgCamera::onBtnConnect()
{
    if(m_connect == false){
        // open device
        //自动识别总线id
        m_hDevice = OpenDevice(m_cur_bus_num); // 连接设备
        if(m_hDevice < 0)
        {
            qDebug() << "驱动异常，请检查通路！";
            return;
        }
        else
        {
            //连接成功
            qDebug()<<"m_hDevice: " << m_hDevice;
            m_connect = true;
            m_btn_connect->setText("断开设备");
            InitDevice(m_hDevice);//初始化设备
            qDebug() << "设备连接成功!";
            // 将FPGA清零，防止硬件出现传输错误
            unsigned int reg_base = 0x6000;     // 寄存器基址
            unsigned int reg_offset3 = 21 * 4;  // 软件清零
            unsigned int reg_offset7 = 23 * 4;  // 软件验证码

            int ret = 0;
            ret = write_19eg_reg(m_hDevice, reg_base + reg_offset3, 0x00000001);
            ret = write_19eg_reg(m_hDevice, reg_base + reg_offset3, 0x00000000);

            if (m_show_fpga_debug_info == 1) {
                if (ret == W_REG_OK) {
                    qDebug() << "Succeed to clear FPGA!";
                }
                else {
                    qDebug() << "Failed to clear FPGA!";
                }
            }

            ret = write_19eg_reg(m_hDevice, reg_base + reg_offset7, m_FPGADNA);     // FPGA DNA, e.g. 40070524 | 90042762

            if (m_show_fpga_debug_info == 1) {
                if (ret == W_REG_OK) {
                    qDebug() << "Succeed to write FPGA DNA!";
                }
                else {
                    qDebug() << "Failed to write FPGA DNA!";
                }
            }
        }
    }else{
        CloseDevice(m_hDevice);//断开设备
        qDebug() << "设备已断开！";
        m_hDevice = -1;
        m_connect = false;
        m_trans = false;
        m_btn_trans->setText("开始传输");
        //m_saveImgWorkwer->stop();
        m_btn_connect->setText("连接设备");
    }
}

int bgCamera::onBtnTrans()
{
    if(m_trans == false){
        // 开始传输
        if(m_hcam && m_hDevice != -1){
            if(m_save_dir == ""){
                emit directly_click_btn_trans();
            }
            m_trans = true;
            m_btn_trans->setText("停止传输");

            qDebug() << "正在传输数据...";
        }else{
            QMessageBox::warning(this, "错误", "请检查是否连接设备.");
            return -1;
        }
    }else{
        m_trans = false;
        m_btn_trans->setText("开始传输");
        qDebug() << "已停止传输！";
    }
    return 0;
}

void bgCamera::on_m_cbox_saveStateChanged(const Qt::CheckState &arg1)
{
    if(arg1 == Qt::CheckState::Checked){
        m_save_img = true;
    }else{
        m_save_img = false;
    }
}

void bgCamera::on_m_cbox_magStateChanged(const Qt::CheckState &arg1)
{
    if(arg1 == Qt::CheckState::Checked){
        //检测设备
        const QList<QCameraDevice> videoDevices = QMediaDevices::videoInputs();
        for (const QCameraDevice &device : videoDevices)
        {
            if(device.description() == MAGNIFICATION_CAMERA_ID){
                if(m_camera == nullptr){
                    m_camera = new QCamera(device);
                    qDebug() << "bind camera: " << device.description();
                }else{
                    qDebug() << "had bind camera: " << device.description();
                }
            }
        }
        if(m_camera){
            m_captureSession->setCamera(m_camera);
            m_captureSession->setVideoSink(m_mySink);
            m_camera->start();
            //working start
            m_magDetWorkwer->start();
        }else{
            qDebug() << "not find camera.";
        }
    }else{
        //destroy thread
        delete m_camera;
        m_camera = nullptr;
        m_magDetWorkwer->stop();
    }
}

void bgCamera::eventCallBack(unsigned nEvent, void* pCallbackCtx)
{
    bgCamera* pThis = reinterpret_cast<bgCamera*>(pCallbackCtx);
    emit pThis->evtCallback(nEvent);        //发出事件回调的Signal
}


int get_blur_from_fpga(uchar* _pData, int _device, int _cnt_num, int _mag, int _slide_class,
    unsigned int& rsharp, unsigned int& rframe, unsigned int& rsimilar, unsigned int& rarea) {
    //pcie最小数据传输单位是*8M,适应图像尺寸以及包头512位-64字节
    int _img_size = TDIBWIDTHBYTES(DEFUALT_WIDTH * 24 * DEFUALT_HEIGHT);
    const int PCIE_SIZE = 64 + _img_size;
    char* down_buf = static_cast<char*> (malloc(PCIE_SIZE));
    if (down_buf) {
        memset(down_buf, 0, PCIE_SIZE);
    }
    else {
        qDebug() << "malloc down_buf error!";
        exit(1);
    }
    //定义包头
    unsigned int frame_flag = 0x12345678;
    unsigned int frame_length = _img_size / 3;
    unsigned int frame_num = _cnt_num;
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
    //倍率包头
    unsigned int mag = _mag;
    down_buf[16] = mag & 0xff;
    down_buf[17] = (mag >> 8) & 0xff;
    down_buf[18] = (mag >> 16) & 0xff;
    down_buf[19] = (mag >> 24) & 0xff;
    //切片种类包头
    unsigned int slide_class = _slide_class;
    down_buf[20] = slide_class & 0xff;
    down_buf[21] = (slide_class >> 8) & 0xff;
    down_buf[22] = (slide_class >> 16) & 0xff;
    down_buf[23] = (slide_class >> 24) & 0xff;

    //unsigned char tail[] = {0x00,0x1a,0x1a};
    //后512位都是像素信息
    memcpy(down_buf + 64, _pData, _img_size);
    //memcpy(down_buf + 64+ _img_size,tail,3);
    ULONG n_send = 0;                   //返回的字节数，发送了几个
    clock_t last_clock = clock();
    unsigned long long total_size = 0;  //一帧传送的字节数
    unsigned int reg_offset = 11 * 4;       //寄存器地址偏移量，清晰度
    unsigned int reg_offset1 = 13 * 4;//相似度
    unsigned int reg_offset2 = 14 * 4;//帧号
    unsigned int reg_offset3 = 21 * 4;//软件清零
    unsigned int reg_offset4 = 12 * 4;//有效面积
    unsigned int reg_offset5 = 15 * 4;//硬件接收图片报错
    unsigned int reg_offset6 = 22 * 4;//红绿色差阈值，和有效面积成反比，默认是40，区间1~255；若要使用这个寄存器，注意首位得是1。//=0x8000_0000 | 红绿色差阈值；
    unsigned int reg_offset7 = 23 * 4;//软件端验证码，90042762
    unsigned int reg_base = 0x6000;     //寄存器基址
    int ret = 0;
    // ret = write_19eg_reg(_device,reg_base + reg_offset3, 0x00000001);
    //ret = write_19eg_reg(_device,reg_base + reg_offset3, 0x00000000);
    //这里的时延根据单次传输的数据量调节，经调试，小于30读出来的read_reg_1可能为0，来不及更新
    //int ret

    ret = WriteDataToFpga(_device, down_buf, PCIE_SIZE, FPGA_CHANNEL, 0, n_send, 100);
    
    if (n_send != 6654016) {
        //qDebug() << "----------------n_send" << n_send;
        //将FPGA清零，防止硬件出现传输错误
        unsigned int reg_base = 0x6000;     // 寄存器基址
        unsigned int reg_offset3 = 21 * 4;  // 软件清零
        int ret1 = 0;
        ret1 = write_19eg_reg(_device, reg_base + reg_offset3, 0x00000001);
        ret1 = write_19eg_reg(_device, reg_base + reg_offset3, 0x00000000);


    }

    if (ret == -1) {
        qDebug() << "WriteDataToFpga: 等待超时.";
    }

    clock_t cur_clock = clock();
    double time = (cur_clock - last_clock) * 1.0 / 1000;
    double speed = n_send * 1.0 / 1024 / 1024 / time;
    // qDebug() << FPGA_CHANNEL << "号下行通道传输时间 " << time
    //         << " s, 数据量 " << n_send
    //         << " B, 速度 " << speed << " MB/s";

    free(down_buf);

    Sleep(5);

    //读返回结果
    if (read_19eg_reg(_device, reg_base + reg_offset, rsharp) != R_REG_OK) {
        //qDebug() << "read_reg1: " << read_reg1;
        return -1;
    }
    if (read_19eg_reg(_device, reg_base + reg_offset1, rsimilar) != R_REG_OK) {
        //qDebug() << "read_reg1: " << read_reg1;
        return -1;
    }
    if (read_19eg_reg(_device, reg_base + reg_offset2, rframe) != R_REG_OK) {

        //qDebug() << "read_reg1: " << read_reg1;
        return -1;
    }
    if (read_19eg_reg(_device, reg_base + reg_offset4, rarea) != R_REG_OK) {

        //qDebug() << "read_reg1: " << read_reg1;
        return -1;
    }
    return 0;
}


// 处理从相机过来的图片
void bgCamera::handleImageEvent()
{
    unsigned width = 0, height = 0;
    if (SUCCEEDED(Bgcam_PullImage(m_hcam, m_pData, 24, &width, &height)))       // 拉取图片，RGB格式
    {
        int _size = TDIBWIDTHBYTES(width * 24) * height;  // 59535360=5440*3648*3

        // 引用m_pData数据，不进行拷贝
        QImage image = QImage(m_pData, width, height, QImage::Format_RGB888);

        // 将显微镜传来的图片缩放后显示（创建新内存，拷贝并缩放）
        QImage newimage = image.copy().scaled(m_lbl_video->width(), m_lbl_video->height(),
                                       Qt::KeepAspectRatio, Qt::FastTransformation);
        m_lbl_video->setPixmap(QPixmap::fromImage(newimage));       


        // 将视频流数据传输至FPGA
        if (m_trans) {
            trans_cnt++;

            // 倍率检测失败则直接返回，无处理的必要
            if (m_color_res == 0) {
                return;
            }

            // 预定义图片质量评价指标
            unsigned int read_sharp = 16;               // 清晰度
            unsigned int read_frame = 16;               // 帧号
            unsigned int read_similarity = 16;          // 相似度
            unsigned int norm_clear = 16;               // 归一化后的清晰度
            unsigned int slide_class = 1;               // 切片类型，如H&E,CD138-MUM1等
            unsigned int read_eff_area = 16;            // 有效组织区域大小
            unsigned int read_med_area = 16;
            unsigned int read_high_area = 16;
                
            // 发送给FPGA，读取清晰度，相似度，归一化后的清晰度，有效组织区域大小，帧号
            int ret = get_blur_from_fpga(m_pData, m_hDevice, trans_cnt, m_color_res, slide_class, read_sharp, read_frame, read_similarity, read_eff_area);
            if (ret == -1) {
                qDebug() << "Failed to read value from FPGA.";
                return;
            }

            read_med_area = (read_eff_area >> 10) & 0x000003ff;
            read_high_area = (read_eff_area >> 20) & 0x000003ff;
            read_eff_area = read_high_area;

            if (read_eff_area != 0) {
                norm_clear = (read_sharp * 1.0 / static_cast<double>(read_high_area)) * 450;
                    
            }
            else {
                norm_clear = 0x00000000;
            }

            // 是否在控制台打印FPGA读取信息
            if (m_show_fpga_debug_info == 1) {
                qDebug() << "---read_frame" << read_frame << "read_sharp: " << read_sharp << "read_similarity: " << read_similarity << "last_similarity" << m_last_similarity << "efficient_area:" << read_eff_area << "norm_clear" << norm_clear;
            }
            m_lbl_debug->setText(QString::asprintf("Frame_num = %u, Similarity = %u, Sharpness = %u, Eff_area = %u, Norm_clear = %u", read_frame, read_similarity, read_sharp, read_eff_area, norm_clear));
                

            // 判断是否保存所有图片
            if (m_save_all_images == 1) {
                if (m_save_img == true) {
                    QString fileName =
                        QString::number(read_frame)
                        + "_" + QString::number(read_sharp)
                        + "_" + QString::number(read_similarity)
                        + "_" + QString::number(trans_cnt)
                        + "_" + QString::number(read_eff_area)
                        + "_" + QString::number(norm_clear)
                        + ".png"; // 保存为PNG格式

                    QString cur_folder = m_save_dir;
                    QDir().mkpath(cur_folder + "/all");
                    QString filePath = cur_folder + "/all/" + fileName;

                    // 保存图片
                    if (image.save(filePath)) {
                        if (m_show_save_image_debug_info == 1) {
                            qDebug() << "success to save image:" << filePath;
                        }
                    }
                    else {
                        if (m_show_save_image_debug_info == 1) {
                            qDebug() << "Failed to save image:" << filePath;
                        }
                    }
                }
            }


            // 预定义筛选条件
            int _max_num = 2;                                       // 当显微镜头静止不动时能输出的图片的最大数量
            unsigned int _similarity = m_similarity;                // 相似度阈值
            unsigned int _sharp = m_sharp;                          // 清晰度阈值
            unsigned int _area = m_area;                            // 愉快面积阈值

            // 对图片进行条件筛选，若符合筛选条件且有空闲的节点时，将图片挂载到deal链表上进行AI处理；否则丢图，直接返回
            if ((m_last_similarity > _similarity && read_similarity <= _similarity) || (m_last_similarity <= _similarity && read_similarity <= _similarity)) {
                m_last_similarity = read_similarity;

                if (m_same_best_img_num < _max_num) {
                    if (m_similar_img_num < 5) {
                        m_similar_img_num++;
                        // 筛选最优图片
                        if (norm_clear > _sharp && read_eff_area > _area && norm_clear > m_best_image->sharpness) {
                            // 预览图片
                            emit signal_show_preview_img(QPixmap::fromImage(image.copy()), m_best_img_num + 1);

                            QString time_stamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");

                            // 确保释放之前记录图片所用的内存
                            if (m_best_image) {
                                delete m_best_image;
                                m_best_image = nullptr;
                            }
                            m_best_image = new BestImage(image.copy(), time_stamp, m_best_img_num, read_frame, read_sharp, read_similarity, read_eff_area, norm_clear, m_color_res, m_save_img);
                        }
                    }
                    if (m_similar_img_num == 5) {
                        m_similar_img_num = 0;
                        if (m_best_image->sharpness != 0) {
                            HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();
                            if (img_node == NULL) {
                                qDebug() << "当前free链表没有空闲节点，丢失图片：" << m_best_image->total_image_count;

                                m_best_image->clear();
                                return;
                            }
                            m_best_img_num++;
                            m_same_best_img_num++;
                            signal_show_preview_img(QPixmap::fromImage(m_best_image->image.copy()), m_best_img_num);

                            img_node->best_image = BestImage(m_best_image);
                            
                            img_node->best_image.image_num = m_best_img_num;
                            
                            // 填充好内容后，将结点挂载到used_list链表上，由deal_thd函数的子线程处理
                            m_img_pool->fill_deal_mem_pool(img_node);
                            img_node = NULL;
                            m_best_image->clear();
                        }
                    }
                }
                else {
                    m_best_image->clear();
                }
            }
            else if (m_last_similarity <= _similarity && read_similarity > _similarity) {
                m_last_similarity = read_similarity;
                do {
                    if (m_best_image->sharpness != 0) {
                        m_similar_img_num = 0;

                        if (m_same_best_img_num < _max_num) {
                            m_same_best_img_num = 0;
                            HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();

                            if (img_node == NULL) {
                                qDebug() << "当前free链表没有空闲节点，丢失图片：" << m_best_image->total_image_count;
                                m_best_image->clear();
                                break;
                            }

                            m_best_img_num++;
                            signal_show_preview_img(QPixmap::fromImage(m_best_image->image.copy()), m_best_img_num);

                            img_node->best_image = BestImage(m_best_image);
                            img_node->best_image.image_num = m_best_img_num;

                            //填充好内容后，将结点挂载到used_list链表上，由deal_thd函数的子线程处理
                            m_img_pool->fill_deal_mem_pool(img_node);
                            img_node = NULL;
                            m_best_image->clear();
                        }
                        else {
                            m_best_image->clear();
                        }
                    }
                    else {
                        m_similar_img_num = 0;
                        m_same_best_img_num = 0;
                    }
                } while (false);

                if (norm_clear > _sharp && read_eff_area > _area) {
                    HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();
                    if (img_node == NULL) {
                        qDebug() << "当前free链表没有空闲节点，丢失图片：" << m_best_image->image_num;
                        m_best_image->clear();
                        return;
                    }
                    m_best_img_num++;
                    signal_show_preview_img(QPixmap::fromImage(image.copy()), m_best_img_num);

                    // 确保释放之前记录图片所用的内存
                    if (m_best_image) {
                        delete m_best_image;
                        m_best_image = nullptr;
                    }
                    QString time_stamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
                    m_best_image = new BestImage(image.copy(), time_stamp, m_best_img_num, read_frame, read_sharp, read_similarity, read_eff_area, norm_clear, m_color_res, m_save_img);

                    img_node->best_image = BestImage(m_best_image);
                    
                    //填充好内容后，将结点挂载到used_list链表上，由deal_thd函数的子线程处理
                    m_img_pool->fill_deal_mem_pool(img_node);
                    img_node = NULL;
                    m_best_image->clear();
                }
            }
            else if (m_last_similarity > _similarity && read_similarity > _similarity) {
                m_last_similarity = read_similarity;
                if (norm_clear > _sharp && read_eff_area > _area) {
                    HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();
                    if (img_node == NULL) {
                        qDebug() << "当前free链表没有空闲节点，丢失图片：" << m_best_image->image_num;
                        m_best_image->clear();

                        return;
                    }
                    m_best_img_num++;
                    signal_show_preview_img(QPixmap::fromImage(image.copy()), m_best_img_num);

                    // 确保释放之前记录图片所用的内存
                    if (m_best_image) {
                        delete m_best_image;
                        m_best_image = nullptr;
                    }
                    QString time_stamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
                    m_best_image = new BestImage(image.copy(), time_stamp, m_best_img_num, read_frame, read_sharp, read_similarity, read_eff_area, norm_clear, m_color_res, m_save_img);

                    img_node->best_image = BestImage(m_best_image);

                    //填充好内容后，将结点挂载到used_list链表上，由deal_thd函数的子线程处理
                    m_img_pool->fill_deal_mem_pool(img_node);
                    img_node = NULL;
                    m_best_image->clear();
                }
            }

        }
        else {
            m_last_similarity = 1024;
        }
    }  //pull img success
}

void bgCamera::handleExpoEvent()
{
    unsigned time = 0;
    unsigned short gain = 0;
    unsigned short target = 0;
    Bgcam_get_AutoExpoTarget(m_hcam, &target);
    Bgcam_get_ExpoTime(m_hcam, &time);
    Bgcam_get_ExpoAGain(m_hcam, &gain);
    {
        const QSignalBlocker blocker(m_slider_expoTarget);
        m_slider_expoTarget->setValue(int(target));
    }
    {
        const QSignalBlocker blocker(m_slider_expoTime);
        m_slider_expoTime->setValue(int(time)/1000);
    }
    {
        const QSignalBlocker blocker(m_slider_expoGain);
        m_slider_expoGain->setValue(int(gain));
    }
    m_lbl_expoTarget->setText(QString::number(target));
    m_lbl_expoTime->setText(QString::number(time/1000));
    m_lbl_expoGain->setText(QString::number(gain));
}

void bgCamera::handleTempTintEvent()
{
    int nTemp = 0, nTint = 0;
    if (SUCCEEDED(Bgcam_get_TempTint(m_hcam, &nTemp, &nTint)))
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

void bgCamera::handleStillImageEvent()
{
    unsigned width = 0, height = 0;
    if (SUCCEEDED(Bgcam_PullStillImage(m_hcam, nullptr, 24, &width, &height))) // peek
    {
        std::vector<uchar> vec(TDIBWIDTHBYTES(width * 24) * height);
        if (SUCCEEDED(Bgcam_PullStillImage(m_hcam, &vec[0], 24, &width, &height)))
        {
            QImage image(&vec[0], width, height, QImage::Format_RGB888);
            image.save(QString::asprintf("demoqt_%u.jpg", ++m_count));
        }
    }
}

QVBoxLayout* bgCamera::makeLayout(QLabel* lbl1, QSlider* sli1, QLabel* val1,QLabel* lbl2, QSlider* sli2, QLabel* val2)
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

QVBoxLayout* bgCamera::makeLayout3(QLabel* lbl1, QSlider* sli1, QLabel* val1, 
                                  QLabel* lbl2, QSlider* sli2, QLabel* val2,
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

void bgCamera::slot_on_magnify_frame_changed(const QVideoFrame &frame)
{
    m_gap_image ++;

    if(m_gap_image == 3){
        m_gap_image = 0;
        if(frame.isValid()){
            unique_lock<mutex> lk(m_magDetImage->m_lock,std::defer_lock);
            if(lk.try_lock()){
                m_magDetImage->m_image = frame.toImage();
                m_color_res = m_magDetImage->color;
            }
        }
    }
}

cv::Mat bgCamera::qimageToMatRGB(const QImage& qimage)
{
    QImage rgbImage;

    // 检查输入图像格式，将其转换为RGB888格式
    if (qimage.format() != QImage::Format_RGB888) {
        rgbImage = qimage.convertToFormat(QImage::Format_RGB888);
    }
    else {
        rgbImage = qimage;
    }

    cv::Mat mat(rgbImage.height(), rgbImage.width(), CV_8UC3);
    std::memcpy(mat.data, rgbImage.constBits(), rgbImage.sizeInBytes());

    return mat;
}



AIResult image_inference(deployment* deployer, bgCamera* bgcamera, cv::Mat& _image, const BestImage* best_image) {

    /*裁剪patch*/
    std::pair<std::vector<cv::Mat>, std::vector<cv::Point>> pairs = deployer->extract_patches(_image); 


    if (pairs.second.size() == 0) {
        if (deployer->m_show_model_debug_info == 1) {
            std::cout << "This img have no valid patch!" << std::endl;
        }

        return AIResult(QString::fromStdString("invalid"), -1, QVector<float> {-1, -1, -1, -1, -1, -1}, cv::Point(0, 0));
    }

    /*提取特征向量*/
    std::vector<float> features = deployer->embedding(pairs.first);
    
    /*根据切片部位选择对应的AI模型*/
    std::pair<std::vector<float>, int> output = deployer->image_predict(features, bgcamera->m_slide_info->slicesource);

    mem_lock.lock();
    bgcamera->m_video_features.insert(bgcamera->m_video_features.end(), features.begin(), features.end()); // 按图片特征推理视频
    // 若是肠息肉切片且为10，20或40倍率，则存储MMR特征
    if (bgcamera->m_slide_info->slicesource == SLICESOURCE_GUT) {
        if (best_image->magnification == 10 || best_image->magnification == 20 || best_image->magnification == 40) {
            bgcamera->m_mmr_features.insert(bgcamera->m_mmr_features.end(), features.begin(), features.end());
        }
    }
    mem_lock.unlock();

    /*将关键patch的坐标保存为yaml*/
    cv::Point key_patch_coord = pairs.second[output.second];        
    std::string key_str = std::to_string(best_image->image_num);
    cv::FileStorage fs(bgcamera->m_save_dir.toStdString() + "/" + key_str + ".yaml", cv::FileStorage::WRITE);
    fs << "coords" << key_patch_coord;
    fs.release();


    /*搜索最大分类概率和最大概率对应的下表*/
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
    auto it = bgCamera::classes_map.find(bgcamera->m_slide_info->slicesource);
    if (it != bgCamera::classes_map.end()) {
        cls_name = it->second;
    }
    else {
        cls_name = bgCamera::classes_map.find(SLICESOURCE_DEFAULT)->second;
    }

    std::string result = cls_name[max_index] + "," + oss.str();     // seemingly useless

    //return { result, cls_prob };
    return AIResult(QString::fromStdString(cls_name[max_index]), cls_prob[max_index], QVector<float> (cls_prob.begin(), cls_prob.end()), key_patch_coord);
}




std::pair<string, std::vector<float>> video_inference(deployment* deployer, std::vector<float>& video_features, const std::string& slice_part) {

    std::vector<float> cls_prob = deployer->image_predict(video_features, slice_part).first;

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
    auto it = bgCamera::classes_map.find(slice_part);
    if (it != bgCamera::classes_map.end()) {
        cls_name = it->second;
    }
    else {
        cls_name = bgCamera::classes_map.find(SLICESOURCE_DEFAULT)->second;
    }

    std::string result = cls_name[max_index] + ", " + oss.str();

    if (deployer->m_show_model_debug_info == 1) {
        std::cout << "Final diagnosis result: " << result << std::endl;
    }

    return { result, cls_prob };
}



void deal_thd(bgCamera* _bgcamera) {
    CImgPool* mem_pool = _bgcamera->m_img_pool;
    HL_IMG_POOL_NODE* mem_node = NULL;

    deployment* deployer = new deployment();

#ifdef FTP_SEND
    SOCKET db_client_sock;
    int db_connect_ret = dbSockInit(db_client_sock, _bgcamera->m_db_addr, _bgcamera->m_db_port);

    // 连接数据库失败则弹出警告
    if (db_connect_ret) {
        if (!_bgcamera->warning_shown.test_and_set()) {
            emit _bgcamera->signal_show_db_connect_warning();
        }
    }
#endif // FTP_SEND

    while (_bgcamera->deal_flag == 1)
    {
        // 等待图片信号，没有信号的话程序就空跑
        mem_node = mem_pool->malloc_used_mem_pool();   
        if (mem_node == NULL)
        {
            continue;
        }

        // 进入AI处理线程，m_free_thread_num减一，表示空闲线程减少一个
        mem_lock.lock();
        _bgcamera->m_free_thread_num--;
        mem_lock.unlock();

        const BestImage* best_image = new BestImage(mem_node->best_image);

        // 读取完数据后，及时将used_list的结点挂载到free_list中，方便其他线程使用
        mem_pool->free_back_mem_pool(mem_node);     
        mem_node = NULL;

        cv::Mat model_input = bgCamera::qimageToMatRGB(best_image->image);       //返回RGB类型的Mat对象

        /*图片诊断*/
        AIResult tmp = image_inference(deployer, _bgcamera, model_input, best_image);
        AIResult* image_inference_res = new AIResult(tmp.diagnosis_res, tmp.diagnosis_prob, tmp.cls_prob, tmp.ROI);


        // 若无有效小图（即切的patch的大小为0）则退出
        if (image_inference_res->diagnosis_res == "invalid") {

            qDebug() << "无效图，图片序号：" << best_image->image_num;

            // 无效图时，将预览图的Loading信息改为invalid
            emit _bgcamera->signal_show_invalid_img(best_image->image_num);

            // 退出AI处理线程，m_free_thread_num加一，表示空闲线程增加一个
            mem_lock.lock();
            _bgcamera->m_free_thread_num++;
            mem_lock.unlock();

            continue;
        }

        QVector<float> cls_prob = image_inference_res->cls_prob;
        QStringList temp_list = QString::number(std::round(image_inference_res->diagnosis_prob * 1000.f) / 1000.f).split(".");
        QString _img_inference_result = image_inference_res->diagnosis_res + "_" + temp_list.join("");


        /*视频诊断*/
        std::pair<std::string, std::vector<float>> video_diagnosis_result = video_inference(deployer, _bgcamera->m_video_features, _bgcamera->m_slide_info->slicesource);

        send_lock.lock();
        _bgcamera->m_video_res = video_diagnosis_result;
        send_lock.unlock();

        QString video_res = QString::fromStdString(video_diagnosis_result.first);

        // 判断是否要进行MMR基因预测
        if (_bgcamera->m_slide_info->slicesource == SLICESOURCE_GUT && video_res.split(',')[0] == "C") {
            std::array<float, 4> mmr_result = deployer->mmr_predict(_bgcamera->m_mmr_features);
            QString MLH1_res;
            QString MSH2_res;
            QString MSH6_res;
            QString PMS2_res;
            float mmr_thres = 0.5;
            if (mmr_result[0] > mmr_thres) {
                MLH1_res = "+";
            }
            else {
                MLH1_res = "-";
            }
            if (mmr_result[1] > mmr_thres) {
                MSH2_res = "+";
            }
            else {
                MSH2_res = "-";
            }
            if (mmr_result[2] > mmr_thres) {
                MSH6_res = "+";
            }
            else {
                MSH6_res = "-";
            }
            if (mmr_result[3] > mmr_thres) {
                PMS2_res = "+";
            }
            else {
                PMS2_res = "-";
            }
            video_res = video_res + "(MLH1: " + MLH1_res + "  MSH2: " + MSH2_res
                + "  MSH6: " + MSH6_res + "  PMS2: " + PMS2_res + ")";
        }

        /*数据传输与落盘*/
#ifdef FTP_SEND
        // 判断socket是否初始化成功
        if (!db_connect_ret) {

            std::vector<double> video_res_prob(video_diagnosis_result.second.begin(), video_diagnosis_result.second.end());
            std::string _video_res = video_res.toStdString();

            PATHO_RES video_res = {
                _bgcamera->m_slide_info->pathological_id,                       // 病理号
                _bgcamera->m_slide_info->slice_id,                              // 切片号
                bgConverter::wchar_to_string_bg(_bgcamera->m_cur.id),		    // 传感器编号
                version,                                                        // 版本号
                static_cast<long int>(time(nullptr)),                           // 诊断时间
                _bgcamera->m_slide_info->pathological_order,	                // 诊断次数
                best_image->image_num,	                                        // 一个切片号对应图片
                9,	                                                            // 图片特征参数个数
                _bgcamera->m_slide_info->pathological_order,                    // 诊断次序
                _video_res,                                                     // AI诊断结果
                video_res_prob,                                                 // AI诊断概率
                "",                                                             // 医生诊断结果,这里必须为空
                "A02",                                                          // 包的类型
            };
            dbResultUpload(&video_res, db_client_sock);
        }

        // 保存图片
        if (best_image->save == true) {
            QString fileName = QString::number(best_image->image_num) + ".png";
            QString save_dir = _bgcamera->m_save_dir;
            QString filePath = save_dir + "/" + fileName;

            // 判断图片是否保存成功
            if (best_image->image.save(filePath)) {
                emit _bgcamera->signal_show_image(best_image, image_inference_res, video_res);
                if (_bgcamera->m_show_save_image_debug_info == 1) {
                    qDebug() << "Success to save image:" << filePath;
                }

                std::array<float, 6> img_ai_prob;
                std::copy(tmp.cls_prob.begin(), tmp.cls_prob.end(), img_ai_prob.begin());
                std::string img_ai_res = (image_inference_res->diagnosis_res + "," + QString::number(std::round(image_inference_res->diagnosis_prob * 1000.f) / 1000.f)).toStdString();

                // 将当前图片的手工特征等信息存入json对象中。键为图片名
                send_lock.lock();
                _bgcamera->m_images_features[fileName.toStdString()] = {
                    {"Frame",best_image->image_num},
                    {"Magnification",best_image->magnification},
                    {"Clarity",best_image->sharpness},
                    {"Similarity",best_image->similarity},
                    {"Effective Area",best_image->effective_area},
                    {"AI_Diagnosis",img_ai_res},
                    {"Normal_Clarity",best_image->normalized_sharpness},
                    {"Medical Diagnosis","NA"},
                    {"AI_Probability",img_ai_prob},
                };
                send_lock.unlock();
            }
            else {
                if (_bgcamera->m_show_save_image_debug_info == 1) {
                    qDebug() << "Fail to save image:" << filePath;
                }
            }
        }
#else
        // 若未启用数据库，则默认将图片手工特征存放在图片名中
        if (best_image->save == true) {
            QString fileName = QString::number(best_image->image_num)           // 序号
                + "_" + QString::number(best_image->total_image_count)          // 帧号
                + "_" + QString::number(best_image->sharpness)                  // 清晰度
                + "_" + QString::number(best_image->similarity)                 // 相似度
                + "_" + QString::number(best_image->magnification)              // 倍率
                + "_" + QString::number(best_image->effective_area)             // 有效区域面积
                + "_" + QString::number(best_image->normalized_sharpness)       // 归一化后的清晰度
                + "_" + _img_inference_result +                                 // AI诊断结果+置信度
                +"_" + "NA"                                                     // 医生诊断结果，默认为NA
                + ".png";                                                       // 保存为PNG格式

            QString save_dir = _bgcamera->m_save_dir;
            QString filePath = save_dir + "/" + fileName;

            // 判断图片是否保存成功
            if (best_image->image.save(filePath)) {
                emit _bgcamera->signal_show_image(best_image, image_inference_res, video_res);
                if (_bgcamera->m_show_save_image_debug_info == 1) {
                    qDebug() << "Success to save image:" << filePath;
                }
            }
            else {
                if (_bgcamera->m_show_save_image_debug_info == 1) {
                    qDebug() << "Fail to save image:" << filePath;
                }
            }
        }

#endif // FTP_SEND


        // 保存完图片后，退出AI处理线程。退出时将变量m_free_thread_num加一，表示当前被占用的AI处理线程被释放
        mem_lock.lock();
        _bgcamera->m_free_thread_num++;
        mem_lock.unlock();
    }

#ifdef FTP_SEND
    if (!db_connect_ret) {
        dbSockClose(db_client_sock);
    }
#endif // FTP_SEND

    delete deployer;
    return;
}
   

void bgCamera::enable_deal_thd(unsigned int deal_cnt)
{
    if (deal_flag == 1)
    {
        return;
    }

    deal_flag = 1;
    deal_thd_cnt = deal_cnt;
    for (unsigned int i = 0; i < deal_cnt; i++)
    {
        m_deal_thread[i] = std::thread(deal_thd, this);
    }
    qDebug() << "enable_deal_thd.";
}

void bgCamera::disable_deal_thd()
{
    if (deal_flag == 0)
    {
        return;
    }

    deal_flag = 0;
    for (unsigned int i = 0; i < deal_thd_cnt; i++)
    {
        m_deal_thread[i].join();
    }
}


//void bgCamera::slot_on_handle_file_test(QString file_path)
//{
//    unsigned width = DEFUALT_WIDTH, height = DEFUALT_HEIGHT;
//    QImage img;
//    img.load(file_path);
//
//    {
//        int _size = TDIBWIDTHBYTES(width * 24) * height;  //59535360=5440*3648*3
//        QImage newimage = img.scaled(m_lbl_video->width(), m_lbl_video->height(),
//            Qt::KeepAspectRatio, Qt::FastTransformation);
//        m_lbl_video->setPixmap(QPixmap::fromImage(newimage));       //将相机传来的图片显示在中心屏幕
//
//
//
//
//                
//
//        if (m_color_res == 0) {
//            return;
//        }
//
//        //    // 发送给FPGA，读取清晰度，相似度，归一化后的清晰度，有效组织区域大小，帧号
//        //    {
//
//        unsigned int read_sharp = 16;       // 清晰度
//        unsigned int read_frame = 16;       // 帧号
//        unsigned int read_similarity = 16;      // 相似度
//        unsigned int read_eff_area = 16;        // 有效组织区域大小
//        unsigned int norm_clear = 16;       // 归一化后的清晰度
//        unsigned int slide_class = 1;       //切片类型，如H&E,CD138-MUM1等
//        unsigned int read_low_area = 16;
//        unsigned int read_med_area = 16;
//        m_pData = img.bits();
//
//        uchar* _data = (uchar*)malloc(_size);
//        uchar* _temp_data = _data;
//      /*  for (uchar i = 0; i < 136; i++) {
//            for (uchar j = 0; j < 256; j++) {
//                for (uchar k = 0; k < 256; k++) {
//                    if ((256 * 256 * i + 256 * j + k) < 1824 * 1216 * 4) {
//                        memcpy(_data + uchar(3) * (256 * 256 * i + 256 * j + k), m_pData + uchar(4) * (256 * 256 * i + 256 * j + k), 3);
//                    }
//                }
//            }
//        }*/
//        for (int i = 0; i < 1824 * 1216; i++) {
//
//            memcpy(_temp_data , m_pData , 3);
//            _temp_data = _temp_data + uchar(1) + uchar(1) + uchar(1);
//            m_pData = m_pData + uchar(1) + uchar(1) + uchar(1) + uchar(1);
//
//        }
//
//
//        int ret = get_blur_from_fpga(_data, m_hDevice, trans_cnt, m_color_res, slide_class, read_sharp, read_frame, read_similarity, read_eff_area);
//        read_med_area = 0x000003ff & (read_eff_area >> 10);
//        //read_eff_area = read_med_area;
//
//        m_pData = nullptr;
//        if (ret == -1) {
//            qDebug() << "Failed to read value from FPGA.";
//        }
//        if (read_eff_area != 0) {
//            norm_clear = (read_sharp * 1.0 / static_cast<double>(read_med_area)) * 450;
//
//        }
//        else {
//            norm_clear = 0x00000000;
//        }
//        if (m_show_fpga_debug_info == 1) {
//            qDebug() << "---read_frame" << read_frame << "read_sharp: " << read_sharp << "read_similarity: " << read_similarity 
//                << "last_similarity" << m_last_similarity  << "efficient_area:" << read_eff_area << "norm_clear" << norm_clear;
//            //m_lbl_debug->setText(QString::asprintf("Fn:%u, cl:%u, si:%u, eff:%u, nor:%u .", read_frame, read_sharp, read_similarity, read_eff_area, norm_clear));
//        }
//
//
//        // 对图片进行条件筛选，若符合筛选条件且有空闲的节点则将图片挂载到deal链表上进行AI处理，否则丢图，直接返回
//        int _max_num = 2;       // 当显微镜头静止不动时能输出的图片的最大数量
//        unsigned int _similarity = m_similarity;        // 相似度阈值
//        unsigned int _sharp = m_sharp;      // 清晰度阈值
//        unsigned int _area = m_area;
//
//
//        if ((m_last_similarity > _similarity && read_similarity <= _similarity) || (m_last_similarity <= _similarity && read_similarity <= _similarity)) {
//            m_last_similarity = read_similarity;
//            if (m_same_best_img_num < _max_num) {
//                if (m_similar_img_num < 5) {
//                    m_similar_img_num++;
//                    if (norm_clear > _sharp && read_eff_area > _area && norm_clear > m_best_img_sharpness) {
//                        signal_show_preview_img(QPixmap::fromImage(img), m_best_img_num + 1);
//                        m_best_img = img.copy();
//                        m_best_img_frame = read_frame;
//                        m_best_img_sharpness = read_sharp;
//                        m_best_img_similarity = read_similarity;
//                        m_best_img_mag = m_color_res;
//                        m_best_eff_area = read_eff_area;
//                        m_best_norm_clear = norm_clear;
//                    }
//                }
//                if (m_similar_img_num == 5) {
//                    m_similar_img_num = 0;
//                    if (m_best_img_sharpness != 0) {
//                        HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();
//                        if (img_node == NULL) {
//                            qDebug() << "当前free链表没有空闲节点，丢失图片：" << m_best_img_frame;
//                            m_best_img_frame = 0;
//                            m_best_img_sharpness = 0;
//                            m_best_img_similarity = 0;
//                            m_best_img_mag = 0;
//                            m_best_eff_area = 0;
//                            m_best_norm_clear = 0;
//                            return;
//                        }
//                        m_best_img_num++;
//                        m_same_best_img_num++;
//                        signal_show_preview_img(QPixmap::fromImage(m_best_img), m_best_img_num);
//                        img_node->m_best_img = m_best_img.copy();
//                        img_node->m_best_img_num = m_best_img_num;
//                        img_node->m_cnt = m_best_img_frame;
//                        img_node->m_best_img_sharpness = m_best_img_sharpness;
//                        img_node->m_best_img_similartiy = m_best_img_similarity;
//                        img_node->m_best_img_eff_area = m_best_eff_area;
//                        img_node->m_best_img_norm_clear = m_best_norm_clear;
//                        img_node->m_mag_num = m_best_img_mag;
//                        img_node->m_save = m_save_img;
//                        //填充好内容后，将结点挂载到used_list链表上，由deal_thd函数的子线程处理
//                        m_img_pool->fill_deal_mem_pool(img_node);
//                        img_node = NULL;
//                        m_best_img_frame = 0;
//                        m_best_img_sharpness = 0;
//                        m_best_img_similarity = 0;
//                        m_best_img_mag = 0;
//                        m_best_eff_area = 0;
//                        m_best_norm_clear = 0;
//                    }
//                }
//            }
//            else {
//                m_similar_img_num = 0;
//                m_best_img_frame = 0;
//                m_best_img_sharpness = 0;
//                m_best_img_similarity = 0;
//                m_best_img_mag = 0;
//                m_best_eff_area = 0;
//                m_best_norm_clear = 0;
//            }
//        }
//        else if (m_last_similarity <= _similarity && read_similarity > _similarity) {
//            m_last_similarity = read_similarity;
//            do {
//                if (m_best_img_sharpness != 0) {
//                    m_similar_img_num = 0;
//                    if (m_same_best_img_num < _max_num) {
//                        m_same_best_img_num = 0;
//                        HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();
//                        if (img_node == NULL) {
//                            qDebug() << "当前free链表没有空闲节点，丢失图片：" << m_best_img_frame;
//                            m_best_img_frame = 0;
//                            m_best_img_sharpness = 0;
//                            m_best_img_similarity = 0;
//                            m_best_img_mag = 0;
//                            m_best_eff_area = 0;
//                            m_best_norm_clear = 0;
//                            break;
//                        }
//                        m_best_img_num++;
//                        signal_show_preview_img(QPixmap::fromImage(m_best_img), m_best_img_num);
//                        img_node->m_best_img = m_best_img.copy();
//                        img_node->m_best_img_num = m_best_img_num;
//                        img_node->m_cnt = m_best_img_frame;
//                        img_node->m_best_img_sharpness = m_best_img_sharpness;
//                        img_node->m_best_img_similartiy = m_best_img_similarity;
//                        img_node->m_mag_num = m_best_img_mag;
//                        img_node->m_best_img_eff_area = m_best_eff_area;
//                        img_node->m_best_img_norm_clear = m_best_norm_clear;
//                        img_node->m_save = m_save_img;
//                        //填充好内容后，将结点挂载到used_list链表上，由deal_thd函数的子线程处理
//                        m_img_pool->fill_deal_mem_pool(img_node);
//                        img_node = NULL;
//                        m_best_img_frame = 0;
//                        m_best_img_sharpness = 0;
//                        m_best_img_similarity = 0;
//                        m_best_img_mag = 0;
//                        m_best_eff_area = 0;
//                        m_best_norm_clear = 0;
//                    }
//                    else {
//                        m_same_best_img_num = 0;
//                        m_best_img_frame = 0;
//                        m_best_img_sharpness = 0;
//                        m_best_img_similarity = 0;
//                        m_best_img_mag = 0;
//                        m_best_eff_area = 0;
//                        m_best_norm_clear = 0;
//                    }
//                }
//            } while (false);
//
//            if (norm_clear > _sharp && read_eff_area > _area) {
//                HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();
//                if (img_node == NULL) {
//                    qDebug() << "当前free链表没有空闲节点，丢失图片：" << read_frame;
//                    m_best_img_frame = 0;
//                    m_best_img_sharpness = 0;
//                    m_best_img_similarity = 0;
//                    m_best_img_mag = 0;
//                    m_best_eff_area = 0;
//                    m_best_norm_clear = 0;
//                    return;
//                }
//                m_best_img_num++;
//                signal_show_preview_img(QPixmap::fromImage(img), m_best_img_num);
//                img_node->m_best_img = img.copy();
//                img_node->m_best_img_num = m_best_img_num;
//                img_node->m_cnt = read_frame;
//                img_node->m_best_img_sharpness = read_sharp;
//                img_node->m_best_img_similartiy = read_similarity;
//                img_node->m_best_img_eff_area = read_eff_area;
//                img_node->m_best_img_norm_clear = norm_clear;
//                img_node->m_mag_num = m_color_res;
//                img_node->m_save = m_save_img;
//                //填充好内容后，将结点挂载到used_list链表上，由deal_thd函数的子线程处理
//                m_img_pool->fill_deal_mem_pool(img_node);
//                img_node = NULL;
//                m_best_img_frame = 0;
//                m_best_img_sharpness = 0;
//                m_best_img_similarity = 0;
//                m_best_img_mag = 0;
//                m_best_eff_area = 0;
//                m_best_norm_clear = 0;
//            }
//        }
//        else if (m_last_similarity > _similarity && read_similarity > _similarity) {
//            m_last_similarity = read_similarity;
//            if (norm_clear > _sharp && read_eff_area > _area) {
//                HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();
//                if (img_node == NULL) {
//                    qDebug() << "当前free链表没有空闲节点，丢失图片：" << read_frame;
//                    m_best_img_frame = 0;
//                    m_best_img_sharpness = 0;
//                    m_best_img_similarity = 0;
//                    m_best_img_mag = 0;
//                    m_best_eff_area = 0;
//                    m_best_norm_clear = 0;
//                    return;
//                }
//                m_best_img_num++;
//                signal_show_preview_img(QPixmap::fromImage(img), m_best_img_num);
//                img_node->m_best_img = img.copy();
//                img_node->m_best_img_num = m_best_img_num;
//                img_node->m_cnt = read_frame;
//                img_node->m_best_img_sharpness = read_sharp;
//                img_node->m_best_img_similartiy = read_similarity;
//                img_node->m_best_img_eff_area = read_eff_area;
//                img_node->m_best_img_norm_clear = norm_clear;
//                img_node->m_mag_num = m_color_res;
//                img_node->m_save = m_save_img;
//                //填充好内容后，将结点挂载到used_list链表上，由deal_thd函数的子线程处理
//                m_img_pool->fill_deal_mem_pool(img_node);
//                img_node = NULL;
//                m_best_img_frame = 0;
//                m_best_img_sharpness = 0;
//                m_best_img_similarity = 0;
//                m_best_img_mag = 0;
//                m_best_eff_area = 0;
//                m_best_norm_clear = 0;
//            }
//        }
//        else {
//            m_last_similarity = 1024;
//            m_same_best_img_num = 0;
//            m_similar_img_num = 0;
//            m_best_img_frame = 0;
//            m_best_img_sharpness = 0;
//            m_best_img_similarity = 0;
//            m_best_img_mag = 0;
//            m_best_eff_area = 0;
//            m_best_norm_clear = 0;
//        }
//
//        free(_data);
//
//    }  //pull img
//}


void bgCamera::slot_on_handle_file_test(QString file_path) {
        unsigned width = DEFUALT_WIDTH, height = DEFUALT_HEIGHT;
        QImage image;
        image.load(file_path);

        {
            int _size = TDIBWIDTHBYTES(width * 24) * height;  //59535360=5440*3648*3
            QImage newimage = image.scaled(m_lbl_video->width(), m_lbl_video->height(),
                Qt::KeepAspectRatio, Qt::FastTransformation);
            m_lbl_video->setPixmap(QPixmap::fromImage(newimage));       //将相机传来的图片显示在中心屏幕

            if (m_color_res == 0) {
                return;
            }

            unsigned int read_sharp = 16;       // 清晰度
            unsigned int read_frame = 16;       // 帧号
            unsigned int read_similarity = 16;      // 相似度
            unsigned int read_eff_area = 16;        // 有效组织区域大小
            unsigned int norm_clear = 16;       // 归一化后的清晰度
            unsigned int slide_class = 1;       //切片类型，如H&E,CD138-MUM1等
            unsigned int read_low_area = 16;
            unsigned int read_med_area = 16;
            unsigned int read_high_area = 16;
            m_pData = image.bits();
        
            uchar* _data = (uchar*)malloc(_size);
            uchar* _temp_data = _data;

            for (int i = 0; i < 1824 * 1216; i++) {
                memcpy(_temp_data , m_pData , 3);
                _temp_data = _temp_data + uchar(1) + uchar(1) + uchar(1);
                m_pData = m_pData + uchar(1) + uchar(1) + uchar(1) + uchar(1);
            }

            // 发送给FPGA，读取清晰度，相似度，归一化后的清晰度，有效组织区域大小，帧号
            int ret = get_blur_from_fpga(m_pData, m_hDevice, trans_cnt, m_color_res, slide_class, read_sharp, read_frame, read_similarity, read_eff_area);
            if (ret == -1) {
                qDebug() << "Failed to read value from FPGA.";
                return;
            }


            read_med_area = (read_eff_area >> 10) & 0x000003ff;
            read_high_area = (read_eff_area >> 20) & 0x000003ff;
            read_eff_area = read_high_area;
            if (read_eff_area != 0) {
                norm_clear = (read_sharp * 1.0 / static_cast<double>(read_high_area)) * 450;
            }
            else {
                norm_clear = 0x00000000;
            }


            // 是否在控制台打印FPGA读取信息
            if (m_show_fpga_debug_info == 1) {
                qDebug() << "---read_frame" << read_frame << "read_sharp: " << read_sharp << "read_similarity: " << read_similarity << "last_similarity" << m_last_similarity << "efficient_area:" << read_eff_area << "norm_clear" << norm_clear;
            }
            m_lbl_debug->setText(QString::asprintf("Frame_num = %u, Similarity = %u, Sharpness = %u, Eff_area = %u, Norm_clear = %u", read_frame, read_similarity, read_sharp, read_eff_area, norm_clear));


            // 预定义筛选条件
            int _max_num = 2;                                       // 当显微镜头静止不动时能输出的图片的最大数量
            unsigned int _similarity = m_similarity;                // 相似度阈值
            unsigned int _sharp = m_sharp;                          // 清晰度阈值
            unsigned int _area = m_area;                            // 愉快面积阈值


            // 对图片进行条件筛选，若符合筛选条件且有空闲的节点时，将图片挂载到deal链表上进行AI处理；否则丢图，直接返回
            if ((m_last_similarity > _similarity && read_similarity <= _similarity) || (m_last_similarity <= _similarity && read_similarity <= _similarity)) {
                m_last_similarity = read_similarity;

                if (m_same_best_img_num < _max_num) {
                    if (m_similar_img_num < 5) {
                        m_similar_img_num++;
                        // 筛选最优图片
                        if (norm_clear > _sharp && read_eff_area > _area && norm_clear > m_best_image->sharpness) {
                            // 预览图片
                            emit signal_show_preview_img(QPixmap::fromImage(image.copy()), m_best_img_num + 1);

                            QString time_stamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");

                            // 确保释放之前记录图片所用的内存
                            if (m_best_image) {
                                delete m_best_image;
                                m_best_image = nullptr;
                            }
                            m_best_image = new BestImage(image.copy(), time_stamp, m_best_img_num, read_frame, read_sharp, read_similarity, read_eff_area, norm_clear, m_color_res, m_save_img);
                        }
                    }
                    if (m_similar_img_num == 5) {
                        m_similar_img_num = 0;
                        if (m_best_image->sharpness != 0) {
                            HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();
                            if (img_node == NULL) {
                                qDebug() << "当前free链表没有空闲节点，丢失图片：" << m_best_image->total_image_count;

                                m_best_image->clear();
                                return;
                            }
                            m_best_img_num++;
                            m_same_best_img_num++;
                            signal_show_preview_img(QPixmap::fromImage(m_best_image->image.copy()), m_best_img_num);

                            img_node->best_image = BestImage(m_best_image);

                            img_node->best_image.image_num = m_best_img_num;

                            // 填充好内容后，将结点挂载到used_list链表上，由deal_thd函数的子线程处理
                            m_img_pool->fill_deal_mem_pool(img_node);
                            img_node = NULL;
                            m_best_image->clear();
                        }
                    }
                }
                else {
                    m_best_image->clear();
                }
            }
            else if (m_last_similarity <= _similarity && read_similarity > _similarity) {
                m_last_similarity = read_similarity;
                do {
                    if (m_best_image->sharpness != 0) {
                        m_similar_img_num = 0;

                        if (m_same_best_img_num < _max_num) {
                            m_same_best_img_num = 0;
                            HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();

                            if (img_node == NULL) {
                                qDebug() << "当前free链表没有空闲节点，丢失图片：" << m_best_image->total_image_count;
                                m_best_image->clear();
                                break;
                            }

                            m_best_img_num++;
                            signal_show_preview_img(QPixmap::fromImage(m_best_image->image.copy()), m_best_img_num);

                            img_node->best_image = BestImage(m_best_image);
                            img_node->best_image.image_num = m_best_img_num;

                            //填充好内容后，将结点挂载到used_list链表上，由deal_thd函数的子线程处理
                            m_img_pool->fill_deal_mem_pool(img_node);
                            img_node = NULL;
                            m_best_image->clear();
                        }
                        else {
                            m_best_image->clear();
                        }
                    }
                    else {
                        m_similar_img_num = 0;
                        m_same_best_img_num = 0;
                    }
                } while (false);

                if (norm_clear > _sharp && read_eff_area > _area) {
                    HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();
                    if (img_node == NULL) {
                        qDebug() << "当前free链表没有空闲节点，丢失图片：" << m_best_image->image_num;
                        m_best_image->clear();
                        return;
                    }
                    m_best_img_num++;
                    signal_show_preview_img(QPixmap::fromImage(image.copy()), m_best_img_num);

                    // 确保释放之前记录图片所用的内存
                    if (m_best_image) {
                        delete m_best_image;
                        m_best_image = nullptr;
                    }
                    QString time_stamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
                    m_best_image = new BestImage(image.copy(), time_stamp, m_best_img_num, read_frame, read_sharp, read_similarity, read_eff_area, norm_clear, m_color_res, m_save_img);

                    img_node->best_image = BestImage(m_best_image);

                    //填充好内容后，将结点挂载到used_list链表上，由deal_thd函数的子线程处理
                    m_img_pool->fill_deal_mem_pool(img_node);
                    img_node = NULL;
                    m_best_image->clear();
                }
            }
            else if (m_last_similarity > _similarity && read_similarity > _similarity) {
                m_last_similarity = read_similarity;
                if (norm_clear > _sharp && read_eff_area > _area) {
                    HL_IMG_POOL_NODE* img_node = m_img_pool->malloc_free_mem_pool();
                    if (img_node == NULL) {
                        qDebug() << "当前free链表没有空闲节点，丢失图片：" << m_best_image->image_num;
                        m_best_image->clear();

                        return;
                    }
                    m_best_img_num++;
                    signal_show_preview_img(QPixmap::fromImage(image.copy()), m_best_img_num);

                    // 确保释放之前记录图片所用的内存
                    if (m_best_image) {
                        delete m_best_image;
                        m_best_image = nullptr;
                    }
                    QString time_stamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
                    m_best_image = new BestImage(image.copy(), time_stamp, m_best_img_num, read_frame, read_sharp, read_similarity, read_eff_area, norm_clear, m_color_res, m_save_img);

                    img_node->best_image = BestImage(m_best_image);

                    //填充好内容后，将结点挂载到used_list链表上，由deal_thd函数的子线程处理
                    m_img_pool->fill_deal_mem_pool(img_node);
                    img_node = NULL;
                    m_best_image->clear();
                }
            }
        }
};

void bgCamera::slot_on_rect_value_changed(int _rectX, int _rectY) {
    m_magDetWorkwer->m_rectX = _rectX;
    m_magDetWorkwer->m_rectY= _rectY;
}


