#include "mainwidget.h"
#include <QDebug>
#include <QMenuBar>
#include <QOpenGLWidget>
#include <QToolBar>
#include <QtConcurrent/QtConcurrent>
#include "aboutdialog.h"
#include "codemap.h"
#include "expgaindlg.h"
#include "qxhelp.h"
#include "triggerdlg.h"
#include "ui_mainwidget.h"

MainWidget::MainWidget(QWidget *parent)
    : FramelessWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);
    abandoned();
    initCfg();
    initObj();
    initGUI();
    initCNT();
    initDef();
}

MainWidget::~MainWidget()
{
    m_bThread = false;
    if (m_camRunning) {
        if (m_cam) {
            m_cam->CloseDevice();
        }
    }
    saveSysCfg();
}

void MainWidget::initObj()
{
    qRegisterMetaType<StCmdRet>("StCmdRet");
    connect(this, &MainWidget::sndPixmap, this, &MainWidget::rcvPixmap);

    std::thread tWork([this]() { workThread(); });
    tWork.detach();

    m_optCmd = new OperateCmd(this);
    connect(m_optCmd, &OperateCmd::sndOptLog, this, &MainWidget::rcvOptLog);
    connect(m_optCmd, &OperateCmd::sndCmdRet, this, &MainWidget::rcvCmdRet);

    m_pImgAttr = new ImgAttrDlg(this);
    connect(this, &MainWidget::sndSetROI, m_pImgAttr, &ImgAttrDlg::rcvSetROI);
    connect(this, &MainWidget::sndUpdateUI, m_pImgAttr, &ImgAttrDlg::rcvUpdateUi);
    connect(this, &MainWidget::sndImgAttrCmdRet, m_pImgAttr, &ImgAttrDlg::rcvCmdRet);
    connect(m_pImgAttr, &ImgAttrDlg::sndData, this, &MainWidget::rcvRoiAndFps);

    m_pExpAndGain = new ExpGainDlg(this);
    connect(this, &MainWidget::sndUpdateUI, m_pExpAndGain, &ExpGainDlg::rcvUpdateUi);
    connect(this, &MainWidget::sndExpAndGainCmdRet, m_pExpAndGain, &ExpGainDlg::rcvCmdRet);
    connect(this, &MainWidget::sndExpGainValue, m_pExpAndGain, &ExpGainDlg::rcvValue);
    connect(this, &MainWidget::sndExpGainMode, m_pExpAndGain, &ExpGainDlg::rcvModel);
    connect(m_pExpAndGain, &ExpGainDlg::sndOneAEAG, this, &MainWidget::rcvOneAEAG);
    connect(m_pExpAndGain, &ExpGainDlg::sndCmd, [this](EuCMD cmd, uint32_t data, bool bShow) {
        writeCmd(cmd, data, bShow);
        readCmd(cmd, m_pExpAndGain);
    });

    m_pTrigger = new TriggerDlg(this);
    connect(this, &MainWidget::sndUpdateUI, m_pTrigger, &TriggerDlg::rcvUpdateUi);
    connect(this, &MainWidget::sndRetTrg, m_pTrigger, &TriggerDlg::rcvRet);
    connect(this, &MainWidget::sndTriggerCmdRet, m_pTrigger, &TriggerDlg::rcvCmdRet);
    connect(m_pTrigger, &TriggerDlg::sndCmd, this, [this](EuCMD cmd, uint32_t data, bool bShow) {
        writeCmd(cmd, data, bShow);
        if (cmd != trgone) {
            readCmd(cmd, m_pTrigger);
        }
    });

    m_pRegister = new RegisterDlg(this);
    m_pRegister->setCfg(m_mapNameRegister, m_mapAddrRegister);
    connect(this, &MainWidget::sndUpdateUI, m_pRegister, &RegisterDlg::rcvUpdateUi);
    connect(this, &MainWidget::sndRegisterCmdRet, m_pRegister, &RegisterDlg::rcvCmdRet);
    connect(m_pRegister, &RegisterDlg::sndReg, this, &MainWidget::rcvReg);
    connect(m_pRegister, &RegisterDlg::sndCmd, this, &MainWidget::writeCmd);
    connect(m_pRegister, &RegisterDlg::sndImport, this, &MainWidget::rcvImportReg);
    connect(m_pRegister, &RegisterDlg::sndExport, this, &MainWidget::rcvExportReg);

    m_pCurAttr = new CurAttrDlg(this);
    connect(this, &MainWidget::sndUpdateUI, m_pCurAttr, &CurAttrDlg::rcvUpdateUi);
    connect(this, &MainWidget::sndCurAttrCmdRet, m_pCurAttr, &CurAttrDlg::rcvCmdRet);

    m_cam = new Camera(this);
}

void MainWidget::initCfg()
{
    qRegisterMetaType<StImgInfo>("StImgInfo");

    m_strCfgPath = QApplication::applicationDirPath() + "/config/";
    {
        m_mapCmdReg[aaroi_x] = "0x0C2C";
        m_mapCmdReg[aaroi_y] = "0x0C30";
        m_mapCmdReg[aaroi_w] = "0x0C34";
        m_mapCmdReg[aaroi_h] = "0x0C38";
    }
    {
        m_mapCmdReg[model] = "0x0004";
        m_mapCmdReg[serialno] = "0x000C";
        m_mapCmdReg[version] = "0x0010";
        m_mapCmdReg[roi_x] = "0x0820";
        m_mapCmdReg[roi_y] = "0x0824";
        m_mapCmdReg[roi_w] = "0x0818";
        m_mapCmdReg[roi_h] = "0x081C";
        m_mapCmdReg[fps] = "0x0814";
        m_mapCmdReg[pixelformat] = "0x0804";
        m_mapCmdReg[factoryparam] = "0x0014";
        m_mapCmdReg[paramsave] = "0x0018";
    }
    {
        m_mapCmdReg[expmode] = "0x0C04";
        m_mapCmdReg[aatarget] = "0x0C08";
        m_mapCmdReg[metime] = "0x0C10";
        m_mapCmdReg[aemaxtime] = "0x0C14";
        m_mapCmdReg[exptime] = "0x0C18";
        m_mapCmdReg[gainmode] = "0x0C1C";
        m_mapCmdReg[mgain] = "0x0C20";
        m_mapCmdReg[agmaxgain] = "0x0C24";
        m_mapCmdReg[curgain] = "0x0C28";
        m_mapCmdReg[exptime_min] = "0x0C90";
        m_mapCmdReg[exptime_max] = "0x0C8C";
    }
    {
        m_mapCmdReg[trgmodecap] = "0x0030";
        m_mapCmdReg[imgacq] = "0x0400";
        m_mapCmdReg[trgmode] = "0x0404";
        m_mapCmdReg[trgsrc] = "0x0408";
        m_mapCmdReg[trgnum] = "0x040C";
        m_mapCmdReg[trginterval] = "0x0410";
        m_mapCmdReg[trgone] = "0x0414";
        m_mapCmdReg[trgexp_delay] = "0x1010";
        m_mapCmdReg[trgdelay] = "0x1000";
        m_mapCmdReg[trgfilter_time] = "0x1004";
    }

    {
        mapPlatform["rockchip"] = Rockchip;
        mapPlatform["raspberrypi"] = RaspberryPi;
        mapPlatform["raspberrypi5"] = RaspberryPi5;
        mapPlatform["jetson"] = Jetson;

        mapProbefile["rockchip"] = "probe_camera_info-rk.sh";
        mapProbefile["raspberrypi"] = "probe_camera_info-rpi.sh";
        mapProbefile["raspberrypi5"] = "probe_camera_info-rpi5.sh";
        mapProbefile["jetson"] = "probe_camera_info-jetson.sh";
    }

    readPlatform();

    readRegCfg();

    readSysCfg();
}

