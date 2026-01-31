#include "MainWidget.h"
#include <QDebug>
#include <QFormLayout>
#include <QMenuBar>
#include <QOpenGLWidget>
#include <QToolBar>
#include "AboutDialog.h"
#include "CodeMap.h"
#include "ExpGainDlg.h"
#include "QxHelp.h"
#include "TriggerDlg.h"
#include "Version.h"
#include "ui_MainWidget.h"
#include <arm_neon.h>

MainWidget::MainWidget(QWidget *parent)
    : FramelessWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);
    initCfg();
    initObj();
    setupUi();
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

void MainWidget::initCfg()
{
    m_strCfgPath = QApplication::applicationDirPath() + "/config/";
    {
        mapPlatform["rockchip"] = EuPlatform::Rockchip;
        mapPlatform["raspberrypi"] = EuPlatform::RaspberryPi;
        mapPlatform["raspberrypi5"] = EuPlatform::RaspberryPi5;
        mapPlatform["jetson"] = EuPlatform::Jetson;

        mapProbefile["rockchip"] = "probe_camera_info-rk.sh";
        mapProbefile["raspberrypi"] = "probe_camera_info-rpi.sh";
        mapProbefile["raspberrypi5"] = "probe_camera_info-rpi5.sh";
        mapProbefile["jetson"] = "probe_camera_info-jetson.sh";
    }
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
        m_mapCmdReg[fps_gx] = "0x0850"; //gx
        m_mapCmdReg[maxfps] = "0x0810"; //gx
        m_mapCmdReg[pixelformat] = "0x0804";
        m_mapCmdReg[factoryparam] = "0x0014";
        m_mapCmdReg[paramsave] = "0x0018";
    }
    { //gx
        m_mapCmdReg[readmode] = "0x0838";
        m_mapCmdReg[videomode] = "0x082c";
        m_mapCmdReg[videomodenum] = "0x0060";
        m_mapCmdReg[videomodewh1] = "0x0064";
        m_mapCmdReg[videomodeparam1] = "0x0068";
        m_mapCmdReg[videomodewh2] = "0x006C";
        m_mapCmdReg[videomodeparam2] = "0x0070";
        m_mapCmdReg[videomodewh3] = "0x0074";
        m_mapCmdReg[videomodeparam3] = "0x0078";
        m_mapCmdReg[videomodewh4] = "0x007C";
        m_mapCmdReg[videomodeparam4] = "0x0080";
        m_mapCmdReg[videomodewh5] = "0x0084";
        m_mapCmdReg[videomodeparam5] = "0x0088";
        m_mapCmdReg[videomodewh6] = "0x008C";
        m_mapCmdReg[videomodeparam6] = "0x0090";
        m_mapCmdReg[videomodewh7] = "0x0094";
        m_mapCmdReg[videomodeparam7] = "0x0098";
        m_mapCmdReg[videomodewh8] = "0x009C";
        m_mapCmdReg[videomodeparam8] = "0x00A0";
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
        //gx
        m_mapCmdReg[workmodecap] = "0x0030";
        m_mapCmdReg[workmode] = "0x0404";
        m_mapCmdReg[slavemode] = "0x0460";
        //mv
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
        m_mapCmdReg[wbmode] = "0x0C3C";
        m_mapCmdReg[awbcolortempmin] = "0x0C40";
        m_mapCmdReg[awbcolortempmax] = "0x0C44";
        m_mapCmdReg[mwbrgain] = "0x0C48";
        m_mapCmdReg[mwbbgain] = "0x0C4C";
        m_mapCmdReg[colortemp] = "0x0C50";
        m_mapCmdReg[currgain] = "0x0C54";
        m_mapCmdReg[curbgain] = "0x0C58";
    }
    {
        m_mapCmdReg[saturation] = "0x0CB0";
        m_mapCmdReg[contrast] = "0x0CB4";
        m_mapCmdReg[hue] = "0x0CB8";
    }

    readPlatform();

    readCamIndex();

    adjustAdaptive();

    readRegCfg();

    readSysCfg();
}

void MainWidget::initObj()
{
    //m_renderer = new UYVYRenderer(ui->widgetRender);
    // ui->widgetRender->setFixedSize(300, 400);
    // ui->graphicsView->hide();
    CommDevCfg devCfg = {m_euCamType, m_euPlatform};
    qRegisterMetaType<StCmdRet>("StCmdRet");
    connect(this, &MainWidget::sndPixmap, this, &MainWidget::rcvPixmap);

    std::thread tWork([this]() { workThread(); });
    tWork.detach();

    //m_optCmd = new OperateCmd(this);
    //connect(m_optCmd, &OperateCmd::sndOptLog, this, &MainWidget::rcvOptLog);
    //connect(m_optCmd, &OperateCmd::sndCmdRet, this, &MainWidget::rcvCmdRet);

    m_cam = new Camera(devCfg, this);
    connect(m_cam, &Camera::sndOptLog, this, &MainWidget::rcvOptLog);
    connect(m_cam, &Camera::sndCmdRet, this, &MainWidget::rcvCmdRet);

    m_pImgAttr = new ImgAttrDlg(devCfg, this);
    connect(this, &MainWidget::sndSetROI, m_pImgAttr, &ImgAttrDlg::rcvSetROI);
    connect(this, &MainWidget::sndUpdateUI, m_pImgAttr, &ImgAttrDlg::rcvUpdateUi);
    connect(this, &MainWidget::sndImgAttrCmdRet, m_pImgAttr, &ImgAttrDlg::rcvCmdRet);
    connect(m_pImgAttr, &ImgAttrDlg::setRoiAndFpsData, this, &MainWidget::onSetRoiAndFps);
    connect(m_pImgAttr, &ImgAttrDlg::sndCmd, this, [this](EuCMD cmd, uint32_t data, bool bShow) {
        asyncWriteCmd(cmd, data, bShow);
        readCmd(cmd, m_pImgAttr);
    });
    connect(m_pImgAttr, &ImgAttrDlg::readCmd, this, &MainWidget::asyncReadCmd);

    m_pExpAndGain = new ExpGainDlg(devCfg, this);
    connect(this, &MainWidget::sndUpdateUI, m_pExpAndGain, &ExpGainDlg::rcvUpdateUi);
    connect(this, &MainWidget::sndExpAndGainCmdRet, m_pExpAndGain, &ExpGainDlg::rcvCmdRet);
    connect(this, &MainWidget::sndExpGainValue, m_pExpAndGain, &ExpGainDlg::rcvValue);
    connect(this, &MainWidget::sndExpGainMode, m_pExpAndGain, &ExpGainDlg::rcvModel);
    connect(m_pExpAndGain, &ExpGainDlg::sndOneAEAG, this, &MainWidget::rcvOneAEAG);
    connect(m_pExpAndGain, &ExpGainDlg::sndCmd, this, [this](EuCMD cmd, uint32_t data, bool bShow) {
        asyncWriteCmd(cmd, data, bShow);
        readCmd(cmd, m_pExpAndGain);
    });

    m_pTrigger = new TriggerDlg(devCfg, this);
    connect(this, &MainWidget::sndUpdateUI, m_pTrigger, &TriggerDlg::rcvUpdateUi);    
    connect(this, &MainWidget::sndTriggerCmdRet, m_pTrigger, &TriggerDlg::rcvCmdRet);
    connect(m_pTrigger, &TriggerDlg::sndCmd, this, [this](EuCMD cmd, uint32_t data, bool bShow) {
        asyncWriteCmd(cmd, data, bShow);
        if (cmd != trgone) {
            readCmd(cmd, m_pTrigger);
        }
    });

    m_pRegister = new RegisterDlg(devCfg, this);
    m_pRegister->setCfg(m_mapNameRegister, m_mapAddrRegister);
    connect(this, &MainWidget::sndUpdateUI, m_pRegister, &RegisterDlg::rcvUpdateUi);
    connect(this, &MainWidget::sndRegisterCmdRet, m_pRegister, &RegisterDlg::rcvCmdRet);
    connect(m_pRegister, &RegisterDlg::sndReg, this, &MainWidget::rcvReg);
    connect(m_pRegister, &RegisterDlg::sndCmd, this, &MainWidget::asyncWriteCmd);
    connect(m_pRegister, &RegisterDlg::sndImport, this, &MainWidget::rcvImportReg);
    connect(m_pRegister, &RegisterDlg::sndExport, this, &MainWidget::rcvExportReg);

    m_pCurAttr = new CurAttrDlg(devCfg, this);
    connect(this, &MainWidget::sndUpdateUI, m_pCurAttr, &CurAttrDlg::rcvUpdateUi);
    connect(this, &MainWidget::sndCurAttrCmdRet, m_pCurAttr, &CurAttrDlg::rcvCmdRet);

    m_pWhiteBlance = new WhiteBlanceDlg(devCfg, this);
    connect(this, &MainWidget::sndUpdateUI, m_pWhiteBlance, &WhiteBlanceDlg::rcvUpdateUi);
    connect(m_pWhiteBlance,
            &WhiteBlanceDlg::sndCmd,
            this,
            [this](EuCMD cmd, uint32_t data, bool bShow) {
                asyncWriteCmd(cmd, data, bShow);
                readCmd(cmd, m_pWhiteBlance);
            });
    connect(this, &MainWidget::sndWBCmdRet, m_pWhiteBlance, &WhiteBlanceDlg::rcvCmdRet);

    m_pImgProc = new ImgProcDlg(devCfg, this);
    connect(this, &MainWidget::sndUpdateUI, m_pImgProc, &ImgProcDlg::rcvUpdateUi);
    connect(m_pImgProc, &ImgProcDlg::sndCmd, this, [this](EuCMD cmd, uint32_t data, bool bShow) {
        asyncWriteCmd(cmd, data, bShow);
        readCmd(cmd, m_pImgProc);
    });
    connect(this, &MainWidget::sndImgProcCmdRet, m_pImgProc, &ImgProcDlg::rcvCmdRet);
}

