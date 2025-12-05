#include "microslidewindow.h"
#include "DBInterfaceDLL.h"
#include "ui_microslidewindow.h"
#include <unordered_map>
#include <QCamera>
#include <QLabel>
#include <QMessageBox>
#include <QButtonGroup>
#include <QScrollBar>
#include <QInputDialog>
#include <QFileDialog>
#include <QFile>
#include <QTimer>
#include <QDateTime>
#include <QThread>
#include <thread>
#include <codecvt>
#include "db_raw.h"
#include "hpec_lib.h"
#include "sysconfigform.h"
#include "deployment.h"
#include "magdebug.h"
#include "IniParser.h"
#include "filetest.h"
#include "CamImgPool.h"
#include "scanner_processor.h"


MicroSlideWindow::MicroSlideWindow(QWidget* parent) :
    QMainWindow(parent),
    //m_img_save_path(""), 
    ui(new Ui::MicroSlideWindow),
    ft(nullptr),
    sysCfgForm(nullptr),
    magdebug(nullptr),
    m_scanner(nullptr),
    is_scannerConnected(true),
    m_scannerTimer(new QTimer(this)),
    m_scannerChecker(new QTimer(this))
{
    //拿到设备总线id
    //initBusNumWindow();
    initApp();
    ui->setupUi(this); //this指针指向当前正在创建的MicroSlideWindow对象
    initUI();
    initTableImg();
    initDataBase();
    initVariable();     //将powerPC的设备id号传给bgcamera

    ui->lineEdit_id->setFocus();

    // 初始化扫描仪设备
    //setup_scanner_connections();
    //initialize_scanner();


    connect(m_scannerTimer, &QTimer::timeout,
        this, &MicroSlideWindow::try_initialize_scanner);
    m_scannerTimer->setInterval(5000);
    connect(m_scannerChecker, &QTimer::timeout,
        this, &MicroSlideWindow::check_scanner_usb_status);
    m_scannerChecker->setInterval(20000);

    set_scanner_state(false);


    qRegisterMetaType<QVideoFrame>("QVideoFrame");
    connect(ui->action_magdebug,&QAction::triggered,this,[this](){
        if (ui->widget_camera->m_trans == true) {
            ui->widget_camera->onBtnTrans();
        }
        magdebug = nullptr;
        magdebug = new MagDebug();
        magdebug->setAttribute(Qt::WA_DeleteOnClose);
        magdebug->setWindowModality(Qt::ApplicationModal);
        magdebug->setWindowTitle("倍率调试");
        connect(magdebug, &MagDebug::signal_rect_value_changed, ui->widget_camera, &bgCamera::slot_on_rect_value_changed);
        connect(magdebug, &MagDebug::destroyed, [this] {
            ui->widget_camera->on_m_cbox_magStateChanged(Qt::CheckState::Unchecked);
            ui->widget_camera->on_m_cbox_magStateChanged(Qt::CheckState::Checked);
            magdebug = nullptr;   
            });
        magdebug->show();

    });

}

MicroSlideWindow::~MicroSlideWindow()
{

    release_scanner();
    CloseApp(m_cur_bus_num);	//关闭应用程序
    delete ui;

}

void MicroSlideWindow::initApp()
{
    //应用程序开启阶段调用，只能调用一次
    //描述：初始化上层接口资源
    if(PowerPCExitCheck() == -1)
    {
        QMessageBox::warning(this, "Warning", "未检测到PowerPC.");
        qDebug() << "未检测到PowerPC.";
        // exit(1);
        // return;
    }else{

        m_cur_bus_num = bus_num_get(0);
        if (m_cur_bus_num == -1) {
            qDebug() << "获取总线id失败";
        }
        else {
            qDebug() << "获取总线id成功：" << m_cur_bus_num ;
        }
        //初始化应用程序
        int ret = InitApp(m_cur_bus_num);
        if(ret == 0){
            qDebug() << "初始化设备成功";
        }else{
            qDebug()<<"初始化设别失败.";
        }

    }
}

void MicroSlideWindow::initBusNumWindow()
{
    qDebug() <<"initBusNumWindow";
    // 获取用户输入的数字
    bool ok;
    int number = QInputDialog::getInt(nullptr, QObject::tr("输入设备总线"),       //标题
                                      QObject::tr("请输入总线num:"),     //提示语
                                      0, // 默认值
                                      0, // 最小值
                                      12, // 最大值
                                      1, // 步长
                                      &ok);     

    if (ok) {
        qDebug() << "User entered number:" << number;
        m_cur_bus_num = number;
    } else {
        qDebug() << "User canceled the input.";
        exit(0);
    }
}

//将powerPC的设备id号传给bgcamera
void MicroSlideWindow::initVariable()
{
    qDebug() <<"initVariable";
    ui->widget_camera->set_cur_bus_num(m_cur_bus_num);      
    connect(ui->widget_camera,&bgCamera::signal_show_image,
            this,&MicroSlideWindow::slot_add_image_to_table);
    connect(ui->widget_camera, &bgCamera::signal_show_preview_img,
        this, &MicroSlideWindow::slot_add_preview_img_to_table_line);
    connect(ui->widget_camera, &bgCamera::signal_delete_preview_img,
        this, &MicroSlideWindow::slot_delete_preview_img_from_table_line);
    connect(ui->widget_camera, &bgCamera::signal_show_invalid_img,
        this, &MicroSlideWindow::slot_change_preview_img_from_table_line);
    connect(this, &MicroSlideWindow::signal_ai_inference_finished,
        this, &MicroSlideWindow::slot_on_ai_inference_finished);
    connect(ui->widget_camera, &bgCamera::directly_click_btn_trans,
        this, &MicroSlideWindow::on_btn_id_clicked);
    connect(ui->widget_camera, &bgCamera::signal_show_db_connect_warning,
        this, [this]() {QMessageBox::warning(this, tr("warning"), tr("连接数据库失败，请检查网络连接并重启应用程序!")); });
}

json convert2json(IF_QUE* que) {
    json res = {
        {"image_dir",que->image_dir},
        {"feature_path",que->feature_path},
        {"pathological_id",que->pathological_id},
        {"slice_id",que->slice_id},
        {"pathological_order",que->pathological_order},
        {"port",que->port},
        {"ftpnode",que->ftpnode},
        {"ftpusr",que->ftpusr},
        {"ftppwd",que->ftppwd},
    };
    return res;
}