void MainWidget::initGUI()
{
    // setAttribute(Qt::WA_TranslucentBackground);
    // setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);

    setWindowIcon(QIcon(":/image/logo.png"));
    setWindowTitle("veye_viewer");
    QPixmap pixmap(":/image/logo.png");
    ui->labelIcon->setPixmap(pixmap.scaled(ui->labelIcon->size(), Qt::KeepAspectRatio));
    ui->labelTitle->setText("veye_viewer v1.0");
    m_bkImage.load(":/image/bk.png");

    this->setWindowTitle(ui->labelTitle->text());
    this->setTitleBar(ui->widgetTitle);

    m_timer = new QTimer(this);
    m_scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(m_scene);
    m_pixItem = new QGraphicsPixmapItem();
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_scene->addItem(m_pixItem);

    ui->widgetTitle->setFixedHeight(42);

    ui->pushButtonHelp->setIcon(QIcon(":/image/help.png"));
    ui->pushButtonHelp->setFixedSize(32, 32);
    ui->pushButtonHelp->setToolTip(tr("帮助"));

    ui->pushButtonClose->setFixedWidth(42);
    ui->pushButtonMax->setFixedWidth(42);
    ui->pushButtonMin->setFixedWidth(42);
    ui->labelIcon->setFixedSize(68, 32);
    ui->labelIcon->setScaledContents(true);

    ui->pushButtonMin->setIcon(QIcon(":/image/min.png"));
    ui->pushButtonClose->setIcon(QIcon(":/image/close.png"));
    ui->pushButtonMax->setIcon(QIcon(":/image/max.png"));

    // QStringList items;
    // items << "Rockchip" << "RaspberryPi" << "RaspberryPi5" << "Jetson";
    // ui->comboBoxPlatform->addItems(items);
    ui->comboBoxPlatform->addItem("Rockchip", Rockchip);
    ui->comboBoxPlatform->addItem("RaspberryPi", RaspberryPi);
    ui->comboBoxPlatform->addItem("RaspberryPi5", RaspberryPi5);
    ui->comboBoxPlatform->addItem("Jetson", Jetson);

    ui->lineEditPswd->setEchoMode(QLineEdit::Password);
    ui->lineEditSavePath->setFixedWidth(256);
    //ui->spinBox_showFPS->setRange(15, 120);
    ui->comboBox_showFPS->addItem(tr("30帧/秒"), 30);
    ui->comboBox_showFPS->addItem(tr("60帧/秒"), 60);
    ui->comboBox_showFPS->setFixedWidth(82);

    ui->pushButtonCapture->setCheckable(true);
    ui->pushButtonCapture->setChecked(false);
    ui->pushButtonShow->setCheckable(true);
    ui->pushButtonShow->setChecked(false);
    ui->pushButtonShow->setEnabled(false);
    ui->radioButton_adaptive->setChecked(true);
    ui->pushButtonSaveImg->setEnabled(false);

    int btnWidth = 110;
    ui->pushButtonCapture->setFixedWidth(btnWidth);
    ui->pushButtonShow->setFixedWidth(btnWidth);
    ui->pushButtonSaveImg->setFixedWidth(btnWidth);
    ui->pushButtonSavePath->setFixedWidth(btnWidth);

    ui->tableWidgetLog->horizontalHeader()->setHighlightSections(false);
    ui->tableWidgetLog->verticalHeader()->setVisible(false);
    ui->tableWidgetLog->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidgetLog->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tableWidgetLog->setAlternatingRowColors(true);
    ui->tableWidgetLog->setShowGrid(false);
    ui->tableWidgetLog->setFocusPolicy(Qt::NoFocus);
    ui->tableWidgetLog->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetLog->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidgetLog->setWordWrap(true);
    //ui->tableWidgetLog->setTextElideMode(Qt::ElideRight);
    ui->tableWidgetLog->setSortingEnabled(false);
    ui->tableWidgetLog->setMouseTracking(true);
    ui->tableWidgetLog->verticalHeader()->setVisible(false);
    ui->tableWidgetLog->resizeRowsToContents();
    QStringList headers;
    headers << tr("序号") << tr("结果") << tr("操作记录");
    ui->tableWidgetLog->setColumnCount(headers.size());
    ui->tableWidgetLog->setHorizontalHeaderLabels(headers);
    ui->tableWidgetLog->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableWidgetLog->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->tableWidgetLog->setColumnWidth(0, 50);
    ui->tableWidgetLog->setColumnWidth(1, 50);

    ui->spinBoxSaveCount->setMinimum(1);
    ui->spinBoxSaveCount->setMaximum(INT32_MAX);

    ui->lineEditPswd->setPlaceholderText(tr("请输入主板密码"));

    ui->line_0->setFrameShadow(QFrame::Sunken);
    ui->line_1->setFrameShadow(QFrame::Sunken);
    ui->line_2->setFrameShadow(QFrame::Sunken);
    ui->line_3->setFrameShadow(QFrame::Sunken);
    ui->line_4->setFrameShadow(QFrame::Sunken);
    ui->line_5->setFrameShadow(QFrame::Sunken);
    ui->line_6->setFrameShadow(QFrame::Sunken);
    ui->line_7->setFrameShadow(QFrame::Sunken);
    ui->line_8->setFrameShadow(QFrame::Sunken);
    ui->line_9->setFrameShadow(QFrame::Sunken);
    ui->line_8->hide();

    ui->tabWidget->setContentsMargins(0, 0, 0, 0);

    {
        QLayout *lay = new QVBoxLayout();
        lay->addWidget(m_pImgAttr);
        lay->setContentsMargins(10, 0, 10, 6);
        ui->tabWidget->widget(0)->setLayout(lay);
    }
    {
        QLayout *lay = new QVBoxLayout();
        lay->addWidget(m_pExpAndGain);
        lay->setContentsMargins(10, 0, 10, 6);
        ui->tabWidget->widget(1)->setLayout(lay);
    }
    {
        QLayout *lay = new QVBoxLayout();
        lay->addWidget(m_pTrigger);
        lay->setContentsMargins(10, 0, 10, 6);
        ui->tabWidget->widget(2)->setLayout(lay);
    }
    {
        QLayout *lay = new QVBoxLayout();
        lay->addWidget(m_pRegister);
        lay->setContentsMargins(10, 0, 10, 6);
        ui->tabWidget->widget(3)->setLayout(lay);
    }
    {
        QLayout *lay = new QVBoxLayout();
        lay->addWidget(m_pCurAttr);
        lay->setContentsMargins(10, 0, 10, 6);
        ui->tabWidget->widget(4)->setLayout(lay);
    }
    initBar();

    ui->widgetLeft->setFixedWidth(350);
    ui->widgetRight->setFixedWidth(420);
    ui->widgetBottom->setFixedHeight(200);
    resize(1800, 1000);

    QxHelp::moveCenter(this);
}

void MainWidget::initBar()
{
    m_menuHelp = new QMenu(this);
    //新建action
    act1 = new QAction(QIcon(":/image/cn.png"), tr("简体中文"), this);
    act2 = new QAction(QIcon(":/image/us.png"), "English", this);
    act3 = new QAction(QIcon(":/image/about.png"), tr("关于"), this);

    //把action追加到menu
    m_menuHelp->addAction(act1);
    m_menuHelp->addAction(act2);
    m_menuHelp->addAction(act3);
    connect(act1, &QAction::triggered, this, [this]() {
        bool bSuccess = m_translator.load(m_strCfgPath + "veye_viewer_zh_CN.qm");
        if (bSuccess) {
            qApp->installTranslator(&m_translator);
            ui->retranslateUi(this);
            ui->lineEditPswd->setPlaceholderText(tr("请输入主板密码"));
            emit sndUpdateUI();
            emit ui->pushButtonGetCamParam->clicked();
            QStringList headers;
            headers << tr("序号") << tr("结果") << tr("操作记录");
            ui->tableWidgetLog->setColumnCount(headers.size());
            ui->tableWidgetLog->setHorizontalHeaderLabels(headers);
            act3->setText(tr("关于"));
            if (m_camRunning) {
                ui->pushButtonCapture->setText(("停止采集"));
            }
            if (m_camImgShow) {
                ui->pushButtonShow->setText(("停止显示"));
            }
            int index = ui->comboBox_showFPS->currentIndex();
            ui->comboBox_showFPS->clear();
            ui->comboBox_showFPS->addItem(tr("30帧/秒"), 30);
            ui->comboBox_showFPS->addItem(tr("60帧/秒"), 60);
            ui->comboBox_showFPS->setCurrentIndex(index);
            m_language = 0;
        }
    });

    connect(act2, &QAction::triggered, this, [this]() {
        bool bSuccess = m_translator.load(m_strCfgPath + "veye_viewer_en_US.qm");
        if (bSuccess) {
            qApp->installTranslator(&m_translator);
            ui->retranslateUi(this);
            ui->lineEditPswd->setPlaceholderText(tr("Enter motherboard pwd"));
            emit sndUpdateUI();
            emit ui->pushButtonGetCamParam->clicked();
            QStringList headers;
            headers << tr("Num") << tr("Result") << tr("Operation Log");
            ui->tableWidgetLog->setColumnCount(headers.size());
            ui->tableWidgetLog->setHorizontalHeaderLabels(headers);
            act3->setText(tr("About"));
            if (m_camRunning) {
                ui->pushButtonCapture->setText(("Stop Capture"));
            }
            if (m_camImgShow) {
                ui->pushButtonShow->setText(("Stop Show"));
            }
            int index = ui->comboBox_showFPS->currentIndex();
            ui->comboBox_showFPS->clear();
            ui->comboBox_showFPS->addItem(tr("30 fps"), 30);
            ui->comboBox_showFPS->addItem(tr("60 fps"), 60);
            ui->comboBox_showFPS->setCurrentIndex(index);
            m_language = 1;
        }
    });

    connect(act3, &QAction::triggered, this, [this]() {
        AboutDialog *dlg = new AboutDialog(this);
        QxHelp::moveParentCenter(this, dlg);
        dlg->show();
    });

    ui->pushButtonHelp->setMenu(m_menuHelp);
    ui->pushButtonHelp->setStyleSheet("QPushButton::menu-indicator {image: none;}");
}