void MainWidget::setupUi()
{
    ui->graphicsView->installEventFilter(this);
    setWindowIcon(QIcon(":/image/logo.png"));
    ui->labelIcon->setPixmap(QPixmap(":/image/logo.png"));
    ui->labelTitle->setText(QString(APP_NAME) + " V" + APP_VERSION);
    ui->pushButtonTrans->setIcon(QIcon(":/image/en.png"));
    ui->pushButtonHelp->setIcon(QIcon(":/image/about.png"));
    ui->pushButtonMin->setIcon(QIcon(":/image/min.png"));
    ui->pushButtonClose->setIcon(QIcon(":/image/close.png"));
    ui->pushButtonMax->setIcon(QIcon(":/image/max.png"));

    m_bkImage.load(":/image/bk.png");

    setTitleBar(ui->widgetTitle);
    m_timer = new QTimer(this);
    m_scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(m_scene);
    m_pixItem = new QGraphicsPixmapItem();
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_scene->addItem(m_pixItem);

    ui->widgetTitle->setFixedHeight(42);

    ui->pushButtonTrans->setFixedSize(32, 32);
    ui->pushButtonTrans->setIconSize(QSize(28, 28));
    ui->pushButtonHelp->setFixedSize(32, 32);
    ui->pushButtonHelp->setIconSize(QSize(28, 28));
    ui->pushButtonHelp->setToolTip(tr("帮助"));

    ui->labelIcon->setFixedSize(68, 32);
    ui->labelIcon->setScaledContents(true);
    ui->pushButtonClose->setFixedWidth(42);
    ui->pushButtonMax->setFixedWidth(42);
    ui->pushButtonMin->setFixedWidth(42);

    ui->comboBoxPlatform->addItem("Rockchip", (int) EuPlatform::Rockchip);
    ui->comboBoxPlatform->addItem("RaspberryPi", (int) EuPlatform::RaspberryPi);
    ui->comboBoxPlatform->addItem("RaspberryPi5", (int) EuPlatform::RaspberryPi5);
    ui->comboBoxPlatform->addItem("Jetson", (int) EuPlatform::Jetson);

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

    ui->line_1->setFrameShadow(QFrame::Sunken);
    ui->line_2->setFrameShadow(QFrame::Sunken);
    ui->line_3->setFrameShadow(QFrame::Sunken);
    ui->line_4->setFrameShadow(QFrame::Sunken);
    ui->line_5->setFrameShadow(QFrame::Sunken);
    ui->line_6->setFrameShadow(QFrame::Sunken);
    ui->line_7->setFrameShadow(QFrame::Sunken);

    QLayout *lay = new QVBoxLayout();
    lay->setContentsMargins(10, 0, 10, 6);
    m_btnCtrls.push_back(ui->pushButtonImgAttr);
    m_winCtrls.push_back(m_pImgAttr);
    lay->addWidget(m_pImgAttr);

    m_btnCtrls.push_back(ui->pushButtonExpAndGain);
    m_winCtrls.push_back(m_pExpAndGain);
    lay->addWidget(m_pExpAndGain);

    m_btnCtrls.push_back(ui->pushButtonWhiteBlance);
    m_winCtrls.push_back(m_pWhiteBlance);
    lay->addWidget(m_pWhiteBlance);

    m_btnCtrls.push_back(ui->pushButtonImgProc);
    m_winCtrls.push_back(m_pImgProc);
    lay->addWidget(m_pImgProc);

    m_btnCtrls.push_back(ui->pushButtonTrigger);
    m_winCtrls.push_back(m_pTrigger);
    lay->addWidget(m_pTrigger);

    m_btnCtrls.push_back(ui->pushButtonRegister);
    m_winCtrls.push_back(m_pRegister);
    lay->addWidget(m_pRegister);

    m_btnCtrls.push_back(ui->pushButtonCurAttr);
    m_winCtrls.push_back(m_pCurAttr);
    lay->addWidget(m_pCurAttr);

    for (int i = 0; i < m_btnCtrls.size(); ++i) {
        m_btnCtrls[i]->setCheckable(true);
    }
    for (int i = 0; i < m_winCtrls.size(); ++i) {
        m_winCtrls[i]->hide();
    }
    m_btnCtrls[0]->setChecked(true);
    m_winCtrls[0]->show();

    ui->widgetFunction->setLayout(lay);

    if (EuCamType::MvCamera == m_euCamType) {
        ui->pushButtonImgProc->hide();
        ui->pushButtonWhiteBlance->hide();
    }

    ui->widgetLeft->setFixedWidth(350);
    ui->widgetRight->setFixedWidth(420);
    ui->widgetBottom->setFixedHeight(200);
    ui->spinBoxSaveCount->setRange(1, 9999);
    ui->lineEditEntityName->setEnabled(false);
    ui->lineEditI2cBus->setEnabled(false);
    ui->lineEditVideoNode->setEnabled(false);
    ui->lineEditSubNode->setEnabled(false);
    ui->lineEditMediaNode->setEnabled(false);

    connect(this, &FramelessWidget::titleDblClick, this, &MainWidget::onTitleDblClick);

    connect(ui->pushButtonClose, &QPushButton::clicked, this, [this]() {
        if (QxHelp::showMsgBoxQuesion(tr("提示"), tr("确定退出?"))) {
            hide();
            saveSysCfg();
            closeCam();
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
    connect(ui->pushButtonTrans, &QPushButton::clicked, this, [this]() {
        if (m_language) {
            bool bSuccess = m_translator.load(m_strCfgPath + "veye_viewer_zh_CN.qm");
            if (bSuccess) {
                m_language = 0;

                ui->pushButtonTrans->setIcon(QIcon(":/image/en.png"));
            }
        } else {
            bool bSuccess = m_translator.load(m_strCfgPath + "veye_viewer_en_US.qm");
            if (bSuccess) {
                m_language = 1;

                ui->pushButtonTrans->setIcon(QIcon(":/image/ch.png"));
            }
        }
        qApp->installTranslator(&m_translator);
        ui->retranslateUi(this);
        emit sndUpdateUI();
        transUi();
 
    });
    connect(ui->pushButtonHelp, &QPushButton::clicked, this, [this]() {
        AboutDialog *dlg = new AboutDialog(this);
        QxHelp::moveParentCenter(this, dlg);
        dlg->show();
    });

    connect(ui->comboBoxPlatform,
            QOverload<int>::of(&QComboBox::activated),
            this,
            [this](int index) { m_euPlatform = (EuPlatform) index; });

    connect(ui->comboBoxDev, QOverload<int>::of(&QComboBox::activated), this, [this]() {
        if (m_camRunning) {
            closeCam();
            ui->pushButtonCapture->setChecked(false);
            ui->pushButtonShow->setChecked(false);
        }
        setCamIndex(StCamIndex());
        QString strValue = ui->comboBoxDev->currentText();
        if (m_mapAutoCamIndex.contains(strValue)) {
            StCamIndex camIndex = m_mapAutoCamIndex[strValue];
            setCamIndex(camIndex);
        }
        readSysfs();
        QString strI2C = ui->lineEditI2cBus->text();
        if (QxHelp::hasWritePermission(strI2C.toStdString().c_str())) {
            emit ui->pushButtonGetCamParam->clicked();
        }        
    });

    connect(ui->pushButtonSearchCamera, &QPushButton::clicked, this, [this]() {
        ui->comboBoxDev->clear();
        m_mapAutoCamIndex.clear();
        readCamIndex();
        if (m_mapAutoCamIndex.size() > 0) {
            emit ui->comboBoxDev->activated(0);
        }
        
    });

    connect(ui->pushButtonGetCamParam, &QPushButton::clicked, this, [this]() {
        readSysfs();
        switch (m_curIndex) {
        case CurPage::ImgAttrDlg:
            getImgAttr();
            break;
        case CurPage::ExpGainDlg:
            getExpAndGain();
            break;
        case CurPage::WhiteBlanceDlg:
            getWB();
            break;
        case CurPage::ImgProcDlg:
            getImgProc();
            break;
        case CurPage::TriggerDlg:
            getTrigger();
            break;
        case CurPage::RegisterDlg:
            break;
        case CurPage::CurAttrDlg:
            getCurAttr();
            break;
        default:
            break;
        };
        getBaseInfo();
    });

    connect(ui->pushButtonCapture, &QPushButton::clicked, this, [this](bool checked) {
        // ui->pushButtonCapture->setEnabled(false);
        // QTimer::singleShot(300, this, [this]() { ui->pushButtonCapture->setEnabled(true); });
        if (tr("开始采集") == ui->pushButtonCapture->text()) {
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
    connect(ui->comboBox_showFPS,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index) { m_setFps = ui->comboBox_showFPS->currentData().toInt(); });

    connect(ui->pushButtonSaveImg, &QPushButton::clicked, this, [this]() {
        ui->pushButtonSaveImg->setEnabled(false);
        QString strPath = QApplication::applicationDirPath() + "/image";
        strPath = !ui->lineEditSavePath->text().isEmpty() ? ui->lineEditSavePath->text() : strPath;
        QxHelp::makeFolder(strPath);
        QxHelp::makeFolderTail(strPath);
        int count = ui->spinBoxSaveCount->value();
        emit sndSaveImg(strPath, count);
    });

    connect(ui->pushButtonCleanOptLog, &QPushButton::clicked, this, [this]() {
        ui->tableWidgetLog->clearContents();
        ui->tableWidgetLog->setRowCount(0);
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
                        {
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

    connect(ui->pushButtonImgAttr, &QPushButton::clicked, this, [this]() {
        m_curIndex = CurPage::ImgAttrDlg;
        showCtrlWidget();
    });
    connect(ui->pushButtonExpAndGain, &QPushButton::clicked, this, [this]() {
        m_curIndex = CurPage::ExpGainDlg;
        showCtrlWidget();
    });
    connect(ui->pushButtonTrigger, &QPushButton::clicked, this, [this]() {
        m_curIndex = CurPage::TriggerDlg;
        showCtrlWidget();
    });
    connect(ui->pushButtonRegister, &QPushButton::clicked, this, [this]() {
        m_curIndex = CurPage::RegisterDlg;
        showCtrlWidget();
    });
    connect(ui->pushButtonCurAttr, &QPushButton::clicked, this, [this]() {
        m_curIndex = CurPage::CurAttrDlg;
        showCtrlWidget();
    });
    connect(ui->pushButtonWhiteBlance, &QPushButton::clicked, this, [this]() {
        m_curIndex = CurPage::WhiteBlanceDlg;
        showCtrlWidget();
    });
    connect(ui->pushButtonImgProc, &QPushButton::clicked, this, [this]() {
        m_curIndex = CurPage::ImgProcDlg;
        showCtrlWidget();
    });

    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        QString i2c = ui->lineEditI2cBus->text();
        if (!i2c.isEmpty()) {
            getCurAttr();
            readCmd(exptime, m_pExpAndGain, false);
            readCmd(curgain, m_pExpAndGain, false);

            readCmd(colortemp, m_pWhiteBlance, false);
            readCmd(currgain, m_pWhiteBlance, false);
            readCmd(curbgain, m_pWhiteBlance, false);
        }

        ui->label_showFps->setText(QString::number(m_lblFps, 'f', 2));
        m_lblFps = 0;
        
    });
    m_timer->start(1000);

    QTimer::singleShot(0, this, [this]() {
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            m_setFps = ui->comboBox_showFPS->currentData().toInt();
        }

        ui->comboBoxPlatform->setCurrentIndex((int) m_euPlatform);

        emit ui->comboBoxDev->setCurrentIndex(0);

        setCamIndex(StCamIndex());
        QString strValue = ui->comboBoxDev->currentText();
        if (m_mapAutoCamIndex.contains(strValue)) {
            StCamIndex camIndex = m_mapAutoCamIndex[strValue];
            setCamIndex(camIndex);
        }

        if (1 == m_language) {
            m_language = 0;
            emit ui->pushButtonTrans->clicked();
        } else {
            QString strI2C = ui->lineEditI2cBus->text();
            if (QxHelp::hasWritePermission(strI2C.toStdString().c_str())) {
                emit ui->pushButtonGetCamParam->clicked();
            }
        }

        showBGImg();
    });

    resize(1800, 1000);
    QxHelp::moveCenter(this);
}

void MainWidget::transUi()
{
    ui->labelTitle->setText(QString(APP_NAME) + " V" + APP_VERSION);
    emit ui->pushButtonGetCamParam->clicked();
    if (m_language) {
        QStringList headers;
        headers << tr("Num") << tr("Result") << tr("Operation Log");
        ui->tableWidgetLog->setColumnCount(headers.size());
        ui->tableWidgetLog->setHorizontalHeaderLabels(headers);
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
    } else {
        QStringList headers;
        headers << tr("序号") << tr("结果") << tr("操作记录");
        ui->tableWidgetLog->setColumnCount(headers.size());
        ui->tableWidgetLog->setHorizontalHeaderLabels(headers);
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
    }
    QxHelp::setWindowIdentity(this, APP_NAME);
}

void MainWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    if (!m_camRunning) {
        showBGImg();
    }
    if (m_gvSize != ui->graphicsView->size()) {
        m_gvSize = ui->graphicsView->size();
        qDebug() << "m_gvSize" << m_gvSize;
    }
}

bool MainWidget::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == ui->graphicsView) && (event->type() == QEvent::MouseButtonDblClick)
        && m_camImgShow) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            if (!isFullScreen) {
                ui->graphicsView->setAttribute(Qt::WA_TranslucentBackground);
                ui->graphicsView->setWindowFlags(ui->graphicsView->windowFlags()
                                                 | Qt::FramelessWindowHint);
                ui->graphicsView->setWindowState(Qt::WindowFullScreen);
                ui->graphicsView->setParent(nullptr);
                ui->graphicsView->showMaximized();
                m_gvSrcSize = m_gvSize;
                m_gvSize = QApplication::primaryScreen()->availableGeometry().size();
            } else {
                ui->graphicsView->showNormal();
                ui->graphicsView->setWindowState(Qt::WindowNoState);
                ui->graphicsView->setParent(ui->widgetGV);
                ui->widgetGV->layout()->addWidget(ui->graphicsView);
                m_gvSize = m_gvSrcSize;
            }

            isFullScreen = !isFullScreen;
            return true; // 事件已处理，不再传递
        }
    }

    return FramelessWidget::eventFilter(watched, event);
}