void MicroSlideWindow::initTableImg()
{
    qDebug() <<"initTableImg";
    // 设置表头字体
    QFont font("Microsoft YaHei", 10);
    font.setBold(true);
    ui->table_img->horizontalHeader()->setFont(font);

    // 设置最后一列拉伸至最大
    ui->table_img->horizontalHeader()->setStretchLastSection(true);

    // 表头颜色
    ui->table_img->horizontalHeader()->setStyleSheet(
        "QHeaderView::section{background:#b0b0b0}");

    // 设置只可以单选，可以使用ExtendedSelection进行多选
    ui->table_img->setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置选择单个单元格
    ui->table_img->setSelectionBehavior(QAbstractItemView::SelectItems);

    // 设置列宽度
    ui->table_img->horizontalHeader()->resizeSection(0, PIC_WIDTH);
    ui->table_img->horizontalHeader()->resizeSection(1, 50);
    ui->table_img->horizontalHeader()->resizeSection(2, 70);

    // 设置3列
    ui->table_img->setColumnCount(3);
    QStringList str_list = { "图片", "结果", "置信度" };
    ui->table_img->setHorizontalHeaderLabels(str_list);

    // 禁止点表头的列
    ui->table_img->horizontalHeader()->setSectionsClickable(false);

    // 设置表格不可编辑
    ui->table_img->setEditTriggers(QAbstractItemView::NoEditTriggers);

    //禁止表头拖动
    ui->table_img->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    // 设置滑杆滑动步数变小
    // 设置为像素移动
    ui->table_img->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->table_img->verticalScrollBar()->setSingleStep(10);  // 设置越小下滑越慢

     // 设置上下文菜单策略为自定义
    ui->table_img->setContextMenuPolicy(Qt::CustomContextMenu);

    // 连接自定义上下文菜单请求信号
    connect(ui->table_img, &QTableWidget::customContextMenuRequested, this, &MicroSlideWindow::customContextMenuRequested);

}



void MicroSlideWindow::initUI()
{
    qDebug() <<"initUI";

    //复选诊断框
    QButtonGroup *buttonGroup = new QButtonGroup(this);
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

    // 互斥模式设置为false,使按钮可以多选
    buttonGroup->setExclusive(false); 
}

// 将图片添加到左侧栏中的槽函数，与五张出一张配合,即一直将best_img放到左栏，5张后确定最优图片并给出模型结果，增强显示的实时性
void MicroSlideWindow::slot_add_preview_img_to_table_line(QPixmap _preview_img, int _best_img_num) {
    ui->table_img->setRowCount(_best_img_num);      //设置行数
    int row = _best_img_num - 1;
    int col = 0;

    QLabel* item1 = new QLabel(this);

    double ratio = _preview_img.height() * 1.0 / _preview_img.width();
    item1->setAlignment(Qt::AlignmentFlag::AlignCenter);
    item1->setFixedWidth(PIC_WIDTH);
    item1->setFixedHeight(PIC_WIDTH * ratio);

    // 将QPixmap 对象 pix 按宽度缩放到 PIC_WIDTH，并设置为 QLabel 的图片
    item1->setPixmap(_preview_img.scaledToWidth(PIC_WIDTH));
    item1->setScaledContents(true); //设置 QLabel 的内容（图片）在必要时可以缩放以适应标签的大小

    //添加图片
    ui->table_img->setCellWidget(row, col, item1);

    //更改行高
    ui->table_img->setRowHeight(row, item1->height());

    //添加文字
    QLabel* item2 = new QLabel("Loading", this);
    item2->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);      //设置为水平且垂直居中
    ui->table_img->setCellWidget(row, ++col, item2);

    QLabel* item3 = new QLabel("Loading", this);
    item3->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);      //设置为水平且垂直居中
    ui->table_img->setCellWidget(row, ++col, item3);


    ui->table_img->scrollToBottom(); //将 table_img 的视图滚动到底部，以便用户可以看到新添加的图片
}

void MicroSlideWindow::slot_delete_preview_img_from_table_line(int _best_img_num) {
    ui->table_img->setRowCount(_best_img_num);      //设置行数

}

void MicroSlideWindow::slot_change_preview_img_from_table_line(unsigned int _best_img_num)
{
    int row = _best_img_num-1;
    int col = 0;

    //添加文字
    QLabel* item2 = new QLabel("Invalid", this);
    item2->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);      //设置为水平且垂直居中
    ui->table_img->setCellWidget(row, ++col, item2);

    QLabel* item3 = new QLabel("Invalid", this);
    item3->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);      //设置为水平且垂直居中
    ui->table_img->setCellWidget(row, ++col, item3);
}