void MainWidget::initCNT()
{
    connect(this, &FramelessWidget::titleDblClick, this, &MainWidget::titleDblClick);

    connect(ui->pushButtonClose, &QPushButton::clicked, this, [this]() {
        if (QxHelp::showMsgBoxQuesion(tr("提示"), tr("确定退出?"))) {
            saveSysCfg();
            close();
        }
    });
    connect(ui->pushButtonMin, &QPushButton::clicked, this, [this]() { showMinimized(); });
    connect(ui->pushButtonMax, &QPushButton::clicked, this, [this]() {
        if (isMaximized()) {
            showNormal();
            ui->pushButtonMax->setIcon(QIcon(":/image/max.png"));
        } else {
            showMaximized();
            ui->pushButtonMax->setIcon(QIcon(":/image/normal.png"));
        }
    });

    connect(ui->comboBoxPlatform, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        QString strPlatform = ui->comboBoxPlatform->currentText();
        qDebug() << "set platform:" << strPlatform;

        m_platform = (EuPlatform) index;
    });

    connect(ui->comboBoxDev, QOverload<int>::of(&QComboBox::activated), this, [this]() {
        if (m_camRunning) {
            emit ui->pushButtonCapture->clicked(false);
            ui->pushButtonCapture->setChecked(false);
            ui->pushButtonShow->setChecked(false);
        }

        setData(StDevInfo());
        QString strValue = ui->comboBoxDev->currentText();
        if (m_mapAutoDvInfo.contains(strValue)) {
            StDevInfo devInfo = m_mapAutoDvInfo[strValue];
            setData(devInfo);
            setStus(false);
            ui->checkBoxManual->setChecked(false);
        }
        readSysfs();
        QString strI2C = ui->lineEditI2cBus->text();
        if (QxHelp::hasWritePermission(strI2C.toStdString().c_str())) {
            emit ui->pushButtonGetCamParam->clicked(true);
        }
    });

    connect(ui->pushButtonAutoAdd, &QPushButton::clicked, this, [this]() {
        qDebug() << "............pushButtonAutoAdd....";
        ui->comboBoxDev->clear();
        m_mapAutoDvInfo.clear();
        readCamera();
        // for (const auto &it : qAsConst(m_mapManualDvInfo)) {
        //     addDev(it);
        // }
        if (m_mapAutoDvInfo.size() > 0) {
            emit ui->comboBoxDev->activated(0);
        }
    });
    connect(ui->lineEditPswd, &QLineEdit::editingFinished, this, [this]() {
        emit ui->pushButtonAuthorize->clicked();
    });

    connect(ui->pushButtonAuthorize, &QPushButton::clicked, this, [this]() {
        m_strPwd = ui->lineEditPswd->text();
        for (const auto &it : std::as_const(m_mapAutoDvInfo)) {
            QString strI2cBus = it.i2c_bus;
            qDebug() << strI2cBus;
            QString strCmd = "echo " + m_strPwd + " | sudo -S chmod 666 " + strI2cBus;
            int nRet = system(strCmd.toStdString().c_str());
            rcvOptLog(nRet, strCmd);
            if (0 == nRet) {               
                if (ui->lineEditI2cBus->text() == strI2cBus) {
                    emit ui->pushButtonGetCamParam->clicked();
                }
                m_bAuthorize = true;
            } else {
                QxToast::showTip(tr("密码错误"));
            }
        }
    });

    connect(ui->pushButtonOpenTest, &QPushButton::clicked, this, [this]() {
        {
            QString deviceName = ui->lineEditVideoNode->text();
            int fd = open(deviceName.toStdString().c_str(), O_RDWR);
            if (-1 != fd) {
                qDebug() << "open " << deviceName << "OK";
                int nBufType = 0;
                struct v4l2_capability cap;
                int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
                if (ret < 0) {
                    perror("获取功能失败");
                } else {
                    qDebug() << "...capabilities..ok..." << ret;
                    if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
                        nBufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        qDebug() << "...V4L2_BUF_TYPE_VIDEO_CAPTURE.." << ret;
                    } else if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) {
                        nBufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
                        qDebug() << "...V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE.." << ret;
                    } else {
                        qDebug() << "capabilities........no...........CAPTURE";
                    }
                }

                //close(fd);
            }
        }
        // QString strVaule = "";
        // int nBufType = 0;
        // //open video
        // strVaule = ui->lineEditVideoNode->text();
        // if (!strVaule.isEmpty()) {
        //     m_optCmd->openDev(strVaule, 1, nBufType);
        // } else {
        //     QxToast::showTip(tr("video is empty"));
        // }
        // //open i2c
        // strVaule = ui->lineEditI2cBus->text();
        // if (!strVaule.isEmpty()) {
        //     m_optCmd->openDev(strVaule, 0, nBufType);
        // } else {
        //     QxToast::showTip("i2cbus is empty");
        // }
        // //open media
        // strVaule = ui->lineEditMediaNode->text();
        // if (!strVaule.isEmpty()) {
        //     m_optCmd->openDev(strVaule, 1, nBufType);
        // } else {
        //     QxToast::showTip("media is empty");
        // }
        // //open subNode
        // strVaule = ui->lineEditSubNode->text();
        // if (!strVaule.isEmpty()) {
        //     m_optCmd->openDev(strVaule, 3, nBufType);
        // } else {
        //     QxToast::showTip("subNode is empty");
        // }
    });

    connect(ui->pushButtonGetCamParam, &QPushButton::clicked, this, [this]() {
        readSysfs();
        getBaseInfo();
        int index = ui->tabWidget->currentIndex();
        switch (index) {
        case 0:
            getImgAttr();
            break;
        case 1:
            getExpAndGain();
            break;
        case 2:
            getTrigger();
            break;
        case 3:
            break;
        case 4:
            getCurAttr();
            break;
        default:
            break;
        };
    });

    connect(ui->pushButtonCapture, &QPushButton::clicked, this, [this](bool checked) {
        if (checked) {
            openCam();
        } else {
            closeCam();
        }
    });
    connect(ui->pushButtonShow, &QPushButton::clicked, this, [this](bool checked) {
        if (checked) {
            openShow();
        } else {
            closeShow();
        }
    });

    connect(ui->radioButton_adaptive, &QRadioButton::clicked, this, [this]() {
        m_scale = true;
        ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    });
    connect(ui->radioButton_src, &QRadioButton::clicked, this, [this]() {
        m_scale = false;
        ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    });

    connect(ui->pushButtonSavePath, &QPushButton::clicked, this, [this]() {
        QString savePath = QFileDialog::getExistingDirectory(this,
                                                             tr("路径"),
                                                             QApplication::applicationDirPath(),
                                                             QFileDialog::ShowDirsOnly);
        if (QxHelp::hasWritePermission(savePath.toStdString().c_str())) {
            ui->lineEditSavePath->setText(savePath);
        } else {
            QxToast::showTip(tr("当前路径无写权限"), this);
        }
    });

    // connect(ui->spinBox_showFPS,
    //         QOverload<int>::of(&QSpinBox::valueChanged),
    //         this,
    //         [this](int value) { emit sndShowFps(value); });

    connect(ui->comboBox_showFPS,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
                int value = ui->comboBox_showFPS->currentData().toInt();
                emit sndShowFps(value);
            });

    connect(ui->pushButtonSaveImg, &QPushButton::clicked, this, [this]() {
        ui->pushButtonSaveImg->setEnabled(false);
        QString strPath = !ui->lineEditSavePath->text().isEmpty()
                              ? ui->lineEditSavePath->text()
                              : QApplication::applicationDirPath();
        QxHelp::makeFolder(strPath);
        QxHelp::makeFolderTail(strPath);
        int count = ui->spinBoxSaveCount->value();
        emit sndSaveImg(strPath, count);
    });

    connect(ui->pushButtonCleanOptLog, &QPushButton::clicked, this, [this]() {
        ui->tableWidgetLog->clearContents();
        m_optNum = 0;
    });

    connect(ui->pushButtonExportOptLog, &QPushButton::clicked, this, [this]() {
        int romCount = ui->tableWidgetLog->rowCount(); //获取总行数
        if (romCount > 0) {
            auto savePath = QFileDialog::getSaveFileName(this,
                                                         tr("导出"),
                                                         QApplication::applicationDirPath());
            if (!savePath.isEmpty()) {
                QString fileName;
                int pos = savePath.lastIndexOf(".txt");
                qDebug() << "..." << pos << ".." << savePath.length() - strlen(".txt");
                if (pos != (savePath.length() - strlen(".txt"))) {
                    fileName = savePath + ".txt";
                } else {
                    fileName = savePath;
                }
                qDebug() << fileName;
                QFile file(fileName);
                if (file.open(QFile::WriteOnly | QFile::Text)) //只写的方式打开
                {
                    QTextStream out(&file);          
                    for (int i = romCount - 1; i >= 0; i--) //行
                    {
                        QString rowstring;
                        int colCount = ui->tableWidgetLog->columnCount();
                        for (int j = 0; j < colCount; j++) //列
                        {                                  //遍历表格中的字符串
                            rowstring += ui->tableWidgetLog->item(i, j)->text();
                            rowstring += "\t";
                        }
                        rowstring += "\n"; //下一行空格
                        out << rowstring;  //把每行数据传到文件对象
                    }
                    file.close();
                    QxToast::showTip(tr("导出完成"), this);
                }
            }
        } else {
            QxToast::showTip(tr("操作记录为空"), this);
        }
    });

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        switch (index) {
        case 0:
            if (!m_bFirst) {
                getImgAttr();
            }
            m_bFirst = false;
            break;
        case 1:
            getExpAndGain();
            break;
        case 2:
            getTrigger();
            break;
        case 3:
            break;
        case 4:
            getCurAttr();
            break;
        default:
            break;
        }
    });
}