void MainWidget::onTitleDblClick()
{
    emit ui->pushButtonMax->clicked();
}

bool MainWidget::readRegCfg()
{
    QString strJson = m_strCfgPath + m_register;
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
        QString savePath = m_jsonSysCfg["img_path"].asString().c_str();
        if (QxHelp::hasWritePermission(savePath.toStdString().c_str())) {
            savePath = QApplication::applicationDirPath() + "/image";
        }

        ui->lineEditSavePath->setText(savePath);
        m_nChachFrameSize = m_jsonSysCfg["cache_frame_size"].asInt();
        m_nChachFrameSize = m_nChachFrameSize > 0 ? m_nChachFrameSize : 3;
        m_language = m_jsonSysCfg["language"].asInt();
        emit ui->comboBox_showFPS->currentIndexChanged(m_jsonSysCfg["show_fps"].asInt());
    } else {
        qDebug() << "readSysCfg " << strJson << " failed!!!";
    }
}

void MainWidget::saveSysCfg()
{
    QString strJson = m_strCfgPath + "syscfg.json";
    m_jsonSysCfg["img_path"] = ui->lineEditSavePath->text().toStdString();
    m_jsonSysCfg["cache_frame_size"] = m_nChachFrameSize;
    m_jsonSysCfg["language"] = m_language;
    m_jsonSysCfg["show_fps"] = ui->comboBox_showFPS->currentIndex();
    QxHelp::writeJson(strJson, m_jsonSysCfg);
}