void MicroSlideWindow::initDataBase()
{
    qDebug() << "initDataBase";
    //创建数据库连接

    //两个文件
    QString _name1 = "HPEC_database";
    createConnectionByName("firstConnect", _name1);
    db = getConnectionByName("firstConnect");
    //与数据库建立连接
    QSqlQuery query(db);
    //创建一个表格
    query.exec("CREATE TABLE IF NOT EXISTS patients "
        "(patient_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL"
        ",name VARCHAR(50) DEFAULT 'unknown' "
        ",gender CHAR(1) DEFAULT '男'"
        ",age INT DEFAULT 50"
        ",diagnosis_time DATETIME DEFAULT CURRENT_TIMESTAMP"
        ",diagnosis_result TEXT DEFAULT 'unknown' "
        ",confidence_percentage DECIMAL(5,2) DEFAULT 50.000)");
    // query.exec("CREATE TABLE IF NOT EXISTS patients "
    //            "(病人编号 INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
    //            "姓名 VARCHAR(50) DEFAULT 'unknown', "
    //            "性别 CHAR(1) DEFAULT '男', "
    //            "年龄 INT DEFAULT 50, "
    //            "诊断日期 DATETIME DEFAULT CURRENT_TIMESTAMP, "
    //            "病理结果 TEXT DEFAULT 'unknown')");
    //插入内容
    // query.exec("INSERT INTO patients (patient_id, name, gender, age) "
    //            "VALUES (0, '被试一', '男', 50)");
    // insertUserNameGenderAge(db,"被试一","男",50);
    // insertUserNameGenderAge(db,"被试二","男",46);
    // insertUserNameGenderAge(db,"被试三","男",48);
    // insertUserNameGenderAge(db,"被试四","女",46);
    // insertUserNameGenderAge(db,"被试五","男",48);
    // insertUserNameGenderAge(db,"被试六","男",42);
    // insertUserNameGenderAge(db,"被试七","女",44);
    // insertUserNameGenderAge(db,"被试八","女",45);
    queryAllUser(db);
    model = new QSqlTableModel(this, db);
    model->setTable("patients");   //表格名
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select();
    ui->tableView_db->setModel(model);
    ui->tableView_db->resizeColumnsToContents();
    //设置交替颜色
    ui->tableView_db->setAlternatingRowColors(true);
    ui->tableView_db->setAutoScroll(true);
    ui->tableView_db->verticalHeader()->hide();
    //居中
    ui->tableView_db->setStyleSheet(
        "QTableView::item { "
        "text-align: center; "
        "vertical-align: middle; "
        "}"
    );



    //创建第二个表格
    query.exec("CREATE TABLE IF NOT EXISTS diagnosis_results"
        "(result_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
        "patient_id INT NOT NULL,"
        "slice_id VARCHAR(100) NOT NULL,"
        "magnification INT NOT NULL,"
        "image_path TEXT NOT NULL,"
        "diagnosis_result TEXT NOT NULL,"
        "confidence_percentage DECIMAL(5,2),"
        "relative_x_coordinate DECIMAL(10, 5),"
        "relative_y_coordinate DECIMAL(10, 5),"
        "diagnosis_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (patient_id) REFERENCES patients(patient_id)"
        ");");
    //插入内容
     query.exec("INSERT INTO diagnosis_results "
                "(result_id, patient_id, slice_id, magnification,"
                "image_path,diagnosis_result,confidence_percentage,"
                "relative_x_coordinate,relative_y_coordinate) "
                "VALUES (0, 0, 'xsB2024-07387', 10,'D:/wll/image/xsB2024-07387A_IP_ 0992_IP/4_19316_79748_1_10_IP_0974_NA.png',"
                "'IP',97.4,200,512)");
    // query.exec("INSERT INTO diagnosis_results "
    //            "(result_id, patient_id, slice_id, magnification,"
    //            "image_path,diagnosis_result,confidence_percentage,"
    //            "relative_x_coordinate,relative_y_coordinate) "
    //            "VALUES (1, 1, 'B24-06635', 20,'D:/wll/data/patch/Normal/NM_B2023-65311A_10_video.png',"
    //            "'NM',95.96,100,280)");
    //query.exec("INSERT INTO diagnosis_results "
    //    "(result_id, patient_id, slice_id, magnification,"
    //    "image_path,diagnosis_result,confidence_percentage,"
    //    "relative_x_coordinate,relative_y_coordinate) "
    //    "VALUES (2, 1, 'B24-06635',10,'D:/wll/data/patch/Normal/NM_B2023-65311C_13_video.png',"
    //    "'NM',98.12,200,480)");
    model2 = new QSqlTableModel(this, db);
    model2->setTable("diagnosis_results");   //表格名
    //model2->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model2->setEditStrategy(QSqlTableModel::OnFieldChange); // 设置编辑策略，这里使用OnFieldChange
    model2->select();
    ui->tableView_db_2->setModel(model2);
    ui->tableView_db_2->resizeColumnsToContents();
    //禁止点击表头
    ui->tableView_db_2->horizontalHeader()->setSectionsClickable(false);
    //设置交替颜色
    ui->tableView_db_2->setAlternatingRowColors(true);
    ui->tableView_db_2->setAutoScroll(true);
    ui->tableView_db_2->verticalHeader()->hide();
    //居中
    ui->tableView_db_2->setStyleSheet(
        "QTableView::item { "
        "text-align: center; "
        "vertical-align: middle; "
        "}"
    );


    connect(ui->tableView_db_2,&QTableView::pressed,
            this,&MicroSlideWindow::slot_on_tableView_db_2_double_clicked);

}

void MicroSlideWindow::slot_add_image_to_table(const BestImage* img, const AIResult* img_inference_res, QString _video_res)
{
    QString row_number = QString::number(img->image_num);
    QString mag = "×" + QString::number(img->magnification);
    QImage image = img->image.copy();

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

    int col = 0;   //病理结果读文件名得知
    QLabel* item1 = new QLabel(this);
    double ratio = image.height() * 1.0 / image.width();
    //qDebug() << "ratio: " << ratio <<"; height: " << img.height();
    item1->setAlignment(Qt::AlignmentFlag::AlignCenter);
    item1->setFixedWidth(PIC_WIDTH);
    item1->setFixedHeight(PIC_WIDTH * ratio);
    //调整图片大小和格式
    QPixmap pix = QPixmap::fromImage(image); //将QImage对象转换为QPixmap对象
    item1->setPixmap(pix.scaledToWidth(PIC_WIDTH)); //将 QPixmap 对象 pix 按宽度缩放到 PIC_WIDTH，并设置为 QLabel 的图片
    item1->setScaledContents(true); //设置 QLabel 的内容（图片）在必要时可以缩放以适应标签的大小
    //ui->table_img->setRowCount(row + 1); //在 QTableWidget 控件 table_img 中添加一行


    //添加图片
    ui->table_img->setCellWidget(row_number.toInt() - 1, col, item1); 

    //更改行高
    ui->table_img->setRowHeight(row_number.toInt() - 1, item1->height());

    //添加文字
    QString _num = QString::number(std::round(img_inference_res->diagnosis_prob * 1000.f) / 1000.f);
    QString _res = img_inference_res->diagnosis_res + "\n"  + _num;


    QLabel* item = new QLabel(_res, this);
    //QLabel* item = new QLabel(parts[0], this);        //图片序号，用于调试
    item->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);      //设置为水平且垂直居中
    ui->table_img->setCellWidget(row_number.toInt() - 1, ++col, item);

    //在第三行添加可视化的概率分布
    barChartWidget* prob_visualizer = new barChartWidget(this);
    //std::vector<std::string> cls_name = { "C", "SSA", "TA", "HP", "IP", "NM" };
    std::vector<std::string> cls_name;
    auto it = bgCamera::classes_map.find(this->ui->widget_camera->m_slide_info->slicesource);
    if (it != bgCamera::classes_map.end()) {
        cls_name = it->second;
    }
    else {
        cls_name = bgCamera::classes_map.find(SLICESOURCE_DEFAULT)->second;
    }

    prob_visualizer->setChartData(img_inference_res->cls_prob, cls_name);
    ui->table_img->setCellWidget(row_number.toInt() - 1, ++col, prob_visualizer);

    //更新video诊断结果
    ui->label_video_res->setText(_video_res);

    //更新到最底下，滑动的效果
    ui->table_img->scrollToBottom(); //将 table_img 的视图滚动到底部，以便用户可以看到新添加的图片

#ifdef OPEN_DATA_BASE_STORE
    // TA,0.647你想要第一个和第三个部分
    QString db_res = res_parts[0]; // 获取第一部分，即 "TA"
    QString _confidence = res_parts[1]; // 获取第三个部分，即 "0.647"s
    bool ok;
    float value = _confidence.toFloat(&ok); // 使用toFloat()函数转换为float类型
    int mag_num = parts[4].toInt();
    //更新到数据库
    slot_on_add_data_to_diagnosis_results(m_db_patient_id, m_db_slice_id, mag_num, _path, db_res, value, key_patch.x, key_patch.y);
#endif // OPEN_DATA_BASE_STORE
    if (img) {
        delete img;
        img = nullptr;
    }
    
    if (img) {
        delete img_inference_res;
        img_inference_res = nullptr;
    }
    
}

void MicroSlideWindow::clear_image_in_table()
{
    //int rCnt = ui->table_img->rowCount();
    //for(int i = 0; i < rCnt; ++i){
    //    ui->table_img->removeRow(0);
    //}
    ui->table_img->clearContents();
    ui->table_img->setRowCount(0);
}