void MainWidget::initDef()
{
    // QxHelp::detectPlatform();
    // QxHelp::getModel();

    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        //qreal refreshRate = screen->refreshRate();
        //ui->spinBox_showFPS->setValue(qRound(refreshRate));
        int nFps = ui->comboBox_showFPS->currentData().toInt();
        emit sndShowFps(nFps);
    }

    // //readSysfs();
    ui->spinBoxSaveCount->setValue(1);
    ui->lineEditSavePath->setText(QApplication::applicationDirPath() + "/image");

    ui->comboBoxPlatform->setCurrentIndex(m_platform);

    readCamera();

    emit ui->comboBoxDev->activated(0);

    if (0 == m_language) {
        emit act1->triggered();
    } else {
        emit act2->triggered();
    }

    QTimer::singleShot(300, this, [this]() {
        int nWidth = ui->lineEditVideoNode->width();
        ui->comboBoxDev->setFixedWidth(nWidth);
        ui->comboBoxPlatform->setFixedWidth(nWidth);
        ui->lineEditPswd->setFixedWidth(nWidth);
        ui->lineEditPswd->setFocus();      
        showBGImg();
    });

    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        QString i2c = ui->lineEditI2cBus->text();
        if (!i2c.isEmpty()) {
            readCmd(exptime, m_pExpAndGain, false);
            readCmd(curgain, m_pExpAndGain, false);
            getCurAttr();
        }
    });
    m_timer->start(1000);
}

void MainWidget::initCur()
{
    getBaseInfo();
    int index = ui->tabWidget->currentIndex();
    emit ui->tabWidget->setCurrentIndex(index);
}

void MainWidget::abandoned()
{
    ui->pushButtonOpenTest->hide();
    ui->labeltrgone->hide();
    ui->label_trgone->hide();
    ui->pushButtonManualAdd->hide();
    ui->pushButtonManualDel->hide();
    ui->checkBoxManual->hide();
    ui->pushButtonManualAdd->setEnabled(false);
    ui->pushButtonManualDel->setEnabled(false);
    ui->checkBoxManual->setChecked(false);
    ui->pushButtonAuthorize->hide();
    ui->lineEditPswd->hide();
    ui->line_1->hide();
#if 0
    connect(ui->checkBoxManual, &QCheckBox::clicked, this, [this](bool checked) {
        ui->pushButtonManualAdd->setEnabled(checked);
        ui->pushButtonManualDel->setEnabled(checked);
        ui->pushButtonAutoAdd->setEnabled(!checked);
        if (checked) {
            setStus();
            setData(StDevInfo());
            //ui->comboBoxDev->setCurrentIndex(-1);
        } else {
            //emit ui->comboBoxDev->textActivated(ui->comboBoxDev->currentText());
        }
    });
    
connect(ui->pushButtonManualAdd, &QPushButton::clicked, this, [this]() {
        if (noEmpty()) {
            StDevInfo devInfo;
            devInfo.b_manual = true;
            devInfo.i2c_bus = ui->lineEditI2cBus->text();
            devInfo.media_entity_name = ui->lineEditEntityName->text();
            devInfo.media_node = ui->lineEditMediaNode->text();
            devInfo.video_node = ui->lineEditVideoNode->text();
            devInfo.video_subnode = ui->lineEditSubNode->text();
            if (!m_mapManualDvInfo.contains(devInfo.video_node)) {
                addDev(devInfo);
                QString strJson = m_strCfgPath + "manual_camera_index.json";
                saveDevInfo(strJson, true);
            } else {
                QxToast::showTip(devInfo.video_node + " is exist!");
            }
        }
    });

    connect(ui->pushButtonManualDel, &QPushButton::clicked, this, [this]() {
        QString strValue = ui->comboBoxDev->currentText();

        if (m_mapManualDvInfo.contains(strValue)) {
            m_mapManualDvInfo.remove(strValue);
            QString strJson = m_strCfgPath + "manual_camera_index.json";
            saveDevInfo(strJson, true);
        }
        ui->comboBoxDev->removeItem(ui->comboBoxDev->currentIndex());
        ui->comboBoxDev->setCurrentIndex(0);
        emit ui->comboBoxDev->textActivated(ui->comboBoxDev->currentText());
    });

#endif
}

void MainWidget::resizeEvent(QResizeEvent *event)
{
    if (!m_camRunning) {
        showBGImg();
    }
    if (m_gvSize != ui->widget_Img->size()) {
        m_gvSize = ui->widget_Img->size();
    }
}

void MainWidget::titleDblClick()
{
    emit ui->pushButtonMax->clicked();
}

bool MainWidget::readRegCfg()
{
    QString strJson = m_strCfgPath + "register.json";
    bool bRet = false;
    {
        Json::Value jsonRoot;
        if (QxHelp::readJson(strJson, jsonRoot)) {
            m_mapNameRegister.clear();
            m_mapAddrRegister.clear();
            int total = jsonRoot.size();
            for (int i = 0; i < total; ++i) {
                StRegInfo reg;
                Json::Value jsonNode = jsonRoot[i];
                reg.address = jsonNode["address"].asString().c_str();
                reg.rd_wt = jsonNode["rd_wt"].asString().c_str();
                reg.rd_wt = reg.rd_wt.toUpper();
                reg.name = jsonNode["name"].asString().c_str();
                reg.raw_support = jsonNode["raw_support"].asString().c_str();
                reg.raw_support = reg.raw_support.toUpper();
                reg.data_type = jsonNode["data_type"].asString().c_str();
                reg.data_type = reg.data_type.toLower();
                reg.update_timing = jsonNode["update_timing"].asString().c_str();
                reg.update_timing = reg.update_timing.toUpper();
                reg.description = jsonNode["description"].asString().c_str();
                m_mapNameRegister.insert(reg.name, reg);
                m_mapAddrRegister.insert(reg.address.toInt(nullptr, 16), reg);
                bRet = true;
            }
        } else {
            qWarning() << "readRegCfg " << strJson << " failed!!!";
        }
    }
    return bRet;
}

void MainWidget::readSysCfg()
{
    QString strJson = m_strCfgPath + "syscfg.json";
    if (QxHelp::readJson(strJson, m_jsonSysCfg)) {
        //emit ui->comboBoxDev->textActivated(m_jsonSysCfg["cur_dev"].asString().c_str());
        ui->lineEditSavePath->setText(m_jsonSysCfg["img_path"].asString().c_str());
        m_nChachFrameSize = m_jsonSysCfg["cache_frame_size"].asInt();
        m_nChachFrameSize = m_nChachFrameSize > 0 ? m_nChachFrameSize : 5;
        m_language = m_jsonSysCfg["language"].asInt();
        emit ui->comboBox_showFPS->currentIndexChanged(m_jsonSysCfg["show_fps"].asInt());
    } else {
        qDebug() << "readSysCfg " << strJson << " failed!!!";
    }
}

void MainWidget::saveSysCfg()
{
    QString strJson = m_strCfgPath + "syscfg.json";
    //m_jsonSysCfg["cur_dev"] = ui->comboBoxDev->currentText().toStdString();
    m_jsonSysCfg["img_path"] = ui->lineEditSavePath->text().toStdString();
    m_jsonSysCfg["cache_frame_size"] = m_nChachFrameSize;
    m_jsonSysCfg["language"] = m_language;
    m_jsonSysCfg["show_fps"] = ui->comboBox_showFPS->currentIndex();
    QxHelp::writeJson(strJson, m_jsonSysCfg);
}

void MainWidget::addDev(StDevInfo devInfo)
{
    ui->comboBoxDev->addItem(devInfo.video_node);
    m_mapAutoDvInfo.insert(devInfo.video_node, devInfo);
}