void MainWidget::onSetRoiAndFps(const StImgAttrInfo &info)
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
        //QString addr = m_i2cAddr.mid(2, m_i2cAddr.length() - 2);

        m_camParam.videoNode = videoNode;
        m_camParam.subNode = subNode;
        m_camParam.x = strX.toInt();
        m_camParam.y = strY.toInt();
        m_camParam.w = strW.toInt();
        m_camParam.h = strH.toInt();
        m_camParam.fps = strFps.toInt();

        QString strCmd;
        switch (m_euPlatform) {
        case EuPlatform::Rockchip: { 
            if (EuCamType::MvCamera == m_euCamType) {        
                strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_x=%2").arg(subNode, strX);
                m_cam->runSystemCmd(strCmd);
                strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_y=%2").arg(subNode, strY);
                m_cam->runSystemCmd(strCmd);
                strCmd = QString("media-ctl -d %1 --set-v4l2 \'\"%2\":0[fmt:%3/%4x%5@1/%6]\'")
                             .arg(mediaNode, entityName, "Y8_1X8", strW, strH, strFps);
                m_cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps, strFps.toFloat() * 100, true);
            } else if (EuCamType::GxCamera == m_euCamType) {
                strCmd = QString("media-ctl -d %1 --set-v4l2 \'\"%2\":0[fmt:%3/%4x%5@1/%6]\'")
                             .arg(mediaNode,
                                  entityName,
                                  "UYVY8_2X8",
                                  strW,
                                  strH,
                                  QString::number(static_cast<int>(strFps.toFloat())));
                m_cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps_gx, strFps.toFloat() * 10000, true);
                qDebug() << strCmd;
            }
        } break;
        case EuPlatform::RaspberryPi: {
            if (EuCamType::MvCamera == m_euCamType) {
                strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_x=%2").arg(videoNode, strX);
                m_cam->runSystemCmd(strCmd);
                strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_y=%2").arg(videoNode, strY);
                m_cam->runSystemCmd(strCmd);
                strCmd = QString(
                             "v4l2-ctl -d %1 --set-fmt-video=width=%2,height=%3,pixelformat=GREY")
                             .arg(videoNode, strW, strH);
                m_cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps, strFps.toFloat() * 100, true);
            } else if (EuCamType::GxCamera == m_euCamType) {
                strCmd = QString(
                             "v4l2-ctl -d %1 --set-fmt-video=width=%2,height=%3,pixelformat=UYVY")
                             .arg(videoNode, strW, strH);
                m_cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps_gx, strFps.toFloat() * 10000, true);
            }
            // strCmd = QString("v4l2-ctl -d %1 --set-ctrl frame_rate=%2").arg(videoNode, strFps);
            // m_cam->runSystemCmd(strCmd);

        } break;
        case EuPlatform::RaspberryPi5: {
            if (EuCamType::MvCamera == m_euCamType) {
                strCmd
                    = m_strCfgPath + "config_camera-rpi5.sh"
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
                m_cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps, strFps.toFloat() * 100, true);
            } else if (EuCamType::GxCamera == m_euCamType) {
                strCmd
                    = m_strCfgPath + "config_camera-rpi5.sh"
                      + QString::asprintf(" gxcam -fmt UYVY -x %d -y %d -w %d -h %d -bus %d -media "
                                          "%s -subdev %s -video %s",
                                          strX.toInt(),
                                          strY.toInt(),
                                          strW.toInt(),
                                          strH.toInt(),
                                          i2cBus.toInt(),
                                          mediaNode.toStdString().c_str(),
                                          subNode.toStdString().c_str(),
                                          videoNode.toStdString().c_str());
                m_cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps_gx, strFps.toFloat() * 10000, true);
            }
            // strCmd = QString("v4l2-ctl -d %1 --set-ctrl frame_rate=%2").arg(subNode, strFps);
            // m_cam->runSystemCmd(strCmd);
        } break;
        case EuPlatform::Jetson: {
            if (EuCamType::MvCamera == m_euCamType) {
                strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_x=%2").arg(videoNode, strX);
                m_cam->runSystemCmd(strCmd);
                strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_y=%2").arg(videoNode, strY);
                m_cam->runSystemCmd(strCmd);
                strCmd = QString(
                             "v4l2-ctl -d %1 --set-fmt-video=width=%2,height=%3,pixelformat=GREY")
                             .arg(videoNode, strW, strH);
                m_cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps, strFps.toFloat() * 100, true);
            } else if (EuCamType::GxCamera == m_euCamType) {
                strCmd = QString(
                             "v4l2-ctl -d %1 --set-fmt-video=width=%2,height=%3,pixelformat=UYVY")
                             .arg(videoNode, strW, strH);
                m_cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps_gx, strFps.toFloat() * 10000, true);
            }
            // strCmd = QString("v4l2-ctl -d %1 --set-ctrl frame_rate=%2").arg(videoNode, strFps);
            // m_cam->runSystemCmd(strCmd);
            int width = strW.toInt();
            if (0 != width % 256) {
                width = (width / 256 + 1) * 256;
            }
            strCmd
                = QString("v4l2-ctl -d %1 --set-ctrl preferred_stride=%2").arg(videoNode).arg(width);
            m_cam->runSystemCmd(strCmd);
        } break;
        default:
            break;
        }
        if (info.read) {
            getImgAttr();        
        }
    }
}