std::string wchar_to_string(const wchar_t* wstr) {
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(wstr);
    }
    catch (const std::exception& e) {
        std::cerr << "Conversion error: " << e.what() << std::endl;
        return "";
    }
}


void MicroSlideWindow::scanner_PID_captured(const std::string& input)
{
    std::string PID = input;
    // 处理解码的扫描数据包含换行符'\r'的情况
    if (!input.empty() && input.back() == '\r') {
        PID = PID.substr(0, PID.length() - 1);
    }

    if (ui->widget_camera->m_save_dir != "") {
        QEventLoop loop;
        connect(this, &MicroSlideWindow::doctor_diagnosis_finished,
            &loop, &QEventLoop::quit);

        on_btn_confirm_doc_clicked();

        // 阻塞线程，直到quit被调用
        // 通过阻塞线程来保证on_btn_confirm_doc_clicked()和on_btn_id_clicked()这两个函数的先后执行顺序
        loop.exec(); 

        disconnect(this, &MicroSlideWindow::doctor_diagnosis_finished,
            nullptr, nullptr);
    }
    ui->lineEdit_id->setText(QString::fromStdString(PID));
    on_btn_id_clicked();
}

void MicroSlideWindow::initialize_scanner()
{
    if (m_scanner == nullptr) {
        m_scanner = new ScannerProcessor(this);
    }
    
    // 初始化扫描仪设备
    if (!m_scanner->initialize()) {
        //QMessageBox::warning(this, "警告", tr("未发现扫描仪设备"));
        delete m_scanner;
        m_scanner = nullptr;
        return;
    }
    setup_scanner_connections();
    set_scanner_state(true);
    QMessageBox::warning(this, "提示", tr("扫描仪设备已连接"));
}

void MicroSlideWindow::release_scanner()
{
    if (m_scanner) {
        m_scanner->release();
        set_scanner_state(false);
        delete m_scanner;
        m_scanner = nullptr;
    }
}

void MicroSlideWindow::setup_scanner_connections()
{
    connect(m_scanner, &ScannerProcessor::decodeDataReceived,
        this, &MicroSlideWindow::scanner_PID_captured);
    connect(m_scanner, &ScannerProcessor::error_occured,
        this, &MicroSlideWindow::on_scanner_error_occured);
    connect(m_scanner, &ScannerProcessor::deviceDisconnected,
        this, [this]() {
            this->m_scanner->release();
            //nl_StopListener(m_scanner->m_hDevice);
            //this->m_scanner->m_hDevice = nullptr;
            //this->m_scanner->m_hDeviceList = nullptr;
            //this->m_scanner->m_deviceCount = 0;

            delete m_scanner;
            m_scanner = nullptr;
            set_scanner_state(false);

            QMessageBox::warning(this, "提示", tr("扫描仪设备已退出"));
        });
    //connect(m_scanner, &ScannerProcessor::deviceConnected,
    //    this, [this]() {
    //        if (this->m_scanner->initialize()) {
    //            QMessageBox::warning(this, "提示", tr("扫描仪设备已连接"));
    //        }
    //        else {
    //            QMessageBox::warning(this, "提示", tr("扫描仪设备连接异常"));
    //        };
    //    });
}

void MicroSlideWindow::set_scanner_state(bool isConnected)
{
    if (is_scannerConnected != isConnected) {
        is_scannerConnected = isConnected;
        update_scannerTimer_state();
    }
}

void MicroSlideWindow::update_scannerTimer_state()
{
    if (is_scannerConnected) {
        m_scannerTimer->stop();
        m_scannerChecker->start();
    }
    else {
        m_scannerTimer->start();
        m_scannerChecker->stop();
    }
}

void MicroSlideWindow::on_btn_id_clicked()
{
    // 清空图片栏
    clear_image_in_table();

    // 获取通过扫描枪得到的PID
    QString PID = ui->lineEdit_id->text();
    if(PID == ""){
        // 未输入PID则默认为当前系统时间
        PID = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    }

    // 检查PID中是否包含禁用字符。禁用字符包括：“_”
    if (PID.contains("_")) {
        QMessageBox::warning(this, tr("警告"), tr("PID中不能包含下划线"));
        return;
    }

    // 将PID输入框与确认按钮设定为禁用状态
    ui->lineEdit_id->setEnabled(false);
    ui->btn_id->setEnabled(false);

#ifdef FTP_SEND
    // 向数据库发送病理号、切片号等信息，从PIS系统中获取切片部位、镜下描述等医学信息
    SOCKET db_client_socket;
    int db_connect_res = dbSockInit(db_client_socket, ui->widget_camera->m_db_addr, ui->widget_camera->m_db_port);

    // 检查是否成功初始化了db套接字
    if (!db_connect_res) {
        GET_PID query = {
            PID.toStdString(),                              // 病理号，这里直接发送扫描枪扫描结果
            PID.toStdString().substr(PID.size() - 1, 1),    // 切片号，这里为病理号的最后一个字符
            wchar_to_string(ui->widget_camera->m_cur.id),   // 显微镜唯一ID
            "A01"                                           // 数据包类型
        };
        dbGetPathinfo(&query, db_client_socket, ui->widget_camera->m_slide_info);
        dbSockClose(db_client_socket);

        // 判断从数据库拿到的数据是否有效
        if (ui->widget_camera->m_slide_info->pathological_id == "") {
            QMessageBox::warning(this, tr("警告"), tr("数据库未查询到有效信息！"));

            // 将PID输入框与确认按钮设定为可用状态
            ui->lineEdit_id->setEnabled(true);
            ui->btn_id->setEnabled(true);
            return;
        }

        // 根据数据库返回的信息，确定文件保存目录。保存目录命名格式为：病理号_切片号_诊断序号
        QString pathological_id = QString::fromStdString(ui->widget_camera->m_slide_info->pathological_id);
        QString slice_id = QString::fromStdString(ui->widget_camera->m_slide_info->slice_id);

        QString pathological_order = QString::number(ui->widget_camera->m_slide_info->pathological_order);
        ui->widget_camera->m_save_dir = ui->widget_camera->m_save_root + "/" + pathological_id + "_" + slice_id + "_" + pathological_order;

        // 切换切片，清空图片特征
        ui->widget_camera->m_images_features.clear();

    }
    else {
        QMessageBox::warning(this, tr("警告"), tr("未连接数据库，无法获取切片信息!"));
        // 若未成功连接数据库，则保存路径与不开启数据库功能时的命名相同
        ui->widget_camera->m_save_dir = ui->widget_camera->m_save_root + "/" + PID + "_NA_0000_NA";
    }


#else
    // 若未启用数据库功能，则文件保存目录默认为：PID_NA_0000_NA
    ui->widget_camera->m_save_dir = ui->widget_camera->m_save_root + "/" + PID + "_NA_0000_NA";

#endif // FTP_SEND

    // 为当前病理切片创建保存文件夹
    QDir().mkpath(ui->widget_camera->m_save_dir);


    // 更改病理号后，清除bgcamera类中video_features中记录的视频特征
    ui->widget_camera->m_video_features.clear();
    ui->label_video_res->setText(".........");


    // 初始化与bgcamera筛选图片相关的变量
    ui->widget_camera->m_same_best_img_num = 0;             // 用于判定连续给出相同图片的数量，若大于等于4则不出图
    ui->widget_camera->m_similar_img_num = 0;               // 记录相似的图片的数目，每5张图片输出一次，或移动输出
    ui->widget_camera->m_best_img_num = 0;                  // 记录输出的图片的数量
    ui->widget_camera->m_last_similarity = 1024;            // 上一张图的相似度的初始值，用于送入软件的第一张图的判定
    ui->widget_camera->m_best_image->clear();               // 记录挑选的图片的清晰度

    

#ifdef OPEN_DATA_BASE_STORE
    //从数据读取patient_id
    int lastRow = model->rowCount()-1; // 获取最后一行的行号
    int primaryKeyColumnIndex = model->fieldIndex("patient_id"); // 获取主键列的索引，这里假设主键列名为"result_id"
    QModelIndex lastPrimaryKeyIndex = model2->index(lastRow, primaryKeyColumnIndex); // 获取最后一行主键列的模型索引
    QVariant lastPrimaryKeyValue = model2->data(lastPrimaryKeyIndex); // 获取主键值
    int numm = lastPrimaryKeyValue.toInt();
    m_db_patient_id = numm +1;  
    //插入到patients表中
    insertUserNameGenderAge(db, "被试", "男", 48);
#endif // OPEN_DATA_BASE_STORE


    // 开始传输
    ui->widget_camera->set_trans_state(false);
    if (ui->widget_camera->onBtnTrans() == -1) {
        ui->lineEdit_id->setEnabled(true);
        ui->btn_id->setEnabled(true);
    }
}