void MainWidget::rcvRoiAndFps(const StImgAttrInfo &info)
{
    if (m_camRunning) {
        QxToast::showTip(tr("状态为采集中，不支持该操作"), this);
    } else {
        QString strX = info.strX;
        QString strY = info.strY;
        QString strW = info.strW;
        QString strH = info.strH;
        QString strFps = info.strFps;

        QString videoNode = ui->lineEditVideoNode->text();
        QString i2cBus = ui->lineEditI2cBus->text();
        QString mediaNode = ui->lineEditMediaNode->text();
        QString subNode = ui->lineEditSubNode->text();
        QString entityName = ui->lineEditEntityName->text();

        int nPos = i2cBus.lastIndexOf("-") + 1;
        i2cBus = i2cBus.mid(nPos, i2cBus.length() - nPos);
        QString addr = m_i2cAddr.mid(2, m_i2cAddr.length() - 2);
        QString strCmd;
        switch (m_platform) {
        case Rockchip: {
            //v4l2
            strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_x=%2").arg(subNode, strX);
            m_optCmd->runSystemCmd(strCmd);
            strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_y=%2").arg(subNode, strY);
            m_optCmd->runSystemCmd(strCmd);
            //media
            strCmd = QString("\' \"%1\":0[fmt:%2/%3x%4@1/%5]\'")
                         .arg(entityName, "Y8_1X8", strW, strH, strFps);
            qDebug() << "mediaRunCmd param:" << strCmd;
            m_optCmd->mediaRunCmd(mediaNode, strCmd);
        } break;
        case RaspberryPi: {
            strCmd = QString("v4l2-ctl --set-ctrl roi_x=%1").arg(strX);
            m_optCmd->runSystemCmd(strCmd);
            strCmd = QString("v4l2-ctl --set-ctrl roi_y=%1").arg(strY);
            m_optCmd->runSystemCmd(strCmd);
            strCmd = QString("v4l2-ctl --set-fmt-video=width=%1,height=%2,pixelformat=GREY")
                         .arg(strW, strH);
            m_optCmd->runSystemCmd(strCmd);
            strCmd = QString("v4l2-ctl --set-ctrl frame_rate=%1").arg(strFps);
            m_optCmd->runSystemCmd(strCmd);
        } break;
        case RaspberryPi5: {            
            strCmd = m_strCfgPath + "config_camera-rpi5.sh"
                     + QString::asprintf(" mvcam -fmt RAW8 -x %d -y %d -w %d -h %d -bus %d -media "
                                         "%s -subdev %s -video %s",
                                         strX.toInt(),
                                         strY.toInt(),
                                         strW.toInt(),
                                         strH.toInt(),
                                         i2cBus.toInt(),
                                         mediaNode.toStdString().c_str(),
                                         subNode.toStdString().c_str(),
                                         videoNode.toStdString().c_str());
            m_optCmd->systemCmd(strCmd);
            strCmd = QString("v4l2-ctl -d %1 --set-ctrl frame_rate=%2").arg(subNode, strFps);
            m_optCmd->runSystemCmd(strCmd);
        } break;
        case Jetson: {
            strCmd = QString("v4l2-ctl --set-ctrl roi_x=%1").arg(strX);
            m_optCmd->runSystemCmd(strCmd);
            strCmd = QString("v4l2-ctl --set-ctrl roi_y=%1").arg(strY);
            m_optCmd->runSystemCmd(strCmd);
            strCmd = QString("v4l2-ctl --set-fmt-video=width=%1,height=%2,pixelformat=GREY")
                         .arg(strW, strH);
            m_optCmd->runSystemCmd(strCmd);
            strCmd = QString("v4l2-ctl --set-ctrl frame_rate=%1").arg(strFps);
            m_optCmd->runSystemCmd(strCmd);

            int width = strW.toInt();
            if (0 != width % 256) {
                width = (width / 256 + 1) * 256;
            }
            strCmd
                = QString("v4l2-ctl -d %1 --set-ctrl preferred_stride=%2").arg(videoNode).arg(width);
            m_optCmd->runSystemCmd(strCmd);

        } break;
        default:
            break;
        }

        getImgAttr();
    }
}

void MainWidget::openCam()
{
    QString videoNode = ui->lineEditVideoNode->text();
    QString subNode = ui->lineEditSubNode->text();

    switch (m_platform) {
    case Rockchip: {
    } break;
    case RaspberryPi: {
    } break;
    case RaspberryPi5: {
        emit sndSetROI();
    } break;
    case Jetson: {
        int width = m_roiW;
        if (0 != width % 256) {
            width = (width / 256 + 1) * 256;
        }
        QString strCmd
            = QString("v4l2-ctl -d %1 --set-ctrl preferred_stride=%2").arg(videoNode).arg(width);
        m_optCmd->runSystemCmd(strCmd);
    } break;
    default:
        break;
    }

    StCamParam camParam;
    camParam.videoNode = videoNode;
    camParam.subNode = subNode;
    camParam.x = m_roiX;
    camParam.y = m_roiY;
    camParam.w = m_roiW;
    camParam.h = m_roiH;
    camParam.fps = m_CamFps;
    m_camRunning = m_cam->OpenDevice(camParam);
    emit sndShowFps(ui->comboBox_showFPS->currentData().toInt());
    qDebug() << "open cam " << m_roiX << m_roiY << m_roiW << m_roiH << m_CamFps;
}

void MainWidget::closeCam()
{
    m_camRunning = false;
    m_cam->CloseDevice();
    ui->pushButtonCapture->setChecked(false);
    closeShow();
}

void MainWidget::openShow()
{
    m_showFps = 0;
    m_seconds = 0;
    m_bFps = false;
    m_camImgShow = true;
    emit sndShow(m_camImgShow);
    ui->pushButtonShow->setText(tr("停止显示"));
}

void MainWidget::closeShow()
{
    m_camImgShow = false;

    emit sndShow(m_camImgShow);
    ui->pushButtonShow->setText(tr("开始显示"));
    ui->pushButtonShow->setChecked(false);
    showBGImg();
    m_showFps = 0;
    m_seconds = 0;
    m_bFps = false;
}

void MainWidget::readSysfs()
{
    QString i2cBus = ui->lineEditI2cBus->text();
    if (!i2cBus.isEmpty()) {
        int nPos = i2cBus.lastIndexOf("-") + 1;
        i2cBus = i2cBus.mid(nPos, i2cBus.length() - nPos);
        QString strPath = "/sys/bus/i2c/devices/i2c-" + i2cBus + "/" + i2cBus + "-003b/veye_mvcam/";
        QString model = "";
        bool bRet = QxHelp::readFirstLine(strPath + "camera_model", model);
        QString strValue = "read " + strPath + "camera_model:" + model;
        rcvOptLog(bRet ? 0 : -1, strValue);
        ui->label_model->setText(model);
        QString strW = "";
        bRet = QxHelp::readFirstLine(strPath + "width", strW);
        strValue = "read " + strPath + "width:" + strW;
        rcvOptLog(bRet ? 0 : -1, strValue);
        QString strH = "";
        bRet = QxHelp::readFirstLine(strPath + "height", strH);
        strValue = "read " + strPath + "height:" + strH;
        rcvOptLog(bRet ? 0 : -1, strValue);
        QString strFps = "";
        bRet = QxHelp::readFirstLine(strPath + "fps", strFps);
        strValue = "read " + strPath + "fps:" + strFps;
        rcvOptLog(bRet ? 0 : -1, strValue);

        m_roiH = strH.toInt();
        m_roiW = strW.toInt();
        m_CamFps = strFps.toInt();
    }
}

void MainWidget::readPlatform()
{
    QString strJsonPlatform = m_strCfgPath + "platform.json";
    Json::Value jsonRoot;
    if (QxHelp::readJson(strJsonPlatform, jsonRoot)) {
        QString strPlatform = jsonRoot["platform"].asString().c_str();
        m_platform = mapPlatform[strPlatform.toLower()];
    } else {
        qWarning() << "readPlatform:" << strJsonPlatform << " failed!!!";
    }
}

void MainWidget::getBaseInfo()
{
    readCmd(version, this);
    readCmd(serialno, this);
}

void MainWidget::getImgAttr()
{
    readCmd(roi_x, m_pImgAttr);
    readCmd(roi_y, m_pImgAttr);
    readCmd(roi_w, m_pImgAttr);
    readCmd(roi_h, m_pImgAttr);
    readCmd(fps, m_pImgAttr);
}

void MainWidget::getExpAndGain()
{
    // QElapsedTimer etimer;
    // etimer.start();
    readCmd(model, m_pExpAndGain);
    readCmd(expmode, m_pExpAndGain);
    readCmd(gainmode, m_pExpAndGain);
    readCmd(exptime, m_pExpAndGain);
    readCmd(curgain, m_pExpAndGain);
    readCmd(aatarget, m_pExpAndGain);
    readCmd(aemaxtime, m_pExpAndGain);
    readCmd(agmaxgain, m_pExpAndGain);
    readCmd(metime, m_pExpAndGain);
    readCmd(mgain, m_pExpAndGain);
    readCmd(exptime_min, m_pExpAndGain);
    readCmd(exptime_max, m_pExpAndGain);
    //qDebug() << "getExpAndGain snd " << etimer.elapsed();
}