void MainWidget::onCamStatus(EuCamStatus status)
{
    switch (status) {
    case EuCamStatus::OPEN_STA: {
        ui->pushButtonSaveImg->setEnabled(true);
        //ui->pushButtonShow->setText(tr("停止显示"));
        ui->pushButtonCapture->setText(tr("停止采集"));
        ui->pushButtonShow->setEnabled(true);
    } break;
    case EuCamStatus::CLOSE_STA: {
        ui->pushButtonSaveImg->setEnabled(false);
        ui->pushButtonCapture->setText(tr("开始采集"));
        ui->pushButtonShow->setText(tr("开始显示"));
        rcvFrameInfo(StCamInfo());
        ui->pushButtonShow->setEnabled(false);
        showBGImg();
        m_camRunning = false;
    } break;
    case EuCamStatus::HIDE_STA: {
        showBGImg();
    } break;
    case EuCamStatus::READ_TIMEMOU: {
        QxToast::showTip("Data reading timed out. Please try restarting");
    } break;
    default:
        break;
    }
}

void MainWidget::openCam()
{
    if (!m_camRunning) {
        qDebug() << ".........openCam.........";
        emit sndSetROI(false);
        QString videoNode = ui->lineEditVideoNode->text();
        switch (m_euPlatform) {
        case EuPlatform::Rockchip: {
        } break;
        case EuPlatform::RaspberryPi: {
        } break;
        case EuPlatform::RaspberryPi5: {
        } break;
        case EuPlatform::Jetson: {
            QString strCmd = QString("v4l2-ctl -d %1 --set-ctrl vi_time_out_disable=1")
                                 .arg(videoNode);
            m_cam->runSystemCmd(strCmd);
        } break;
        default:
            break;
        };

        m_camRunning = m_cam->OpenDevice(m_camParam);
        if (m_camRunning) {
            onCamStatus(EuCamStatus::OPEN_STA);
        } else {
            onCamStatus(EuCamStatus::CLOSE_STA);
        }
    }
}

void MainWidget::closeCam()
{
    if (m_camRunning) {
        qDebug() << ".........closeCam.........";
        QString videoNode = ui->lineEditVideoNode->text();
        switch (m_euPlatform) {
        case EuPlatform::Rockchip: {
        } break;
        case EuPlatform::RaspberryPi: {
        } break;
        case EuPlatform::RaspberryPi5: {
        } break;
        case EuPlatform::Jetson: {
            QString strCmd = QString("v4l2-ctl -d %1 --set-ctrl vi_time_out_disable=0")
                                 .arg(videoNode);
            m_cam->runSystemCmd(strCmd);
        } break;
        default:
            break;
        }
        m_cam->CloseDevice();
        ui->pushButtonCapture->setChecked(false);
        closeShow();
    }
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
    if (QxHelp::isDirectoryExists(m_sysfsPath)) {
        QString model = "";
        bool bRet = QxHelp::readFirstLine(m_sysfsPath + "camera_model", model);
        QString strValue = "read " + m_sysfsPath + "camera_model:" + model;
        rcvOptLog(bRet ? 0 : -1, strValue);
        ui->label_model->setText(model);

        QString strW = "";
        bRet = QxHelp::readFirstLine(m_sysfsPath + "width", strW);
        strValue = "read " + m_sysfsPath + "width:" + strW;
        rcvOptLog(bRet ? 0 : -1, strValue);

        QString strH = "";
        bRet = QxHelp::readFirstLine(m_sysfsPath + "height", strH);
        strValue = "read " + m_sysfsPath + "height:" + strH;
        rcvOptLog(bRet ? 0 : -1, strValue);

        QString strFps = "";
        bRet = QxHelp::readFirstLine(m_sysfsPath + "fps", strFps);
        strValue = "read " + m_sysfsPath + "fps:" + strFps;
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
        m_platform = QString::fromStdString(jsonRoot["platform"].asString()).toLower();
        m_euPlatform = mapPlatform[m_platform];
    } else {
        qCritical() << "readPlatform:" << strJsonPlatform << " failed!!!";
    }
}