void MicroSlideWindow::on_btn_confirm_doc_clicked()
{
    // 若PID为空，则提示未输入患者信息
    if (ui->widget_camera->m_save_dir == "") {
        QMessageBox::warning(this, tr("警告"), tr("未检测到有效的患者数据!"));
        return;
    }

    // 获取医生诊断结果。若医生未诊断，则默认为AI的诊断结果
    QString doc_diagnose = get_doctor_diagnosis_result();        
    if (doc_diagnose == "") {
        QString AI_diagnose = ui->label_video_res->text();
        QStringList parts = AI_diagnose.split(",");        
        doc_diagnose = parts[0];
    }

    ui->btn_confirm_doc->setEnabled(false);
    ui->widget_camera->set_trans_state(true);

    ui->widget_camera->onBtnTrans();

    // 创建子线程，在子线程中轮询AI处理线程数以判断是否处理完了所有图片数据，在等待AI线程处理完所有数据后发出AI推理完成的信号并detach该子线程
    std::thread node_check([this, doc_diagnose]() {
        while (true) {
            QThread::msleep(500);
            if (this->ui->widget_camera->m_free_thread_num != DEAL_THREAD_MAX_NUMS) {
                continue;
            }
            QThread::msleep(500);
            if (this->ui->widget_camera->m_free_thread_num != DEAL_THREAD_MAX_NUMS) {
                continue;
            }
            emit this->signal_ai_inference_finished(doc_diagnose);

#ifdef FTP_SEND
            // 保存切片对应的图片特征包至保存目录
            savejson(&ui->widget_camera->m_images_features, ui->widget_camera->m_save_dir.toStdString(), "images_features.json");

            // 医生点击确认后，再次向数据库发送视频诊断结果，补充医生诊断结果（AI处理线程中发送的信息是不包括医生诊断结果的）
            SOCKET db_client_socket;
            int db_connect_res = dbSockInit(db_client_socket, ui->widget_camera->m_db_addr, ui->widget_camera->m_db_port);

            // 检查是否成功初始化了db套接字
            if (!db_connect_res) {
                std::vector<double> video_res_prob(ui->widget_camera->m_video_res.second.begin(), ui->widget_camera->m_video_res.second.end());

                PATHO_RES video_res = {
                    ui->widget_camera->m_slide_info->pathological_id,                       // 病理号
                    ui->widget_camera->m_slide_info->slice_id,                              // 切片号
                    bgConverter::wchar_to_string_bg(ui->widget_camera->m_cur.id),		    // 传感器编号
                    "1.0",                                                                  // 版本号
                    static_cast<long int>(time(nullptr)),                                   // 诊断时间
                    ui->widget_camera->m_slide_info->pathological_order,	                // 诊断次数
                    ui->widget_camera->m_best_image->total_image_count,	                    // 一个切片号对应图片
                    9,	                                                                    // 图片特征参数个数
                    ui->widget_camera->m_slide_info->pathological_order,                    // 诊断次序
                    ui->widget_camera->m_video_res.first,                                   // AI诊断结果
                    video_res_prob,                                                         // AI诊断概率
                    doc_diagnose.toStdString(),                                             // 医生诊断结果
                    "A02",                                                                  // 包的类型
                };
                dbResultUpload(&video_res, db_client_socket);
                dbSockClose(db_client_socket);
            }
            else {
                QMessageBox::warning(this, tr("警告"), tr("连接数据库失败，诊断结果无法保存至数据库!"));
            }

            // 若开启数据库功能，则将图片和图片特征相关的保存信息放入数据队列，发送给数据库
            std::string queue_dir = "C:\\hpec_temp";
            // 创建队列目录
            if (!std::filesystem::exists(queue_dir) || !std::filesystem::is_directory(queue_dir)) {
                std::filesystem::create_directory(queue_dir);
            };

            IF_QUE queue_data = {
            ui->widget_camera->m_save_dir.toStdString(),
            ui->widget_camera->m_save_dir.toStdString() + "/images_features.json",
            ui->widget_camera->m_slide_info->pathological_id,
            ui->widget_camera->m_slide_info->slice_id,
            ui->widget_camera->m_slide_info->pathological_order,
            ui->widget_camera->m_slide_info->storage_node,
            ui->widget_camera->m_slide_info->port,
            ui->widget_camera->m_slide_info->ftpusr,
            ui->widget_camera->m_slide_info->ftppwd
            };
            json queue_data_json = convert2json(&queue_data);
            savejson(&queue_data_json, queue_dir, ui->widget_camera->m_slide_info->pathological_id + "_" + ui->widget_camera->m_slide_info->slice_id + "_" + std::to_string(ui->widget_camera->m_slide_info->pathological_order) + ".json");
#endif //FTP_SEND
            return;
        }

        });
    node_check.detach();


}