void MainWidget::getTrigger()
{
    // QElapsedTimer etimer;
    // etimer.start();
    readCmd(trgmodecap, m_pTrigger);
    readCmd(imgacq, m_pTrigger);
    readCmd(trgmode, m_pTrigger);
    readCmd(trgsrc, m_pTrigger);
    readCmd(trgexp_delay, m_pTrigger);
    readCmd(trgnum, m_pTrigger);
    readCmd(trginterval, m_pTrigger);
    readCmd(trgfilter_time, m_pTrigger);
    readCmd(trgdelay, m_pTrigger);
    readCmd(trgone, m_pTrigger);
    //qDebug() << "getTrigger snd " << etimer.elapsed();
}

void MainWidget::getCurAttr()
{
    readCmd(roi_w, m_pCurAttr, false);
    readCmd(roi_h, m_pCurAttr, false);
    readCmd(fps, m_pCurAttr, false);
    readCmd(pixelformat, m_pCurAttr, false);
    readCmd(trgmode, m_pCurAttr, false);
    readCmd(trgsrc, m_pCurAttr, false);
}

bool MainWidget::readRegData(
    EuCMD cmd, uint32_t reg, QObject *obj, bool showLog, bool async, uint32_t *data)
{
    bool bRet = false;
    QString i2c = ui->lineEditI2cBus->text();
    if (!i2c.isEmpty()) {
        if (async) {
            StI2CCmd stCmd;
            stCmd.bWrite = false;
            stCmd.cmd = cmd;
            stCmd.i2c = i2c;
            stCmd.addr = m_i2cAddr.toInt(nullptr, 16);
            stCmd.reg = reg;
            stCmd.data = 0;
            stCmd.obj = obj;
            stCmd.showlog = showLog;
            // QElapsedTimer timer;
            // timer.start();
            m_optCmd->asynReadData(stCmd);
            //qDebug() << "......asynReadData......." << timer.elapsed();
        } else {
            uint32_t addr = m_i2cAddr.toInt(nullptr, 16);
            m_optCmd->syncReadData(i2c, addr, reg, *data, showLog);
        }
        bRet = true;
    } else {
        QxToast::showTip(tr("i2c序号不能为空"), this);
    }
    return bRet;
}

bool MainWidget::writeRegData(EuCMD cmd, uint32_t reg, uint32_t data, bool showLog)
{
    bool bRet = false;
    if (m_mapAddrRegister.contains(reg)) {
        StRegInfo info = m_mapAddrRegister[reg];

        EuUpdateTiming type = (EuUpdateTiming) codemap::getUpdateTiming(info.update_timing);
        if (Standby == type && m_camRunning) {
            QxToast::showTip(tr("状态为采集中，不支持该操作"), this);
            return false;
        } else if (ReadOnly == type) {
            QxToast::showTip(tr("只读寄存器,不支持该操作"), this);
            return false;
        }
    }

    QString i2cBus = ui->lineEditI2cBus->text();
    if (!i2cBus.isEmpty()) {
        StI2CCmd stCmd;
        stCmd.bWrite = true;
        stCmd.cmd = cmd;
        stCmd.i2c = i2cBus;
        stCmd.addr = m_i2cAddr.toInt(nullptr, 16);
        stCmd.reg = reg;
        stCmd.data = data;
        stCmd.obj = nullptr;
        stCmd.showlog = showLog;
        m_optCmd->asynWriteDate(stCmd);
        bRet = true;
    } else {
        QxToast::showTip(tr("i2c序号不能为空"), this);
    }
    return bRet;
}

void MainWidget::cmdSleep(int n, bool bReal)
{
    if (bReal) {
        QThread::msleep(n);
    }
    rcvOptLog(0, QString("sleep %1 ms").arg(n));
}

void MainWidget::setData(StDevInfo devInfo)
{
    ui->lineEditEntityName->setText(devInfo.media_entity_name);
    ui->lineEditI2cBus->setText(devInfo.i2c_bus);
    ui->lineEditVideoNode->setText(devInfo.video_node);
    ui->lineEditSubNode->setText(devInfo.video_subnode);
    ui->lineEditMediaNode->setText(devInfo.media_node);

    if (!devInfo.i2c_bus.isEmpty()) {
        QString i2cBus = devInfo.i2c_bus;
        int nPos = i2cBus.lastIndexOf("-") + 1;
        i2cBus = i2cBus.mid(nPos, i2cBus.length() - nPos);
        QString strPath = "/sys/bus/i2c/devices/i2c-" + i2cBus + "/" + i2cBus + "-003b/veye_mvcam/";
        QString model = "";
        QxHelp::readFirstLine(strPath + "camera_model", model);
        ui->label_model->setText(model);
    }
}

void MainWidget::setStus(bool bEnable)
{
    ui->lineEditEntityName->setEnabled(bEnable);
    ui->lineEditI2cBus->setEnabled(bEnable);
    ui->lineEditVideoNode->setEnabled(bEnable);
    ui->lineEditSubNode->setEnabled(bEnable);
    ui->lineEditMediaNode->setEnabled(bEnable);
    ui->pushButtonManualAdd->setEnabled(bEnable);
    ui->pushButtonManualDel->setEnabled(bEnable);
}

bool MainWidget::noEmpty()
{
    bool bRet = true;
    QString strVaule = ui->lineEditI2cBus->text();
    if (strVaule.isEmpty()) {
        QxToast::showTip(tr("i2cbus 不能为空"));
        bRet = false;
        goto END;
    }
    //open media
    strVaule = ui->lineEditMediaNode->text();
    if (strVaule.isEmpty()) {
        QxToast::showTip(tr("media node 不能为空"));
        bRet = false;
        goto END;
    }
    //open video
    strVaule = ui->lineEditVideoNode->text();
    if (strVaule.isEmpty()) {
        QxToast::showTip(tr("video node 不能为空"));
        bRet = false;
        goto END;
    }
    //open subNode
    strVaule = ui->lineEditSubNode->text();
    if (strVaule.isEmpty()) {
        QxToast::showTip(tr("subNode 不能为空"));
        bRet = false;
        goto END;
    }
    return bRet;
END:
    return bRet;
}

void MainWidget::readCamera()
{
    QString strPlatform = ui->comboBoxPlatform->currentText();
    QString strCmd = m_strCfgPath + mapProbefile[strPlatform.toLower()];
    if (0 == system(strCmd.toStdString().c_str())) {
        qDebug() << "readCamera system cmd:" << strCmd;
        QString strJson = m_strCfgPath + "auto_camera_index.json";
        if (QxHelp::fileExists(strJson.toStdString().c_str())) {
            Json::Value jsonRoot;
            if (QxHelp::readJson(strJson, jsonRoot)) {
                int total = jsonRoot.size();
                for (int i = 0; i < total; ++i) {
                    Json::Value jsonNode = jsonRoot[i];
                    StDevInfo devInfo;
                    devInfo.i2c_bus = ("/dev/i2c-" + jsonNode["i2c_bus"].asString()).c_str();
                    devInfo.media_entity_name = jsonNode["media_entity_name"].asString().c_str();
                    devInfo.media_node = jsonNode["media_node"].asString().c_str();
                    devInfo.video_node = jsonNode["video_node"].asString().c_str();
                    devInfo.video_subnode = jsonNode["video_subnode"].asString().c_str();
                    addDev(devInfo);
                }
            } else {
                qDebug() << "auto_camera_index.json  readJons error";
            }
        } else {
            qDebug() << "auto_camera_index.json  does not exist ";
        }
    }
}

void MainWidget::showBGImg()
{
    m_gvSize = ui->graphicsView->size();
    QPixmap pix = QPixmap::fromImage(m_bkImage).scaled(m_gvSize / 4, Qt::KeepAspectRatio);
    m_pixItem->setPixmap(pix);
    m_pixItem->update();
    m_scene->setSceneRect(0, 0, pix.width(), pix.height());
}

void MainWidget::rcvCaptureImg(const StImgInfo &sii)
{
    // QElapsedTimer timer;
    // timer.start();
    if (m_camImgShow) {
        m_mtxPix.lock();
        int size = m_queueSii.size();
        if (size <= m_nChachFrameSize) {
            m_queueSii.push(sii);
        } else {
            if (new_buf == sii.strategy) {
                delete[] sii.buf;
            }
            qDebug() << "cache frame size " << size << " > " << m_nChachFrameSize << ", drop frame";
        }
        m_mtxPix.unlock();
    } else {
        if (new_buf == sii.strategy) {
            delete[] sii.buf;
        }
        m_seconds = 0;
        m_showFps = 0;
    }
    // qint64 elapsedTime = timer.elapsed(); // 获取经过的时间，单位为微秒
    // if (elapsedTime > TIEM_OUT) {
    //     qDebug() << "rcvCaptureImg:" << elapsedTime << "microseconds";
    // }
}