void MainWidget::readCamIndex()
{
    //qDebug() << "MainWidget::readCamIndex() is called " << QApplication::applicationDirPath();
    QString i2c_bus;
    QString strCmd = m_strCfgPath + mapProbefile[m_platform];
    if (0 == system(strCmd.toStdString().c_str())) {
        QString strJson = m_strCfgPath + "auto_camera_index.json";
        if (QxHelp::fileExists(strJson.toStdString().c_str())) {
            Json::Value jsonRoot;
            if (QxHelp::readJson(strJson, jsonRoot)) {
                int total = jsonRoot.size();
                for (int i = 0; i < total; ++i) {
                    Json::Value jsonNode = jsonRoot[i];
                    StCamIndex camIndex;
                    QString i2c_bus_num = QString::fromStdString(jsonNode["i2c_bus"].asString());
                    camIndex.i2c_bus = "/dev/i2c-" + i2c_bus_num;
                    camIndex.i2c_bus_num = i2c_bus_num.toInt();
                    camIndex.media_entity_name = jsonNode["media_entity_name"].asString().c_str();
                    camIndex.media_node = jsonNode["media_node"].asString().c_str();
                    camIndex.video_node = jsonNode["video_node"].asString().c_str();
                    camIndex.video_subnode = jsonNode["video_subnode"].asString().c_str();
                    ui->comboBoxDev->addItem(camIndex.video_node);
                    m_mapAutoCamIndex.insert(camIndex.video_node, camIndex);
                    if (!i2c_bus_num.isEmpty()) {
                        i2c_bus = i2c_bus_num;
                    }
                }
            } else {
                qDebug() << "auto_camera_index.json  readJons error";
            }
        } else {
            qDebug() << "auto_camera_index.json  does not exist ";
        }
    } else {
        qCritical() << "runShell " << strCmd << " failed";
    }

    if (!i2c_bus.isEmpty()) {
        QString basePath = "/sys/bus/i2c/devices/i2c-" + i2c_bus + "/" + i2c_bus + "-003b/";
        {
            QString path = basePath + "/veye_mvcam/";
            if (QxHelp::isDirectoryExists(path)) {
                m_sysfsPath = path;
                m_euCamType = EuCamType::MvCamera;
                return;
            }
        }
        {
            QString path = basePath + "/veye_gxcam/";
            if (QxHelp::isDirectoryExists(path)) {
                m_sysfsPath = path;
                m_euCamType = EuCamType::GxCamera;
                return;
            }
        }
    } else {
        qWarning() << "i2cBus is empty, readCamType failed";
    }
}

void MainWidget::adjustAdaptive()
{
    switch (m_euCamType) {
    case EuCamType::MvCamera:
        m_register = "mv_register.json";
        break;
    case EuCamType::GxCamera:
        m_register = "gx_register.json";
        break;
    default:
        qWarning() << "cam type is ?";
        break;
    }
}

void MainWidget::getBaseInfo()
{
    readCmd(version, this);
    readCmd(serialno, this);
}

void MainWidget::getImgAttr()
{
    if (EuCamType::MvCamera == m_euCamType) {
        readCmd(roi_x, m_pImgAttr);
        readCmd(roi_y, m_pImgAttr);
        readCmd(roi_w, m_pImgAttr);
        readCmd(roi_h, m_pImgAttr);
        readCmd(fps, m_pImgAttr);
    } else if (EuCamType::GxCamera == m_euCamType) {
        readCmd(videomodenum, m_pImgAttr);
        readCmd(fps_gx, m_pImgAttr);
    } else {
        qWarning() << "EuCamType is ???" << (int) m_euCamType;
    }
}

void MainWidget::getExpAndGain()
{
    readCmd(model, m_pExpAndGain);
    readCmd(expmode, m_pExpAndGain);
    if (EuCamType::MvCamera == m_euCamType) {
        readCmd(gainmode, m_pExpAndGain);
    }
    readCmd(exptime, m_pExpAndGain);
    readCmd(curgain, m_pExpAndGain);
    readCmd(aatarget, m_pExpAndGain);
    readCmd(aemaxtime, m_pExpAndGain);
    readCmd(agmaxgain, m_pExpAndGain);
    readCmd(metime, m_pExpAndGain);
    readCmd(mgain, m_pExpAndGain);
    if (EuCamType::MvCamera == m_euCamType) {
        readCmd(exptime_min, m_pExpAndGain);
        readCmd(exptime_max, m_pExpAndGain);
    }
}

void MainWidget::getTrigger()
{
    readCmd(imgacq, m_pTrigger);
    if (EuCamType::MvCamera == m_euCamType) {
        readCmd(trgmodecap, m_pTrigger);
        readCmd(trgmode, m_pTrigger);
    } else if (EuCamType::GxCamera == m_euCamType) {
        readCmd(workmodecap, m_pTrigger);
        readCmd(workmode, m_pTrigger);
    }
    readCmd(trgsrc, m_pTrigger);
    readCmd(trgexp_delay, m_pTrigger);
    readCmd(trgnum, m_pTrigger);
    readCmd(trginterval, m_pTrigger);
    readCmd(trgfilter_time, m_pTrigger);
    readCmd(trgdelay, m_pTrigger);
    readCmd(trgone, m_pTrigger);
}

void MainWidget::getCurAttr()
{
    readCmd(roi_w, m_pCurAttr, false);
    readCmd(roi_h, m_pCurAttr, false);
    if (EuCamType::MvCamera == m_euCamType) {
        readCmd(fps, m_pCurAttr, false);
    }
    readCmd(pixelformat, m_pCurAttr, false);
    readCmd(trgmode, m_pCurAttr, false);
    //readCmd(trgsrc, m_pCurAttr, false);
    if (EuCamType::GxCamera == m_euCamType) {
        readCmd(fps_gx, m_pCurAttr, false);
        readCmd(maxfps, m_pCurAttr, false);
        readCmd(readmode, m_pCurAttr, false);
    }
}

void MainWidget::getWB()
{
    readCmd(wbmode, m_pWhiteBlance);
    readCmd(awbcolortempmin, m_pWhiteBlance);
    readCmd(awbcolortempmax, m_pWhiteBlance);
    readCmd(mwbrgain, m_pWhiteBlance);
    readCmd(mwbbgain, m_pWhiteBlance);
    readCmd(colortemp, m_pWhiteBlance);
    readCmd(currgain, m_pWhiteBlance);
    readCmd(curbgain, m_pWhiteBlance);
}

void MainWidget::getImgProc()
{
    readCmd(saturation, m_pImgProc);
    readCmd(contrast, m_pImgProc);
    readCmd(hue, m_pImgProc);
}