QString MicroSlideWindow::get_doctor_diagnosis_result()
{
    QVector<QString> vec;
    if(ui->rBtn_C->isChecked()){
        vec.push_back("C");
        ui->rBtn_C->setChecked(false);
    }
    if(ui->rBtn_SSA->isChecked()){
        vec.push_back("SSA");
        ui->rBtn_SSA->setChecked(false);
    }
    if(ui->rBtn_TA->isChecked()){
        vec.push_back("TA");
        ui->rBtn_TA->setChecked(false);
    }
    if(ui->rBtn_HP->isChecked()){
        vec.push_back("HP");
        ui->rBtn_HP->setChecked(false);
    }
    if(ui->rBtn_IP->isChecked()){
        vec.push_back("IP");
        ui->rBtn_IP->setChecked(false);
    }
    if(ui->rBtn_NM->isChecked()){
        vec.push_back("NM");
        ui->rBtn_NM->setChecked(false);
    }
    if(ui->rBtn_TSA->isChecked()){
        vec.push_back("TSA");
        ui->rBtn_TSA->setChecked(false);
    }
    if(ui->rBtn_HGD->isChecked()){
        vec.push_back("HGD");
        ui->rBtn_HGD->setChecked(false);
    }
    if(ui->rBtn_other->isChecked()){
        vec.push_back(ui->line_other->text()); 
        ui->rBtn_other->setChecked(false);
    }
    if (ui->rBtn_other->isChecked()) {
        vec.push_back(ui->line_other->text());
        ui->rBtn_other->setChecked(false);
    }
    if (ui->rBtn_other->isChecked()) {
        vec.push_back(ui->line_other->text());
        ui->rBtn_other->setChecked(false);
    }
    // 补充
    // 胃
    if (ui->rBtn_stomach_1->isChecked()) {
        vec.push_back(ui->rBtn_stomach_1->text());
        ui->rBtn_stomach_1->setChecked(false);
    }
    if (ui->rBtn_stomach_2->isChecked()) {
        vec.push_back(ui->rBtn_stomach_2->text());
        ui->rBtn_stomach_2->setChecked(false);
    }
    if (ui->rBtn_stomach_3->isChecked()) {
        vec.push_back(ui->rBtn_stomach_3->text());
        ui->rBtn_stomach_3->setChecked(false);
    }
    if (ui->rBtn_stomach_4->isChecked()) {
        vec.push_back(ui->rBtn_stomach_4->text());
        ui->rBtn_stomach_4->setChecked(false);
    }
    if (ui->rBtn_stomach_5->isChecked()) {
        vec.push_back(ui->rBtn_stomach_5->text());
        ui->rBtn_stomach_5->setChecked(false);
    }
    if (ui->rBtn_stomach_6->isChecked()) {
        vec.push_back(ui->rBtn_stomach_6->text());
        ui->rBtn_stomach_6->setChecked(false);
    }
    if (ui->rBtn_stomach_7->isChecked()) {
        vec.push_back(ui->rBtn_stomach_7->text());
        ui->rBtn_stomach_7->setChecked(false);
    }
    if (ui->rBtn_stomach_8->isChecked()) {
        vec.push_back(ui->rBtn_stomach_8->text());
        ui->rBtn_stomach_8->setChecked(false);
    }
    if (ui->rBtn_stomach_9->isChecked()) {
        vec.push_back(ui->rBtn_stomach_9->text());
        ui->rBtn_stomach_9->setChecked(false);
    }
    // 前列腺
    if (ui->rBtn_prostate_1->isChecked()) {
        vec.push_back(ui->rBtn_prostate_1->text());
        ui->rBtn_prostate_1->setChecked(false);
    }
    if (ui->rBtn_prostate_2->isChecked()) {
        vec.push_back(ui->rBtn_prostate_2->text());
        ui->rBtn_prostate_2->setChecked(false);
    }
    if (ui->rBtn_prostate_3->isChecked()) {
        vec.push_back(ui->rBtn_prostate_3->text());
        ui->rBtn_prostate_3->setChecked(false);
    }
    if (ui->rBtn_prostate_4->isChecked()) {
        vec.push_back(ui->rBtn_prostate_4->text());
        ui->rBtn_prostate_4->setChecked(false);
    }
    if (ui->rBtn_prostate_5->isChecked()) {
        vec.push_back(ui->rBtn_prostate_5->text());
        ui->rBtn_prostate_5->setChecked(false);
    }
    if (ui->rBtn_prostate_6->isChecked()) {
        vec.push_back(ui->rBtn_prostate_6->text());
        ui->rBtn_prostate_6->setChecked(false);
    }
    if (ui->rBtn_prostate_7->isChecked()) {
        vec.push_back(ui->rBtn_prostate_7->text());
        ui->rBtn_prostate_7->setChecked(false);
    }
    if (ui->rBtn_prostate_8->isChecked()) {
        vec.push_back(ui->rBtn_prostate_8->text());
        ui->rBtn_prostate_8->setChecked(false);
    }
    if (ui->rBtn_prostate_9->isChecked()) {
        vec.push_back(ui->rBtn_prostate_9->text());
        ui->rBtn_prostate_9->setChecked(false);
    }
    if (ui->rBtn_prostate_10->isChecked()) {
        vec.push_back(ui->rBtn_prostate_10->text());
        ui->rBtn_prostate_10->setChecked(false);
    }
    if (ui->rBtn_prostate_11->isChecked()) {
        vec.push_back(ui->rBtn_prostate_11->text());
        ui->rBtn_prostate_11->setChecked(false);
    }
    if (ui->rBtn_prostate_12->isChecked()) {
        vec.push_back(ui->rBtn_prostate_12->text());
        ui->rBtn_prostate_12->setChecked(false);
    }
    if (ui->rBtn_prostate_13->isChecked()) {
        vec.push_back(ui->rBtn_prostate_13->text());
        ui->rBtn_prostate_13->setChecked(false);
    }
    // 肺
    if (ui->rBtn_lung_1->isChecked()) {
        vec.push_back(ui->rBtn_lung_1->text());
        ui->rBtn_lung_1->setChecked(false);
    }
    if (ui->rBtn_lung_2->isChecked()) {
        vec.push_back(ui->rBtn_lung_2->text());
        ui->rBtn_lung_2->setChecked(false);
    }
    if (ui->rBtn_lung_3->isChecked()) {
        vec.push_back(ui->rBtn_lung_3->text());
        ui->rBtn_lung_3->setChecked(false);
    }
    if (ui->rBtn_lung_4->isChecked()) {
        vec.push_back(ui->rBtn_lung_4->text());
        ui->rBtn_lung_4->setChecked(false);
    }
    if (ui->rBtn_lung_5->isChecked()) {
        vec.push_back(ui->rBtn_lung_5->text());
        ui->rBtn_lung_5->setChecked(false);
    }
    if (ui->rBtn_lung_6->isChecked()) {
        vec.push_back(ui->rBtn_lung_6->text());
        ui->rBtn_lung_6->setChecked(false);
    }
    // 食管
    if (ui->rBtn_esophagus_1->isChecked()) {
        vec.push_back(ui->rBtn_esophagus_1->text());
        ui->rBtn_esophagus_1->setChecked(false);
    }
    if (ui->rBtn_esophagus_2->isChecked()) {
        vec.push_back(ui->rBtn_esophagus_2->text());
        ui->rBtn_esophagus_2->setChecked(false);
    }
    if (ui->rBtn_esophagus_3->isChecked()) {
        vec.push_back(ui->rBtn_esophagus_3->text());
        ui->rBtn_esophagus_3->setChecked(false);
    }
    if (ui->rBtn_esophagus_4->isChecked()) {
        vec.push_back(ui->rBtn_esophagus_4->text());
        ui->rBtn_esophagus_4->setChecked(false);
    }
    // 肝
    if (ui->rBtn_liver_1->isChecked()) {
        vec.push_back(ui->rBtn_liver_1->text());
        ui->rBtn_liver_1->setChecked(false);
    }
    if (ui->rBtn_liver_2->isChecked()) {
        vec.push_back(ui->rBtn_liver_2->text());
        ui->rBtn_liver_2->setChecked(false);
    }
    if (ui->rBtn_liver_3->isChecked()) {
        vec.push_back(ui->rBtn_liver_3->text());
        ui->rBtn_liver_3->setChecked(false);
    }
    if (ui->rBtn_liver_4->isChecked()) {
        vec.push_back(ui->rBtn_liver_4->text());
        ui->rBtn_liver_4->setChecked(false);
    }
    QString res = vec.join("_");

    return res;
}