void MainWidget::pushData(const StImgInfo &sii)
{
    if (m_camImgShow) {
        m_mtxPix.lock();
        int size = m_queueSii.size();
        if (size <= m_nChachFrameSize) {
            m_queueSii.push(sii);
        } else {
            if (new_buf == sii.strategy) {
                delete[] sii.buf;
            }
            qDebug() << "cache frame size " << size << " > " << m_nChachFrameSize << ", drop frame";
        }
        m_mtxPix.unlock();
    } else {
        if (new_buf == sii.strategy) {
            delete[] sii.buf;
        }
        m_seconds = 0;
        m_showFps = 0;
    }
}

void MainWidget::workThread()
{
    QElapsedTimer timer;
    while (m_bThread) {
        timer.restart(); // 开始计时
        StImgInfo sii;
        QImage img;
        int count = 0;
        m_mtxPix.lock();
        count = m_queueSii.size();
        if (count > 0) {
            sii = m_queueSii.front();
            m_queueSii.pop();
        }
        m_mtxPix.unlock();

        if (count > 0) {
            if (m_camImgShow) {
                img = QImage(sii.buf, sii.width, sii.height, QImage::Format_Grayscale8);

                QPixmap pix;
                if (m_scale) {
                    pix = QPixmap::fromImage(img).scaled(m_gvSize,
                                                         Qt::KeepAspectRatio,
                                                         Qt::FastTransformation);
                } else {
                    pix = QPixmap::fromImage(img);
                }
                emit sndPixmap(pix, sii.strTS);
            }
            if (new_buf == sii.strategy) {
                delete[] sii.buf;
            }
        }
        qint64 elapsedTime = timer.elapsed(); // 获取经过的时间，单位为微秒
        if (elapsedTime > TIEM_OUT) {
            qDebug() << "work:" << elapsedTime << "microseconds";
        }
        QThread::msleep(1);
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void MainWidget::rcvPixmap(const QPixmap &pix, QString strTS)
{
    QElapsedTimer timer;
    if (m_camImgShow) {
        qint64 fpsTime = timerFps.elapsed();
        timestamps.push_back(fpsTime);
        if (timestamps.size() > 100) {
            timestamps.pop_front();
        }
        if (timestamps.size() >= 2) {
            double duration = timestamps.back() - timestamps.front();
            double avgFps = (timestamps.size() - 1) / duration;
            ui->label_showFps->setText(QString::number(avgFps * 1000, 'f', 2));
        }

        timer.start(); // 开始计时
        if (m_lastCmd == trgone) {
            m_trgoneCount++;
        } else {
            m_trgoneCount = 0;
        }
        ui->label_trgone->setText(QString::number(m_trgoneCount));

        if (0 == m_showFps) {
            timerFps.restart();
            m_bFps = true;
        }
        m_showFps++;
        ui->labelTimestamp->setText(strTS);
        m_pixItem->setPixmap(pix);
        m_scene->setSceneRect(0, 0, pix.width(), pix.height());
        qint64 elapsedTime = timer.elapsed(); // 获取经过的时间，单位为微秒
        if (elapsedTime > TIEM_OUT) {
            qDebug() << "show:" << elapsedTime << "microseconds";
        }
    } else {
        showBGImg();
    }
}

void MainWidget::rcvSaveFinish(bool bFalg)
{
    QxToast::showTip(tr("保存图像完成"), this);
    ui->pushButtonSaveImg->setEnabled(bFalg);
}

void MainWidget::rcvStatus(int status)
{
    if (status) {
        ui->pushButtonSaveImg->setEnabled(true);
        ui->pushButtonCapture->setText(tr("停止采集"));
        m_camRunning = true;
        ui->pushButtonShow->setEnabled(true);

    } else {
        showBGImg();
        ui->pushButtonSaveImg->setEnabled(false);
        ui->pushButtonCapture->setText(tr("开始采集"));
        ui->pushButtonShow->setText(tr("开始显示"));
        m_camRunning = false;
        ui->labelTimestamp->setText("0");
        ui->label_showFps->setText("0");
        rcvFrameInfo(StCamInfo());
        ui->pushButtonShow->setEnabled(false);
    }
    if (2 == status) {
        showBGImg();
        ui->labelTimestamp->setText("0");
        ui->label_showFps->setText("0");
    }
}

void MainWidget::rcvOptLog(int nErr, QString strMsg, bool bShow)
{
    if (bShow) {
        int col = 0;
        int row = 0;
        ui->tableWidgetLog->setUpdatesEnabled(false);
        ui->tableWidgetLog->setSortingEnabled(false);
        ui->tableWidgetLog->insertRow(row);
        QTableWidgetItem *itemNum = new QTableWidgetItem(QString::number(++m_optNum));
        QTableWidgetItem *itemErr = new QTableWidgetItem(QString::number(nErr));
        QTableWidgetItem *itemMsg = new QTableWidgetItem(strMsg);
        if (0 != nErr) {
            {
                itemErr->setForeground(QBrush(Qt::red));
                QFont font = itemErr->font();
                font.setBold(true);
                itemErr->setFont(font);
            }
            {
                itemMsg->setForeground(QBrush(Qt::red));
                QFont font = itemMsg->font();
                font.setBold(true);
                itemMsg->setFont(font);
            }
        }
        ui->tableWidgetLog->setItem(row, col, itemNum);
        ui->tableWidgetLog->item(row, col++)->setTextAlignment(Qt::AlignCenter);

        ui->tableWidgetLog->setItem(row, col, itemErr);
        ui->tableWidgetLog->item(row, col++)->setTextAlignment(Qt::AlignCenter);

        ui->tableWidgetLog->setItem(row, col, itemMsg);
        ui->tableWidgetLog->item(row, col)->setTextAlignment(Qt::AlignVCenter);
        ui->tableWidgetLog->resizeRowsToContents();

        row = ui->tableWidgetLog->rowCount();
        if (row > 100) {
            int rowToDelete = row - 1;
            for (int col = 0; col < ui->tableWidgetLog->columnCount(); ++col) {
                QTableWidgetItem *item = ui->tableWidgetLog->takeItem(rowToDelete, col);
                delete item;
            }
            ui->tableWidgetLog->removeRow(rowToDelete);
        }
        ui->tableWidgetLog->setSortingEnabled(false);
        ui->tableWidgetLog->setUpdatesEnabled(true);
    }
}

void MainWidget::rcvCmdRet(const StCmdRet &ret)
{
    QElapsedTimer timer;
    timer.start();
    if (m_pImgAttr == qobject_cast<ImgAttrDlg *>(ret.obj)) {
        emit sndImgAttrCmdRet(ret);
        uint32_t data = ret.data;
        switch (ret.cmd) {
        case roi_x:
            m_roiX = data;
            break;
        case roi_y:
            m_roiY = data;
            break;
        case roi_w:
            m_roiW = data;
            break;
        case roi_h:
            m_roiH = data;
            break;
        case fps:
            m_CamFps = data / 100.0;
            break;
        default:
            break;
        }
    } else if (m_pExpAndGain == qobject_cast<ExpGainDlg *>(ret.obj)) {
        emit sndExpAndGainCmdRet(ret);
    } else if (m_pTrigger == qobject_cast<TriggerDlg *>(ret.obj)) {
        emit sndTriggerCmdRet(ret);
    } else if (m_pRegister == qobject_cast<RegisterDlg *>(ret.obj)) {
        emit sndRegisterCmdRet(ret);
    } else if (m_pCurAttr == qobject_cast<CurAttrDlg *>(ret.obj)) {
        emit sndCurAttrCmdRet(ret);
    } else if (this == qobject_cast<MainWidget *>(ret.obj)) {
        uint32_t data = ret.data;
        switch (ret.cmd) {
        case version: {
            QString strControl = QString::asprintf("%02x.%02x",
                                                   data >> 24 & 0xFF,
                                                   data >> 16 & 0xFF);
            QString strLogic = QString::asprintf("%02x.%02x", data >> 8 & 0xFF, data & 0xFF);
            ui->label_control_version->setText(strControl);
            ui->label_logic_version->setText(strLogic);
        } break;
        case serialno:
            ui->label_serialno->setText(QString::number(data, 16).toUpper());
            break;
        default:
            break;
        }
    }
}

void MainWidget::writeCmd(EuCMD cmd, uint32_t data, bool bShow)
{
    if (cmd == trgone) {
        m_trgoneCount = 0;
    }
    if (factoryparam == cmd) {
        if (m_camRunning) {
            emit ui->pushButtonCapture->clicked(false);
        }
    }
    m_lastCmd = cmd;
    ui->label_trgone->setText("0");
    if (m_mapCmdReg.contains(cmd)) {
        QString strReg = m_mapCmdReg.value(cmd);
        uint32_t reg = strReg.midRef(2).toInt(nullptr, 16);
        writeRegData(cmd, reg, data, bShow);
    }
}

void MainWidget::readCmd(EuCMD cmd, QObject *obj, bool bShow, bool async, uint32_t *data)
{
    if (m_mapCmdReg.contains(cmd)) {
        QString strReg = m_mapCmdReg.value(cmd);
        uint32_t reg = strReg.midRef(2).toInt(nullptr, 16);
        readRegData(cmd, reg, obj, bShow, async, data);
    }
}

void MainWidget::rcvOneAEAG(int nAemaxtime, int nAgmaxgain, int nAatarget)
{
    //[step.1]:set.AE.AG.Max.value
    writeCmd(aatarget, nAatarget);
    writeCmd(aemaxtime, nAemaxtime);
    writeCmd(agmaxgain, nAgmaxgain);
    //2 [step.2]:.set·AE·AG.to.ONCE.moden
    writeCmd(expmode, 1);
    writeCmd(gainmode, 1);
    //[step.3]:.wait.5.seconds
    cmdSleep(5000, false);
    QTimer::singleShot(5000, this, [this] {
        //[step.4]:.check.oNcE.result
        uint32_t expValue = 0;
        uint32_t gainValue = 0;
        readCmd(exptime, m_pExpAndGain, true, false, &expValue);
        readCmd(curgain, m_pExpAndGain, true, false, &gainValue);
        emit sndExpGainValue(expValue, gainValue);
        //[step.5]:.set.to.manual.mode
        uint32_t expMode = 0;
        uint32_t gainMode = 0;
        writeCmd(expmode, 0);
        writeCmd(gainmode, 0);
        writeCmd(metime, expValue);
        writeCmd(mgain, gainValue);
        cmdSleep(500);
        //[step.6]:.check.resutlt
        expMode = 0;
        gainMode = 0;
        readCmd(expmode, m_pExpAndGain);
        readCmd(gainmode, m_pExpAndGain);
        //emit sndExpGainMode(expMode, gainMode);
        expValue = 0;
        gainValue = 0;
        readCmd(exptime, m_pExpAndGain);
        readCmd(curgain, m_pExpAndGain);
        //emit sndExpGainValue(expValue, gainValue);
        writeCmd(paramsave, 1);
        getExpAndGain();
    });
}

void MainWidget::rcvReg(const StRegInfo &info, bool bWrite, bool bShow)
{
    uint32_t data = 0;
    uint32_t reg = info.address.midRef(2).toInt(nullptr, 16);

    if (!m_mapAddrRegister.contains(reg) || info.address.isEmpty()) {
        QxToast::showTip(tr("请输入正确的寄存器地址"), this);
        return;
    }

    if (bWrite) {
        QString strData = info.data;
        if (strData.contains("0x") || strData.contains("0X")) {
            strData = strData.midRef(2).toString();
        }
        data = strData.toInt(nullptr, 16);
        writeRegData(def_cmd, reg, data, bShow);
        if (info.rd_wt != "WO") {
            readRegData(def_cmd, reg, m_pRegister);
            // QString strRet = (QString("0x%1").arg(data, 8, 16, QLatin1Char('0')));
            // emit sndRet2Reg(strRet);
        }
    } else {
        readRegData(def_cmd, reg, m_pRegister, bShow);
        // QString strRet = (QString("0x%1").arg(data, 8, 16, QLatin1Char('0')));
        // emit sndRet2Reg(strRet);
    }
}

void MainWidget::rcvExportReg()
{
    QString filename = QFileDialog::getSaveFileName(nullptr,
                                                    tr("导出"),
                                                    QDir::homePath(),
                                                    tr("文本文件 (*.json);;所有文件 (*)"));
    if (!filename.isEmpty()) {
        QString strPath = filename.mid(0, filename.lastIndexOf("/"));

        if (QxHelp::hasWritePermission(strPath.toStdString().c_str())) {
            if (!filename.endsWith(".json")) {
                filename += ".json";
            }

            Json::Value jsonRoot;
            //writeCmd(imgacq, 0);
            for (const auto &it : qAsConst(m_mapAddrRegister)) {
                if ("RW" == it.rd_wt || "RO" == it.rd_wt) {
                    Json::Value jsonNode;
                    QString strReg = it.address;
                    uint32_t reg = strReg.midRef(2).toInt(nullptr, 16);
                    uint32_t data = 0;
                    readRegData(def_cmd, reg, nullptr, true, false, &data);
                    jsonNode["name"] = it.name.toStdString();
                    jsonNode["address"] = it.address.toStdString();
                    jsonNode["data"] = data;
                    jsonNode["rw"] = it.rd_wt.toStdString();
                    jsonRoot.append(jsonNode);
                    //cmdSleep(10);
                }
            }
            if (QxHelp::writeJson(filename, jsonRoot)) {
                QxToast::showTip(tr("导出完成"), this);
            }
        } else {
            QxToast::showTip(tr("当前路径无写权限"), this);
        }
    }

    //writeCmd(imgacq, 1);
    return;
}

void MainWidget::rcvImportReg()
{
    if (m_camRunning) {
        if (QxHelp::showMsgBoxQuesion(tr("提示"),
                                      tr("状态为采集中，是否停止采集，继续导入数据？"))) {
            emit ui->pushButtonCapture->clicked(false);
        } else {
            return;
        }
    }
    auto filename = QFileDialog::getOpenFileName(this, tr("导入"), QDir::homePath(), "*.json");
    if (!filename.isEmpty() && filename.endsWith(".json") && filename != nullptr) {
        struct StROI
        {
            uint32_t data = 0;
            QString addr;
        };
        QVector<StROI> vtROI(4, StROI());
        QVector<StROI> vtAAROI(4, StROI());

        StROI stFps;

        Json::Value jsonRoot;
        if (QxHelp::readJson(filename, jsonRoot)) {
            int total = jsonRoot.size();
            for (int i = 0; i < total; ++i) {
                Json::Value jsonNode = jsonRoot[i];
                QString strRW = jsonNode["rw"].asString().c_str();
                if ("RW" == strRW) {
                    QString name = jsonNode["name"].asString().c_str();
                    QString addr = jsonNode["address"].asString().c_str();
                    uint32_t data = jsonNode["data"].asUInt();
                    if (addr == m_mapCmdReg.value(fps)) {
                        stFps.data = data;
                        stFps.addr = addr;
                    } else if (addr == m_mapCmdReg.value(roi_x)) {
                        vtROI[0].data = data;
                        vtROI[0].addr = addr;
                    } else if (addr == m_mapCmdReg.value(roi_y)) {
                        vtROI[1].data = data;
                        vtROI[1].addr = addr;
                    } else if (addr == m_mapCmdReg.value(roi_w)) {
                        vtROI[2].data = data;
                        vtROI[2].addr = addr;
                    } else if (addr == m_mapCmdReg.value(roi_h)) {
                        vtROI[3].data = data;
                        vtROI[3].addr = addr;
                    } else if (addr == m_mapCmdReg.value(aaroi_x)) {
                        vtAAROI[0].data = data;
                        vtAAROI[0].addr = addr;
                    } else if (addr == m_mapCmdReg.value(aaroi_y)) {
                        vtAAROI[1].data = data;
                        vtAAROI[1].addr = addr;
                    } else if (addr == m_mapCmdReg.value(aaroi_w)) {
                        vtAAROI[2].data = data;
                        vtAAROI[2].addr = addr;
                    } else if (addr == m_mapCmdReg.value(aaroi_h)) {
                        vtAAROI[3].data = data;
                        vtAAROI[3].addr = addr;
                    } else if (m_mapAddrRegister.contains(addr.toInt(nullptr, 16))) {
                        uint32_t reg = addr.midRef(2).toInt(nullptr, 16);
                        QString rd_wt = m_mapAddrRegister.value(reg).rd_wt.toUpper();
                        if ("RW" == rd_wt) {
                            writeRegData(def_cmd, reg, data);
                            //cmdSleep(10);
                        }
                    }
                }
            }
            for (int i = 0; i < 4; ++i) {
                uint32_t data = vtROI[i].data;
                QString addr = vtROI[i].addr;
                if (!addr.isEmpty()) {
                    uint32_t reg = addr.midRef(2).toInt(nullptr, 16);
                    writeRegData(def_cmd, reg, data);
                    //cmdSleep(10);
                }
            }
            for (int i = 0; i < 4; ++i) {
                uint32_t data = vtAAROI[i].data;
                QString addr = vtAAROI[i].addr;
                if (!addr.isEmpty()) {
                    uint32_t reg = addr.midRef(2).toInt(nullptr, 16);
                    writeRegData(def_cmd, reg, data);
                    //cmdSleep(10);
                }
            }
            if (!stFps.addr.isEmpty()) {
                uint32_t reg = stFps.addr.midRef(2).toInt(nullptr, 16);
                writeRegData(def_cmd, reg, stFps.data);
                //cmdSleep(10);
            }

            QxToast::showTip(tr("导入完成"), this);
        } else {
            qDebug() << filename << " parse json failed!!!";
        }
    }

    return;
}

void MainWidget::rcvFrameInfo(const StCamInfo &info)
{
    ui->labelCaptureFps->setText(QString::number(info.fps, 'f', 2));
    ui->labelRES->setText(QString("%1 * %2").arg(info.width).arg(info.height));
}