uint32_t MainWidget::readRegData(EuCMD cmd, uint32_t reg, QObject *obj, bool showlog, bool async)
{
    uint32_t data = 0;
    QString i2c = ui->lineEditI2cBus->text();
    if (!i2c.isEmpty()) {
        StI2CCmd stCmd;
        stCmd.bWrite = false;
        stCmd.cmd = cmd;
        stCmd.camType = m_euCamType;
        stCmd.i2c = i2c;
        stCmd.addr = m_i2cAddr.toInt(nullptr, 16);
        stCmd.reg = reg;
        stCmd.data = 0;
        stCmd.obj = obj;
        stCmd.showlog = showlog;
        if (async) {
            m_cam->asynReadData(stCmd);
        } else {
            m_cam->syncReadData(stCmd);
            data = stCmd.data;
        }
    } else {
        QxToast::showTip(tr("i2c序号不能为空"), this);
    }
    return data;
}

bool MainWidget::writeRegData(EuCMD cmd, uint32_t reg, uint32_t data, bool showLog)
{
    bool bRet = false;
    if (m_mapAddrRegister.contains(reg)) {
        StRegInfo info = m_mapAddrRegister[reg];

        EuUpdateTiming type = (EuUpdateTiming) CodeMap::getUpdateTiming(info.update_timing);
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
        stCmd.camType = m_euCamType;
        stCmd.cmd = cmd;
        stCmd.i2c = i2cBus;
        stCmd.addr = m_i2cAddr.toInt(nullptr, 16);
        stCmd.reg = reg;
        stCmd.data = data;
        stCmd.obj = nullptr;
        stCmd.showlog = showLog;
        m_cam->asynWriteDate(stCmd);
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

void MainWidget::setCamIndex(StCamIndex camIndex)
{
    ui->lineEditEntityName->setText(camIndex.media_entity_name);
    ui->lineEditI2cBus->setText(camIndex.i2c_bus);
    ui->lineEditVideoNode->setText(camIndex.video_node);
    ui->lineEditSubNode->setText(camIndex.video_subnode);
    ui->lineEditMediaNode->setText(camIndex.media_node);
    QString model = "";
    QxHelp::readFirstLine(m_sysfsPath + "camera_model", model);
    ui->label_model->setText(model);
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

void MainWidget::showBGImg()
{
    m_gvSize = ui->graphicsView->size();
    QPixmap pix = QPixmap::fromImage(m_bkImage).scaled(m_gvSize / 4, Qt::KeepAspectRatio);
    m_pixItem->setPixmap(pix);
    m_pixItem->update();
    m_scene->setSceneRect(0, 0, pix.width(), pix.height());
    ui->labelTimestamp->setText("0");
    ui->label_showFps->setText("0");
}

void MainWidget::showCtrlWidget()
{
    for (int i = 0; i < m_btnCtrls.size(); ++i) {
        if (i == (int) m_curIndex) {
            m_btnCtrls[i]->setChecked(true);    
            m_winCtrls[i]->show();
        } else {
            m_btnCtrls[i]->setChecked(false);
            m_winCtrls[i]->hide();
        }
    }
    switch (m_curIndex) {
    case CurPage::ImgAttrDlg:
        getImgAttr();
        break;
    case CurPage::ExpGainDlg:
        getExpAndGain();
        break;
    case CurPage::WhiteBlanceDlg:
        getWB();
        break;
    case CurPage::ImgProcDlg:
        getImgProc();
        break;
    case CurPage::TriggerDlg:
        getTrigger();
        break;
    case CurPage::RegisterDlg:
        break;
    case CurPage::CurAttrDlg:
        getCurAttr();
        break;
    default:
        break;
    }
}

void MainWidget::pushData(const StImgInfo &sii)
{
    if (m_camImgShow) {
        m_mtxPix.lock();
        m_queueSii.push(sii);
        int size = m_queueSii.size();
        if (size > m_nChachFrameSize) {
            StImgInfo tmp = m_queueSii.front();
            m_queueSii.pop();
            if (new_buf == tmp.strategy) {
                delete[] tmp.buf;
            }
            //qDebug() << "cache frame size " << size << " > " << m_nChachFrameSize << ", drop frame";
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

bool MainWidget::isCamRunning()
{
    return m_camRunning;
}

QImage uyvyToQImage(unsigned char *uyvyData, int dataSize, int width, int height, int bytesPerLine)
{
    // 1. 严格参数校验（避免数据不足导致取到无效值）
    if (width <= 0 || height <= 0 || !uyvyData) {
        qWarning() << "无效参数：宽高为0或数据为空";
        return QImage();
    }
    if (bytesPerLine < width * 2) {
        qWarning() << "bytesPerLine不足：需要至少" << width * 2 << "，实际" << bytesPerLine;
        return QImage();
    }
    if (dataSize < height * bytesPerLine) {
        qWarning() << "数据长度不足：需要至少" << height * bytesPerLine << "，实际" << dataSize;
        return QImage();
    }

    // 2. 创建目标图像（RGB888格式：R/G/B顺序）
    QImage image(width, height, QImage::Format_RGB888);
    if (image.isNull()) {
        qWarning() << "无法创建QImage";
        return QImage();
    }

    // 3. BT.601标准（整数计算优先，避免浮点误差导致亮度异常）
    const int Y_MIN = 16; // Y的有效范围：16~235（BT.601）
    const int Y_MAX = 235;
    const int UV_MIN = 16; // UV的有效范围：16~240（部分设备可能是0~255，可调整）
    const int UV_MAX = 240;
    const int UV_OFFSET = 128;

    // 像素钳位（确保RGB在0~255）
    auto clamp = [](int val) -> uchar { return static_cast<uchar>(qBound(0, val, 255)); };

    // 4. 逐行逐像素对转换（核心修复：UYVY索引计算）
    for (int y = 0; y < height; ++y) {
        // 当前行UYVY数据起始地址（按bytesPerLine偏移，正确处理对齐）
        unsigned char *uyvyLine = uyvyData + y * bytesPerLine;
        // 当前行RGB数据起始地址
        uchar *rgbLine = image.scanLine(y);

        // 每次处理1个UYVY像素对（4字节 → 2个RGB像素）
        for (int xPair = 0; xPair < width; xPair += 2) {
            // 计算当前像素对在UYVY行中的起始字节索引（关键修复！）
            int uyvyByteIdx = xPair * 2; // 每个RGB像素对应2字节UYVY（4字节/2像素）
            // 避免超出当前行的有效数据（bytesPerLine是对齐后的总字节数）
            if (uyvyByteIdx + 3 >= bytesPerLine) {
                qWarning() << "UYVY行数据不足：行" << y << "像素对" << xPair / 2 << "索引越界";
                break;
            }

            // 正确提取UYVY分量（严格按格式顺序：U0 Y0 V0 Y1）
            unsigned char U = uyvyLine[uyvyByteIdx + 0];
            unsigned char Y0 = uyvyLine[uyvyByteIdx + 1];
            unsigned char V = uyvyLine[uyvyByteIdx + 2];
            unsigned char Y1 = uyvyLine[uyvyByteIdx + 3];

            // 校验并修正Y/UV的有效范围（避免原始数据异常导致亮度为0）
            Y0 = qBound(Y_MIN, (const int) Y0, Y_MAX);
            Y1 = qBound(Y_MIN, (const int) Y1, Y_MAX);
            U = qBound(UV_MIN, (const int) U, UV_MAX);
            V = qBound(UV_MIN, (const int) V, UV_MAX);

            // 转换为归一化值（避免浮点溢出）
            int Y0_norm = Y0 - Y_MIN;
            int Y1_norm = Y1 - Y_MIN;
            int U_norm = (U - UV_OFFSET) * 255 / (UV_MAX - UV_MIN); // 映射到-127~127
            int V_norm = (V - UV_OFFSET) * 255 / (UV_MAX - UV_MIN);

            // --------------------------
            // YUV转RGB（整数计算，无精度损失）
            // --------------------------
            // 像素0（xPair列）
            int R0 = Y0_norm + (V_norm * 1402) / 1000;                        // 1.402 → 1402/1000
            int G0 = Y0_norm - (U_norm * 344) / 1000 - (V_norm * 714) / 1000; // 0.344、0.714
            int B0 = Y0_norm + (U_norm * 1772) / 1000;                        // 1.772 → 1772/1000
            // 写入RGB（顺序：R→G→B）
            rgbLine[xPair * 3 + 0] = clamp(R0);
            rgbLine[xPair * 3 + 1] = clamp(G0);
            rgbLine[xPair * 3 + 2] = clamp(B0);

            // 像素1（xPair+1列，处理宽度为奇数的情况）
            if (xPair + 1 < width) {
                int R1 = Y1_norm + (V_norm * 1402) / 1000;
                int G1 = Y1_norm - (U_norm * 344) / 1000 - (V_norm * 714) / 1000;
                int B1 = Y1_norm + (U_norm * 1772) / 1000;
                rgbLine[(xPair + 1) * 3 + 0] = clamp(R1);
                rgbLine[(xPair + 1) * 3 + 1] = clamp(G1);
                rgbLine[(xPair + 1) * 3 + 2] = clamp(B1);
            }
        }
    }

    return image;
}

void MainWidget::workThread()
{
    QElapsedTimer timer;
    timer.restart();

    while (m_bThread) {
        double setFrameIntv = 1000.0 / m_setFps * 1000;
        if (timer.nsecsElapsed() / 1000 >= setFrameIntv) {
            //qDebug() << setFrameIntv << timer.nsecsElapsed() / 1000;
            timer.restart();
            StImgInfo sii;
            QImage img;
            m_mtxPix.lock();
            int count = m_queueSii.size();
            if (count > 0) {
                sii = m_queueSii.front();
                m_queueSii.pop();
            }
            m_mtxPix.unlock();

            if (count > 0) {
                if (m_camImgShow) {
                    if (EuCamType::GxCamera == m_euCamType) {
                        img = uyvyToQImage(sii.buf,
                                           sii.len,
                                           sii.width,
                                           sii.height,
                                           sii.nBytesperline);
                    } else if (EuCamType::MvCamera == m_euCamType) {
                        img = QImage(sii.buf,
                                     sii.nBytesperline,
                                     sii.height,
                                     QImage::Format_Grayscale8);
                    }
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
        } else {
        }
        QThread::usleep(1);
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void MainWidget::rcvPixmap(const QPixmap &pix, QString strTS)
{
    QElapsedTimer timer;

    if (m_camImgShow) {
        m_lblFps++;
        timer.start(); // 开始计时
        if (m_lastCmd == trgone) {
            m_trgoneCount++;
        } else {
            m_trgoneCount = 0;
        }

        if (0 == m_showFps) {
            timestamps.clear();
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

void MainWidget::rcvOptLog(int ret, QString strMsg, bool bShow)
{
    if (bShow) {
        int col = 0;
        int row = 0;
        ui->tableWidgetLog->setUpdatesEnabled(false);
        ui->tableWidgetLog->setSortingEnabled(false);
        ui->tableWidgetLog->insertRow(row);
        QTableWidgetItem *itemNum = new QTableWidgetItem(QString::number(++m_optNum));
        QTableWidgetItem *itemErr = new QTableWidgetItem(QString::number(ret));

        QTableWidgetItem *itemMsg = new QTableWidgetItem(strMsg);
        if (0 != ret) {
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
        case fps_gx:
            m_CamFps = data / 1000.0;
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
    } else if (m_pWhiteBlance == qobject_cast<WhiteBlanceDlg *>(ret.obj)) {
        emit sndWBCmdRet(ret);
    } else if (m_pImgProc == qobject_cast<ImgProcDlg *>(ret.obj)) {
        emit sndImgProcCmdRet(ret);
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

void MainWidget::asyncWriteCmd(EuCMD cmd, uint32_t data, bool bShow)
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

    if (m_mapCmdReg.contains(cmd)) {
        QString strReg = m_mapCmdReg.value(cmd);
        uint32_t reg = strReg.midRef(2).toInt(nullptr, 16);
        writeRegData(cmd, reg, data, bShow);
    }
}

void MainWidget::asyncReadCmd(EuCMD cmd, QObject *obj, bool bShow)
{
    readCmd(cmd, obj, bShow, true);
    return;
}

uint32_t MainWidget::readCmd(EuCMD cmd, QObject *obj, bool bShow, bool async)
{
    uint32_t data = 0;
    if (m_mapCmdReg.contains(cmd)) {
        QString strReg = m_mapCmdReg.value(cmd);
        uint32_t reg = strReg.midRef(2).toInt(nullptr, 16);
        data = readRegData(cmd, reg, obj, bShow, async);
    }
    return data;
}

void MainWidget::rcvOneAEAG(int nAemaxtime, int nAgmaxgain, int nAatarget)
{
    //[step.1]:set.AE.AG.Max.value
    asyncWriteCmd(aatarget, nAatarget);
    asyncWriteCmd(aemaxtime, nAemaxtime);
    asyncWriteCmd(agmaxgain, nAgmaxgain);
    //2 [step.2]:.set·AE·AG.to.ONCE.moden
    asyncWriteCmd(expmode, 1);
    asyncWriteCmd(gainmode, 1);
    //[step.3]:.wait.5.seconds
    cmdSleep(5000, false);
    QTimer::singleShot(5000, this, [this] {
        //[step.4]:.check.oNcE.result
        uint32_t expValue = readCmd(exptime, m_pExpAndGain, true, false);
        uint32_t gainValue = readCmd(curgain, m_pExpAndGain, true, false);
        emit sndExpGainValue(expValue, gainValue);
        //[step.5]:.set.to.manual.mode
        uint32_t expMode = 0;
        uint32_t gainMode = 0;
        asyncWriteCmd(expmode, 0);
        asyncWriteCmd(gainmode, 0);
        asyncWriteCmd(metime, expValue);
        asyncWriteCmd(mgain, gainValue);
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
        asyncWriteCmd(paramsave, 1);
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
                    uint32_t data = readRegData(def_cmd, reg, nullptr, true, false);
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
    ui->labelRES->setText(QString("%1 ∗ %2").arg(info.width).arg(info.height));
}