void MicroSlideWindow::on_act_sysConfig_triggered()
{
    if (sysCfgForm == nullptr) {
        sysCfgForm = new SysConfigForm();
        sysCfgForm->setAttribute(Qt::WA_DeleteOnClose);
        connect(sysCfgForm, &SysConfigForm::destroyed, [this] {this->sysCfgForm = nullptr; });
        connect(sysCfgForm, &SysConfigForm::signal_syscfgChanged, this, &MicroSlideWindow::slot_on_syscfgChanged);
        sysCfgForm->show();
        std::cout << "new sysCfgForm" << std::endl;
    }
    else {
        sysCfgForm->show();
        std::cout << "sysCfgForm exists, just show" << std::endl;
    }
}


void MicroSlideWindow::slot_on_syscfgChanged(int _sharp, int _similarity, int _area, QString _filepath, std::string db_addr, int db_port)
{
    ui->widget_camera->m_sharp = _sharp;
    ui->widget_camera->m_similarity = _similarity;
    ui->widget_camera->m_save_root = _filepath;
    ui->widget_camera->m_area = _area;
    ui->widget_camera->m_db_addr = db_addr;
    ui->widget_camera->m_db_port = db_port;
}


void MicroSlideWindow::slot_on_ai_inference_finished(QString diagnose)
{
    ui->lineEdit_id->clear();
    ui->lineEdit_id->setEnabled(true);
    ui->btn_confirm_doc->setEnabled(true);
    ui->btn_id->setEnabled(true);

    // 点击确诊按钮后，将病理号输入框设置为focus，方便扫描枪继续扫描病理号
    ui->lineEdit_id->setFocus();

#ifndef FTP_SEND
    // 根据诊断结果得到新的文件夹名
    QStringList parts = ui->widget_camera->m_save_dir.split("/");          //分离保存路径
    QString folderName = parts[parts.length() - 1];
    QStringList parts2 = folderName.split("_");         //分离图片上级文件夹名
    QString AI_diagnose = ui->label_video_res->text();

    if (AI_diagnose != ".........")
    {
        if (AI_diagnose.length() <= 40) {
            QStringList parts3 = AI_diagnose.split(",");        //分离AI诊断结果
            parts2[1] = parts3[0];
            parts2[2] = parts3[1].split(".").join("");
        }
        else {
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


#ifndef OPEN_DATA_BASE_STORE
    
    // 重命名文件夹
    // 若没有修改则直接返回true，否则返回修改结果（true表示修改成功，false表示修改失败）
    QDir dir;
    bool success;
    if (ui->widget_camera->m_save_dir == new_save_dir) {
        success = true;
    }
    else {
        success = dir.rename(ui->widget_camera->m_save_dir, new_save_dir);
    }



    if (success) {
        ui->widget_camera->m_save_dir = new_save_dir;
        QMessageBox* box = new QMessageBox(QMessageBox::Information, tr("提示"), tr("已完成诊断!"));
        box->setAttribute(Qt::WA_DeleteOnClose);

        QTimer::singleShot(1000, box, SLOT(close()));
        box->show();
    }
    else {
        QMessageBox::warning(this, "警告", "诊断失败：未能重命名文件夹！\n 请重试!");
    }
#endif // !OPEN_DATA_BASE_STORE

#else
    QMessageBox* box = new QMessageBox(QMessageBox::Information, tr("提示"), tr("已完成诊断!"));
    box->setAttribute(Qt::WA_DeleteOnClose);

    QTimer::singleShot(1000, box, SLOT(close()));
    box->show();

    //box->exec();

#endif //FTP_SEND

    ui->line_other->clear();

    emit doctor_diagnosis_finished();
}


void MicroSlideWindow::slot_on_update_view_port()
{
    // 提交更改
    model->submitAll();
    // 刷新视图
    ui->tableView_db->viewport()->update();
}


void MicroSlideWindow::on_combo_diagnose_currentTextChanged(const QString &arg1)
{
    int patient_id = ui->spinBox_id->text().toInt();
    QString dignosis_res = arg1;
    bool ret = updateDiagnosisResult(patient_id,dignosis_res,db);
    if(ret == true){
        slot_on_update_view_port();
        //QMessageBox::information(nullptr, "Success", "Diagnosis result updated successfully.");
    }
}


void MicroSlideWindow::on_btn_search_clicked()
{
    int patient_id = ui->lineEdit_patien_id->text().toInt();
    // 设置查询，要先查询，再添加
    QString query = "SELECT * FROM diagnosis_results"; // 根据你的实际表名和字段名调整
    model2->setQuery(query, db);
    
    // 设置查询过滤器
    QString filter = QString("patient_id = %1").arg(patient_id);
    model2->setFilter(filter);

    // 提交更改
    if (model2->submitAll()) {
        // 提交成功
        // 刷新视图
        ui->tableView_db_2->viewport()->update();
        qDebug() << "on_btn_search success.";
    } 

}


void MicroSlideWindow::on_btn_all_info_clicked()
{
    //取消过滤器
    model2->setFilter(""); // 传递空字符串取消过滤器
    // 提交更改
    model2->submitAll();
    // 刷新视图
    ui->tableView_db_2->viewport()->update();
}

void MicroSlideWindow::slot_on_tableView_db_2_double_clicked(const QModelIndex &index)
{
    // 从模型中获取数据
    QString data = ui->tableView_db_2->model()->data(index).toString();
    //如果双击图像，在下面的lable显示
    if(index.column() == 4){
        ui->widget_res_img->show();
        QImage img = QImage(data);
        //添加倍率和感兴趣区域
        //获取放大倍数
        QModelIndex mag_index = model2->index(index.row(),3, QModelIndex());
        QModelIndex res_index = model2->index(index.row(), 5, QModelIndex());
        QString mag = "×" + ui->tableView_db_2->model()->data(mag_index).toString()
            + "--" + ui->tableView_db_2->model()->data(res_index).toString();
        QPainter painter(&img);
        QFont font = painter.font();
        font.setPointSize(60);
        painter.setFont(font);
        painter.drawText(60, 100, mag);
        painter.setPen(QPen(Qt::red, 8));
        //获取感兴趣区域
        QModelIndex x_index = model2->index(index.row(), 7, QModelIndex());
        QModelIndex y_index = model2->index(index.row(), 8, QModelIndex());
        int coord_x = ui->tableView_db_2->model()->data(x_index).toInt();
        int coord_y = ui->tableView_db_2->model()->data(y_index).toInt();
        QRect rect(coord_x, coord_y, 454, 454);
        QRect imageRect(0, 0, img.width(), img.height());
        QRect intersectedRect = rect.intersected(imageRect);
        painter.drawRect(intersectedRect);
        painter.end();

        double ratio = img.height()*1.0 / img.width();
        m_count_res_img = (m_count_res_img+1)%3;
        switch(m_count_res_img){
            case 1:{
                ui->label_res_img_1->setAlignment(Qt::AlignmentFlag::AlignCenter);
                ui->label_res_img_1->setFixedWidth(300);
                ui->label_res_img_1->setFixedHeight(300*ratio);
                //调整图片大小和格式
                QPixmap pix = QPixmap::fromImage(img); //将QImage对象转换为QPixmap对象
                ui->label_res_img_1->setPixmap(pix.scaledToWidth(300)); //将 QPixmap 对象 pix 按宽度缩放到 PIC_WIDTH，并设置为 QLabel 的图片
                ui->label_res_img_1->setScaledContents(true); //设置 QLabel 的内容（图片）在必要时可以缩放以适应标签的大小
                break;
            }
            case 2:{
                ui->label_res_img_2->setAlignment(Qt::AlignmentFlag::AlignCenter);
                ui->label_res_img_2->setFixedWidth(300);
                ui->label_res_img_2->setFixedHeight(300*ratio);
                //调整图片大小和格式
                QPixmap pix = QPixmap::fromImage(img); //将QImage对象转换为QPixmap对象
                ui->label_res_img_2->setPixmap(pix.scaledToWidth(300)); //将 QPixmap 对象 pix 按宽度缩放到 PIC_WIDTH，并设置为 QLabel 的图片
                ui->label_res_img_2->setScaledContents(true); //设置 QLabel 的内容（图片）在必要时可以缩放以适应标签的大小
                break;
            }
            case 0:{
                ui->label_res_img_3->setAlignment(Qt::AlignmentFlag::AlignCenter);
                ui->label_res_img_3->setFixedWidth(300);
                ui->label_res_img_3->setFixedHeight(300*ratio);
                //调整图片大小和格式
                QPixmap pix = QPixmap::fromImage(img); //将QImage对象转换为QPixmap对象
                ui->label_res_img_3->setPixmap(pix.scaledToWidth(300)); //将 QPixmap 对象 pix 按宽度缩放到 PIC_WIDTH，并设置为 QLabel 的图片
                ui->label_res_img_3->setScaledContents(true); //设置 QLabel 的内容（图片）在必要时可以缩放以适应标签的大小
                break;
            }
            default:
                break;
        }
    }
}

void MicroSlideWindow::slot_on_tableView_data_changed(const QModelIndex &index)
{
    //第一行是字段，不更改
    if(index.row() == 0){
        qDebug()<<"it is head,cannnot change.";
        return;
    }
    // 从模型中获取数据,并写入数据库
    QString data = ui->tableView_db_2->model()->data(index).toString();
    QModelIndex index2 = model2->index(0, index.column(), QModelIndex());
    QString sql_head = ui->tableView_db_2->model()->data(index2).toString();
}


void MicroSlideWindow::on_btn_close_res_img_clicked()
{
    ui->widget_res_img->hide();
}

void MicroSlideWindow::slot_on_add_data_to_diagnosis_results(int patient_id, QString slice_id, int mag, 
    QString img_path, QString diag_res, float confidence, int coord_x, int coord_y)
{
    QSqlQuery query(db);
    //query.prepare("UPDATE patients SET diagnosis_result = :result WHERE patient_id = :id"); 
    query.prepare("INSERT INTO diagnosis_results "
        "(patient_id, slice_id, magnification,"
        "image_path,diagnosis_result,confidence_percentage,"
        "relative_x_coordinate,relative_y_coordinate) "
        "VALUES (:_p_id, :_s_id, :_mag, :_img_p, :_d_res,"
        ":_confi, :_coor_x, :_coor_y);");
    query.bindValue(":_p_id", patient_id);
    query.bindValue(":_s_id", slice_id);
    query.bindValue(":_mag", mag);
    query.bindValue(":_img_p", img_path);  //这里有bug:医生确认病症后会更改文件夹名，这里也要同步更改
    query.bindValue(":_d_res", diag_res);  
    query.bindValue(":_confi", confidence);
    query.bindValue(":_coor_x", coord_x);
    query.bindValue(":_coor_y", coord_y);
    if (!query.exec()) {
        qDebug() <<"[erro]:add_data_to_diagnosis_results:" <<query.lastError();
    }
    else {
        // 提交更改
        model2->submitAll();
        ui->tableView_db_2->viewport()->update();
    }
}

void MicroSlideWindow::on_scanner_error_occured(const QString& error)
{
    QMessageBox::warning(this, "警告", error);
}

void MicroSlideWindow::try_initialize_scanner()
{
    initialize_scanner();
}

void MicroSlideWindow::check_scanner_usb_status()
{
    if (m_scanner->m_hDevice != NULL) {
        char* scanner_status = nl_GetLastError();
        if (strcmp(scanner_status, "Read from usb error, com push out") == 0) {
            release_scanner();
            QMessageBox::warning(this, "提示", tr("扫描仪设备已退出"));
        }
    }
}


void MicroSlideWindow::on_action_filetest_triggered(){
    if (ui->widget_camera->m_connect == false) {
        QMessageBox::warning(this, "测试错误", "未连接FPGA");
        return;
    }
    if (ft == nullptr) {
        ft = new FileTest();
        ft->setAttribute(Qt::WA_DeleteOnClose);
        connect(ft, &FileTest::destroyed, [this] {ft = nullptr; });
        connect(ft, &FileTest::signal_send_img, this->ui->widget_camera, &bgCamera::slot_on_handle_file_test);
        ft->show();
    }
    else {
        ft->show();
    }
}

