#include "MainWidget.h"
#include <QDebug>
#include <QFormLayout>
#include <QGridLayout>
#include <QKeyEvent>
#include <QMenuBar>
#include <QOpenGLWidget>
#include <QProcess>
#include <QResizeEvent>
#include <QScreen>
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
    // 帧处理线程会访问 m_channels[].queueSii/mtxPix，必须先 join 再拆通道，
    // 否则就是 use-after-free（以前 detach 后直接退出，靠进程退出兜底）。
    if (m_threadWork.joinable()) {
        m_threadWork.join();
    }
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        // 退出时事件循环已结束，deleteLater 不会被处理，这里必须同步销毁，
        // 才轮到 ~Camera 把采集线程/命令线程 join 掉。
        teardownChannel(i, true);
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
        m_mapCmdReg[videomodecap] = "0x005C"; //gx
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
    CommDevCfg devCfg = {m_euCamType, m_euPlatform, 0};
    qRegisterMetaType<StCmdRet>("StCmdRet");
    connect(this, &MainWidget::sndPixmap, this, &MainWidget::rcvPixmap);

    // 保存线程句柄，析构时 join（见 ~MainWidget）
    m_threadWork = std::thread([this]() { workThread(); });

    // 多相机：每个通道的 Camera 懒创建(首次绑定设备时)，此处不再创建单个 m_cam。
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

    // ---- 多相机通道视图初始化 ----
    {
        QGraphicsView *views[MAX_CHANNELS] = {ui->gv0, ui->gv1, ui->gv2, ui->gv3};
        for (int i = 0; i < MAX_CHANNELS; ++i) {
            m_channels[i].index = i;
            m_channels[i].view = views[i];
            m_channels[i].scene = new QGraphicsScene(this);
            views[i]->setScene(m_channels[i].scene);
            m_channels[i].pixItem = new QGraphicsPixmapItem();
            views[i]->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
            views[i]->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            views[i]->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            m_channels[i].scene->addItem(m_channels[i].pixItem);
            views[i]->installEventFilter(this);
            m_channels[i].gvSize = views[i]->size();
        }
        // 修正布局伸缩：图像区域始终扩展、工具栏固定高度，
        // 避免单画面模式下顶部工具栏因伸缩传递失效而占据上半部分。
        ui->gridLayoutViews->setRowStretch(0, 1);
        ui->gridLayoutViews->setRowStretch(1, 1);
        ui->gridLayoutViews->setColumnStretch(0, 1);
        ui->gridLayoutViews->setColumnStretch(1, 1);
        ui->widgetGV->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->widget_Img->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        applyViewLayout();
    }

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

    ui->lineEditSavePath->setFixedWidth(150);
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

    int btnWidth = 92;
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
        QString strValue = ui->comboBoxDev->currentText();
        if (!m_mapAutoCamIndex.contains(strValue)) {
            return;
        }
        StCamIndex camIndex = m_mapAutoCamIndex[strValue];
        // 选设备 -> 分配/激活到某个通道(已绑定则直接激活;否则空闲槽位优先)
        int ch = pickChannelForDevice(strValue);
        bindDeviceToChannel(ch, camIndex);
        setActiveChannel(ch);
        readSysfs();
        QString strI2C = ui->lineEditI2cBus->text();
        if (QxHelp::hasWritePermission(strI2C.toStdString().c_str())) {
            emit ui->pushButtonGetCamParam->clicked();
        }
    });

    connect(ui->pushButtonSearchCamera, &QPushButton::clicked, this, [this]() {
        // 重新搜索前停止并复位所有通道（含排空帧队列、销毁 Camera 对象）
        for (int i = 0; i < MAX_CHANNELS; ++i) {
            teardownChannel(i);
            showChannelBg(i);
        }
        m_active = 0;
        ui->comboBoxDev->clear();
        m_mapAutoCamIndex.clear();
        readCamIndex();
        if (m_mapAutoCamIndex.size() > 0) {
            emit ui->comboBoxDev->activated(0);
        } else {
            updatePanelForActive();
            showBGImg();
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

    connect(ui->pushButtonLayout, &QPushButton::clicked, this, [this]() { toggleViewLayout(); });
    connect(ui->pushButtonFullScreen, &QPushButton::clicked, this, [this]() { toggleFullscreen(); });

    connect(ui->radioButton_adaptive, &QRadioButton::clicked, this, [this]() {
        m_scale = true;
        for (int i = 0; i < MAX_CHANNELS; ++i) {
            m_channels[i].view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            m_channels[i].view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    });
    connect(ui->radioButton_src, &QRadioButton::clicked, this, [this]() {
        m_scale = false;
        for (int i = 0; i < MAX_CHANNELS; ++i) {
            m_channels[i].view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            m_channels[i].view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
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
        if (cur().cam) {
            cur().cam->saveImg(strPath, count);
        }
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
                // 结尾判断用 endsWith：lastIndexOf 在下标 0 时也会等于 length-4（size_t 下溢）
                if (!savePath.endsWith(".txt", Qt::CaseInsensitive)) {
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
        m_setFps = ui->comboBox_showFPS->currentData().toInt();
        ui->comboBoxPlatform->setCurrentIndex((int) m_euPlatform);

        // 默认将第一个探测到的设备绑定到通道0并激活
        if (m_mapAutoCamIndex.size() > 0) {
            QString strValue = ui->comboBoxDev->itemText(0);
            ui->comboBoxDev->setCurrentIndex(0);
            if (m_mapAutoCamIndex.contains(strValue)) {
                int ch = pickChannelForDevice(strValue);
                bindDeviceToChannel(ch, m_mapAutoCamIndex[strValue]);
                setActiveChannel(ch);
                readSysfs();
            }
        } else {
            updatePanelForActive();
        }

        showBGImg();

        if (1 == m_language) {
            m_language = 0;
            emit ui->pushButtonTrans->clicked();
        } else {
            QString strI2C = ui->lineEditI2cBus->text();
            if (QxHelp::hasWritePermission(strI2C.toStdString().c_str())) {
                emit ui->pushButtonGetCamParam->clicked();
            }
        }
    });

    // 按 16:9 适配当前屏幕可用区域，避免窗口比例不对/被显示器裁切（显示不全）
    {
        QScreen *scr = QApplication::primaryScreen();
        QRect av = scr ? scr->availableGeometry() : QRect(0, 0, 1920, 1080);
        int h = av.height();
        int w = h * 16 / 9;
        if (w > av.width()) {
            w = av.width();
            h = w * 9 / 16;
        }
        resize(w, h);
    }
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
        if (cur().bRunning) {
            ui->pushButtonCapture->setText(("Stop Capture"));
        }
        if (cur().bShow) {
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
        if (cur().bRunning) {
            ui->pushButtonCapture->setText(("停止采集"));
        }
        if (cur().bShow) {
            ui->pushButtonShow->setText(("停止显示"));
        }
        int index = ui->comboBox_showFPS->currentIndex();
        ui->comboBox_showFPS->clear();
        ui->comboBox_showFPS->addItem(tr("30帧/秒"), 30);
        ui->comboBox_showFPS->addItem(tr("60帧/秒"), 60);
        ui->comboBox_showFPS->setCurrentIndex(index);
    }
    // 布局按钮文本随当前画面模式动态变化，retranslateUi 只会还原成 .ui 里的默认值，
    // 这里按模式重设，避免切换语言后按钮显示与实际情况不符。
    ui->pushButtonLayout->setText(m_bFourView ? tr("单画面") : tr("四画面"));
    QxHelp::setWindowIdentity(this, APP_NAME);
}

void MainWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (m_channels[i].view && m_channels[i].view->isVisible()) {
            m_channels[i].gvSize = m_channels[i].view->size();
        }
    }
    if (!cur().bRunning) {
        showBGImg();
    }
}

bool MainWidget::eventFilter(QObject *watched, QEvent *event)
{
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (watched != m_channels[i].view) {
            continue;
        }
        QEvent::Type et = event->type();
        if (et == QEvent::Resize) {
            m_channels[i].gvSize = m_channels[i].view->size();
        } else if (et == QEvent::MouseButtonPress) {
            // 单击某个画面 -> 激活该通道（允许空格，便于“先选中空格再放相机”）
            if (m_active != i) {
                setActiveChannel(i);
                // 已绑定相机时再刷新右侧参数到该通道
                if (m_channels[i].used) {
                    QString strI2C = ui->lineEditI2cBus->text();
                    if (QxHelp::hasWritePermission(strI2C.toStdString().c_str())) {
                        emit ui->pushButtonGetCamParam->clicked();
                    }
                }
            }
        } else if (et == QEvent::MouseButtonDblClick) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (m_fs != FS_None) {
                    exitFullscreen();
                } else if (m_channels[i].used) {
                    enterFullscreenSingle(i);
                }
                return true; // 已处理
            }
        }
        break;
    }
    return FramelessWidget::eventFilter(watched, event);
}

void MainWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && m_fs != FS_None) {
        exitFullscreen();
        return;
    }
    FramelessWidget::keyPressEvent(event);
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
        // 只有“不可写”才需要回退到默认目录（此处原先判断写反了，导致能用的路径反而被丢弃）
        if (!QxHelp::hasWritePermission(savePath.toStdString().c_str())) {
            savePath = QApplication::applicationDirPath() + "/image";
        }
        // 默认目录可能不存在（刚安装/手动删过），不建的话存图会静默失败
        QDir dir(savePath);
        if (!dir.exists()) {
            dir.mkpath(savePath);
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
    if (cur().bRunning) {
        QxToast::showTip(tr("状态为采集中，不支持该操作"), this);
    } else {
        QString strX = info.strX.trimmed();
        QString strY = info.strY.trimmed();
        QString strW = info.strW.trimmed();
        QString strH = info.strH.trimmed();
        QString strFps = info.strFps.trimmed();

        // 这些字符串会直接拼进 v4l2-ctl / media-ctl / config_camera-rpi5.sh 的参数里，
        // 对话框只判了“非空”，这里做严格的数字格式+范围校验（同时拼不上脏字符）。
        static const QRegularExpression reInt("^[0-9]{1,5}$");
        static const QRegularExpression reFps("^[0-9]{1,3}([.][0-9]{1,3})?$");
        const int nROIVerify = strW.toInt();
        const int nROIHVerify = strH.toInt();
        if (!reInt.match(strX).hasMatch() || !reInt.match(strY).hasMatch()
            || !reInt.match(strW).hasMatch() || !reInt.match(strH).hasMatch()
            || !reFps.match(strFps).hasMatch() || nROIVerify <= 0 || nROIHVerify <= 0
            || nROIVerify > 8192 || nROIHVerify > 8192) {
            QxToast::showTip(tr("请设置合理的ROI和帧率"), this);
            return;
        }
        strX = QString::number(strX.toInt());
        strY = QString::number(strY.toInt());
        strW = QString::number(nROIVerify);
        strH = QString::number(nROIHVerify);

        QString videoNode = ui->lineEditVideoNode->text().trimmed();
        QString i2cBus = ui->lineEditI2cBus->text().trimmed();
        QString mediaNode = ui->lineEditMediaNode->text().trimmed();
        QString subNode = ui->lineEditSubNode->text().trimmed();
        QString entityName = ui->lineEditEntityName->text().trimmed();

        // 这些控件不是只读的，用户（或被改过的 auto_camera_index.json）可以直接填任意文本；
        // 命令参数来到的字符串先过一遍白名单，不干净就拒绝下发。
        if (!QxHelp::isSafeCmdToken(videoNode) || !QxHelp::isSafeCmdToken(mediaNode)
            || !QxHelp::isSafeCmdToken(subNode) || !QxHelp::isSafeCmdToken(entityName)) {
            qCritical() << "reject set-roi with unsafe device strings";
            QxToast::showTip(tr("设备节点/实体名不合法"), this);
            return;
        }

        int nPos = i2cBus.lastIndexOf("-") + 1;
        i2cBus = i2cBus.mid(nPos, i2cBus.length() - nPos);
        //QString addr = m_i2cAddr.mid(2, m_i2cAddr.length() - 2);

        Camera *cam = cur().cam;
        if (!cam) {
            return;
        }
        cur().camParam.videoNode = videoNode;
        cur().camParam.subNode = subNode;
        cur().camParam.x = strX.toInt();
        cur().camParam.y = strY.toInt();
        cur().camParam.w = strW.toInt();
        cur().camParam.h = strH.toInt();
        cur().camParam.fps = strFps.toInt();

        QString strCmd;
        switch (m_euPlatform) {
        case EuPlatform::Rockchip: {
            if (EuCamType::MvCamera == m_euCamType) {
                int nFps = strFps.toFloat() * 100;
                strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_x=%2").arg(subNode, strX);
                cam->runSystemCmd(strCmd);
                strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_y=%2").arg(subNode, strY);
                cam->runSystemCmd(strCmd);
                strCmd = QString("media-ctl -d %1 --set-v4l2 \'\"%2\":0[fmt:%3/%4x%5@1/%6]\'")
                             .arg(mediaNode,
                                  entityName,
                                  "Y8_1X8",
                                  strW,
                                  strH,
                                  QString::number(static_cast<int>(strFps.toFloat())));
                cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps, nFps, true);
            } else if (EuCamType::GxCamera == m_euCamType) {
                int nFps = strFps.toFloat() * 10000;
                if (m_gxVideoModeCap & 0x01) {
                    // GX ROI 模式：设置 ROI x/y
                    strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_x=%2").arg(subNode, strX);
                    cam->runSystemCmd(strCmd);
                    strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_y=%2").arg(subNode, strY);
                    cam->runSystemCmd(strCmd);
                }
                strCmd = QString("media-ctl -d %1 --set-v4l2 \'\"%2\":0[fmt:%3/%4x%5@1/%6]\'")
                             .arg(mediaNode,
                                  entityName,
                                  "UYVY8_2X8",
                                  strW,
                                  strH,
                                  QString::number(static_cast<int>(strFps.toFloat())));
                cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps_gx, nFps, true);
                qDebug() << strCmd;
            }
        } break;
        case EuPlatform::RaspberryPi: {
            if (EuCamType::MvCamera == m_euCamType) {
                int nFps = strFps.toFloat() * 100;
                strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_x=%2").arg(videoNode, strX);
                cam->runSystemCmd(strCmd);
                strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_y=%2").arg(videoNode, strY);
                cam->runSystemCmd(strCmd);
                strCmd = QString(
                             "v4l2-ctl -d %1 --set-fmt-video=width=%2,height=%3,pixelformat=GREY")
                             .arg(videoNode, strW, strH);
                cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps, nFps, true);
            } else if (EuCamType::GxCamera == m_euCamType) {
                int nFps = strFps.toFloat() * 10000;
                if (m_gxVideoModeCap & 0x01) {
                    // GX ROI 模式：设置 ROI x/y
                    strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_x=%2").arg(videoNode, strX);
                    cam->runSystemCmd(strCmd);
                    strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_y=%2").arg(videoNode, strY);
                    cam->runSystemCmd(strCmd);
                }
                strCmd = QString(
                             "v4l2-ctl -d %1 --set-fmt-video=width=%2,height=%3,pixelformat=UYVY")
                             .arg(videoNode, strW, strH);
                cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps_gx, nFps, true);
            }
        } break;
        case EuPlatform::RaspberryPi5: {
            if (EuCamType::MvCamera == m_euCamType) {
                int nFps = strFps.toFloat() * 100;
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
                cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps, nFps, true);
            } else if (EuCamType::GxCamera == m_euCamType) {
                int nFps = strFps.toFloat() * 10000;
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
                cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps_gx, nFps, true);
            }
        } break;
        case EuPlatform::Jetson: {
            if (EuCamType::MvCamera == m_euCamType) {
                int nFps = strFps.toFloat() * 100;
                strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_x=%2").arg(videoNode, strX);
                cam->runSystemCmd(strCmd);
                strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_y=%2").arg(videoNode, strY);
                cam->runSystemCmd(strCmd);
                strCmd = QString(
                             "v4l2-ctl -d %1 --set-fmt-video=width=%2,height=%3,pixelformat=GREY")
                             .arg(videoNode, strW, strH);
                cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps, nFps, true);
            } else if (EuCamType::GxCamera == m_euCamType) {
                int nFps = strFps.toFloat() * 10000;
                if (m_gxVideoModeCap & 0x01) {
                    // GX ROI 模式：设置 ROI x/y
                    strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_x=%2").arg(videoNode, strX);
                    cam->runSystemCmd(strCmd);
                    strCmd = QString("v4l2-ctl -d %1 --set-ctrl roi_y=%2").arg(videoNode, strY);
                    cam->runSystemCmd(strCmd);
                }
                strCmd = QString(
                             "v4l2-ctl -d %1 --set-fmt-video=width=%2,height=%3,pixelformat=UYVY")
                             .arg(videoNode, strW, strH);
                cam->runSystemCmd(strCmd);
                asyncWriteCmd(fps_gx, nFps, true);
            }
            int width = strW.toInt();
            if (0 != width % 256) {
                width = (width / 256 + 1) * 256;
            }
            strCmd
                = QString("v4l2-ctl -d %1 --set-ctrl preferred_stride=%2").arg(videoNode).arg(width);
            cam->runSystemCmd(strCmd);
        } break;
        default:
            break;
        }
        if (info.read) {
            getImgAttr();        
        }
    }
}

void MainWidget::onCamStatus(int ch, EuCamStatus status)
{
    if (ch < 0 || ch >= MAX_CHANNELS) {
        return;
    }
    CameraChannel &c = m_channels[ch];
    switch (status) {
    case EuCamStatus::OPEN_STA: {
        c.bRunning = true;
        if (ch == m_active) {
            ui->pushButtonSaveImg->setEnabled(true);
            ui->pushButtonCapture->setText(tr("停止采集"));
            ui->pushButtonCapture->setChecked(true);
            ui->pushButtonShow->setEnabled(true);
        }
    } break;
    case EuCamStatus::CLOSE_STA: {
        c.bRunning = false;
        c.bShow = false;
        rcvFrameInfo(ch, StCamInfo());
        showChannelBg(ch);
        if (ch == m_active) {
            ui->pushButtonSaveImg->setEnabled(false);
            ui->pushButtonCapture->setText(tr("开始采集"));
            ui->pushButtonCapture->setChecked(false);
            ui->pushButtonShow->setText(tr("开始显示"));
            ui->pushButtonShow->setChecked(false);
            ui->pushButtonShow->setEnabled(false);
        }
    } break;
    case EuCamStatus::HIDE_STA: {
        showChannelBg(ch);
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
    CameraChannel &c = cur();
    if (!c.bRunning) {
        qDebug() << ".........openCam.........";
        ensureChannelCamera(m_active);
        emit sndSetROI(false);
        QString videoNode = ui->lineEditVideoNode->text();
        Camera *cam = c.cam;
        if (!cam) {
            return;
        }
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
            cam->runSystemCmd(strCmd);
        } break;
        default:
            break;
        };

        c.bRunning = cam->OpenDevice(c.camParam);
        if (c.bRunning) {
            onCamStatus(c.index, EuCamStatus::OPEN_STA);
        } else {
            onCamStatus(c.index, EuCamStatus::CLOSE_STA);
        }
    }
}

void MainWidget::closeCam()
{
    CameraChannel &c = cur();
    Camera *cam = c.cam;
    if (cam && c.bRunning) {
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
            cam->runSystemCmd(strCmd);
        } break;
        default:
            break;
        }
        cam->CloseDevice();
        c.bRunning = false;
        c.bShow = false;
        ui->pushButtonCapture->setChecked(false);
        closeShow();
    }
}

void MainWidget::openShow()
{
    m_showFps = 0;
    m_seconds = 0;
    m_bFps = false;
    CameraChannel &c = cur();
    c.bShow = true;
    if (c.cam) {
        c.cam->setShow(true);
    }
    ui->pushButtonShow->setText(tr("停止显示"));
}

void MainWidget::closeShow()
{
    CameraChannel &c = cur();
    c.bShow = false;
    if (c.cam) {
        c.cam->setShow(false);
    }
    ui->pushButtonShow->setText(tr("开始显示"));
    ui->pushButtonShow->setChecked(false);
    showChannelBg(c.index);
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
    // 用 value()+contains()：非 const 的 operator[] 会把未知平台写进 map（下次拿到空字符串），
    // 导致把“配置目录”本身当成脚本交给 shell 执行。
    if (!mapProbefile.contains(m_platform) || mapProbefile.value(m_platform).isEmpty()) {
        qCritical() << "no probe script mapped for platform" << m_platform;
        QxToast::showTip(tr("未知平台，无法检索相机") + ": " + m_platform, this);
        return;
    }
    const QString strScript = m_strCfgPath + mapProbefile.value(m_platform);
    if (!QFile::exists(strScript)) {
        qCritical() << "probe script not found:" << strScript;
        QxToast::showTip(tr("检索脚本不存在") + ": " + strScript, this);
        return;
    }

    // 不再用 system()：直接 /bin/bash 脚本路径，路径里有空格也不会被重新分词/解析
    QProcess probe;
    probe.setProcessChannelMode(QProcess::MergedChannels);
    probe.start("/bin/bash", QStringList() << strScript);
    if (!probe.waitForStarted(3000)) {
        qCritical() << "run probe failed:" << strScript << probe.errorString();
        return;
    }
    if (!probe.waitForFinished(60000)) {
        probe.kill();
        probe.waitForFinished(1000);
        qCritical() << "probe timeout:" << strScript;
        return;
    }
    if (0 == probe.exitCode()) {
        QString strJson = m_strCfgPath + "auto_camera_index.json";
        if (QxHelp::fileExists(strJson.toStdString().c_str())) {
            Json::Value jsonRoot;
            if (QxHelp::readJson(strJson, jsonRoot)) {
                int total = jsonRoot.size();
                for (int i = 0; i < total; ++i) {
                    Json::Value jsonNode = jsonRoot[i];
                    StCamIndex camIndex;
                    QString i2c_bus_num = QString::fromStdString(jsonNode["i2c_bus"].asString());
                    bool busOk = false;
                    // 探测脚本写的是裸数字；非数字（包括被篡改的 JSON）一律丢弃，
                    // 后面的 set-v4l2 / media-ctl 命令不会拼到脏字符串
                    const int busNumber = i2c_bus_num.trimmed().toInt(&busOk);
                    if (!busOk || busNumber < 0) {
                        qWarning() << "skip camera entry with invalid i2c bus:" << i2c_bus_num;
                        continue;
                    }
                    camIndex.i2c_bus = QString("/dev/i2c-%1").arg(busNumber);
                    camIndex.i2c_bus_num = busNumber;
                    camIndex.media_entity_name = jsonNode["media_entity_name"].asString().c_str();
                    camIndex.media_node = jsonNode["media_node"].asString().c_str();
                    camIndex.video_node = jsonNode["video_node"].asString().c_str();
                    camIndex.video_subnode = jsonNode["video_subnode"].asString().c_str();
                    // 设备名字段会被拼进 v4l2-ctl / media-ctl 命令，先做长度与控制字符校验
                    if (!QxHelp::isSafeCmdToken(camIndex.media_entity_name)
                        || !QxHelp::isSafeCmdToken(camIndex.media_node)
                        || !QxHelp::isSafeCmdToken(camIndex.video_subnode)
                        || !camIndex.video_node.startsWith("/dev/video")
                        || !QxHelp::isSafeCmdToken(camIndex.video_node)) {
                        qWarning() << "skip camera entry index" << i << "with suspicious device strings";
                        continue;
                    }
                    ui->comboBoxDev->addItem(camIndex.video_node);
                    m_mapAutoCamIndex.insert(camIndex.video_node, camIndex);
                    if (i2c_bus.isEmpty() && !i2c_bus_num.isEmpty()) {
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
        qCritical() << "runShell " << strScript << " failed, code" << probe.exitCode();
    }

    if (!i2c_bus.isEmpty()) {
        setSysfsPath(i2c_bus);
    } else {
        qWarning() << "i2cBus is empty, readCamType failed";
    }
}

bool MainWidget::setSysfsPath(const QString &i2cBus)
{
    bool validBus = false;
    const int busNumber = i2cBus.section('-', -1).toInt(&validBus);
    if (!validBus) {
        qWarning() << "Invalid I2C bus:" << i2cBus;
        m_sysfsPath.clear();
        return false;
    }

    const QString basePath =
        QString("/sys/bus/i2c/devices/i2c-%1/%1-003b/").arg(busNumber);
    const QString mvPath = basePath + "veye_mvcam/";
    if (QxHelp::isDirectoryExists(mvPath)) {
        m_sysfsPath = mvPath;
        m_euCamType = EuCamType::MvCamera;
        return true;
    }

    const QString gxPath = basePath + "veye_gxcam/";
    if (QxHelp::isDirectoryExists(gxPath)) {
        m_sysfsPath = gxPath;
        m_euCamType = EuCamType::GxCamera;
        return true;
    }

    qWarning() << "No camera sysfs directory found for I2C bus" << busNumber;
    m_sysfsPath.clear();
    return false;
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
        // 先读取 VideoModeCap 判断模式
        uint32_t cap = readCmd(videomodecap, this, true, false); // 同步读取
        m_gxVideoModeCap = cap;
        bool bRoiMode = (cap & 0x01); // Bit0=1 → ROI
        m_pImgAttr->setGxMode(bRoiMode);

        if (bRoiMode) {
            // ROI 模式：读取 ROI 参数
            readCmd(roi_x, m_pImgAttr);
            readCmd(roi_y, m_pImgAttr);
            readCmd(roi_w, m_pImgAttr);
            readCmd(roi_h, m_pImgAttr);
            readCmd(fps_gx, m_pImgAttr);
        } else {
            // VideoMode 模式：当前逻辑
            readCmd(videomodenum, m_pImgAttr);
            readCmd(fps_gx, m_pImgAttr);
        }
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
    Camera *cam = cur().cam;
    if (!i2c.isEmpty() && cam) {
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
            cam->asynReadData(stCmd);
        } else {
            cam->syncReadData(stCmd);
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
        if (Standby == type && cur().bRunning) {
            QxToast::showTip(tr("状态为采集中，不支持该操作"), this);
            return false;
        } else if (ReadOnly == type) {
            QxToast::showTip(tr("只读寄存器,不支持该操作"), this);
            return false;
        }
    }

    QString i2cBus = ui->lineEditI2cBus->text();
    if (!i2cBus.isEmpty() && cur().cam) {
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
        cur().cam->asynWriteDate(stCmd);
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
    if (camIndex.i2c_bus_num > 0) {
        setSysfsPath(QString::number(camIndex.i2c_bus_num));
    }
    QString model = "";
    if (!m_sysfsPath.isEmpty()) {
        QxHelp::readFirstLine(m_sysfsPath + "camera_model", model);
    }
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

void MainWidget::showChannelBg(int ch)
{
    if (ch < 0 || ch >= MAX_CHANNELS) {
        return;
    }
    CameraChannel &c = m_channels[ch];
    if (!c.view || !c.scene || !c.pixItem) {
        return;
    }
    // 不连相机/未在显示时不再铺 logo（放大后难看），仅保留纯色背景
    c.pixItem->setPixmap(QPixmap());
    c.scene->setSceneRect(0, 0, c.view->size().width(), c.view->size().height());
}

void MainWidget::showBGImg()
{
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (m_channels[i].view && m_channels[i].view->isVisible() && !m_channels[i].bShow) {
            showChannelBg(i);
        }
    }
    if (!cur().bShow) {
        ui->labelTimestamp->setText("0");
        ui->label_showFps->setText("0");
    }
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
    int ch = sii.index;
    if (ch < 0 || ch >= MAX_CHANNELS) {
        ch = m_active;
    }
    CameraChannel &c = m_channels[ch];
    if (c.bShow) {
        std::lock_guard<std::mutex> lk(c.mtxPix);
        c.queueSii.push(sii);
        int size = (int) c.queueSii.size();
        if (size > m_nChachFrameSize) {
            StImgInfo tmp = c.queueSii.front();
            c.queueSii.pop();
            if (new_buf == tmp.strategy) {
                delete[] tmp.buf;
            }
            //qDebug() << "cache frame size " << size << " > " << m_nChachFrameSize << ", drop frame";
        }
    } else {
        if (new_buf == sii.strategy) {
            delete[] sii.buf;
        }
        if (ch == m_active) {
            m_seconds = 0;
            m_showFps = 0;
        }
    }
}

bool MainWidget::isCamRunning()
{
    return cur().bRunning;
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
            Y0 = qBound(Y_MIN, (int) Y0, Y_MAX);
            Y1 = qBound(Y_MIN, (int) Y1, Y_MAX);
            U = qBound(UV_MIN, (int) U, UV_MAX);
            V = qBound(UV_MIN, (int) V, UV_MAX);

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
    bool warnedUnknown[MAX_CHANNELS] = {false};

    while (m_bThread) {
        // 下拉框某一项没带 data 时 toInt() 会得到 0，1000/0 = inf 会让预览永远不刷新
        const int fps = m_setFps > 0 ? m_setFps : 30;
        double setFrameIntv = 1000.0 / fps * 1000;
        if (timer.nsecsElapsed() / 1000 >= setFrameIntv) {
            //qDebug() << setFrameIntv << timer.nsecsElapsed() / 1000;
            timer.restart();
            for (int ch = 0; ch < MAX_CHANNELS; ++ch) {
                CameraChannel &c = m_channels[ch];
                StImgInfo sii;
                bool got = false;
                {
                    std::lock_guard<std::mutex> lk(c.mtxPix);
                    if (!c.queueSii.empty()) {
                        sii = c.queueSii.front();
                        c.queueSii.pop();
                        got = true;
                    }
                }
                if (!got) {
                    continue;
                }
                if (c.bShow) {
                    QImage img;
                    if (EuCamType::GxCamera == c.camType) {
                        img = uyvyToQImage(sii.buf,
                                           sii.len,
                                           sii.width,
                                           sii.height,
                                           sii.nBytesperline);
                    } else if (EuCamType::MvCamera == c.camType) {
                        img = QImage(sii.buf,
                                     sii.nBytesperline,
                                     sii.height,
                                     QImage::Format_Grayscale8);
                    } else {
                        // 类型未知：两种格式的字节含义完全不同，当作 GRAY8 只会解出花屏，
                        // 宁可不显示并留一条日志。
                        if (!warnedUnknown[ch]) {
                            warnedUnknown[ch] = true;
                            qWarning() << "channel" << ch << "cam type unknown, frame dropped";
                        }
                        if (new_buf == sii.strategy) {
                            delete[] sii.buf;
                        }
                        continue;
                    }
                    QPixmap pix;
                    if (m_scale) {
                        pix = QPixmap::fromImage(img).scaled(c.gvSize,
                                                             Qt::KeepAspectRatio,
                                                             Qt::FastTransformation);
                    } else {
                        pix = QPixmap::fromImage(img);
                    }
                    emit sndPixmap(ch, pix, sii.strTS);
                }
                if (new_buf == sii.strategy) {
                    delete[] sii.buf;
                }
            }
        } else {
            // 睡到下一个出帧节拍（最多 5ms 一次，保证停止时及时退出），
            // 而不是原来 1us 的忙等（四路时会白白吃掉一颗核）
            qint64 waitUs = (qint64) setFrameIntv - timer.nsecsElapsed() / 1000;
            if (waitUs < 200) {
                waitUs = 200;
            } else if (waitUs > 5000) {
                waitUs = 5000;
            }
            QThread::usleep((unsigned long) waitUs);
        }
    }
}

void MainWidget::rcvPixmap(int ch, const QPixmap &pix, QString strTS)
{
    if (ch < 0 || ch >= MAX_CHANNELS) {
        return;
    }
    CameraChannel &c = m_channels[ch];
    if (c.bShow && c.pixItem && c.scene) {
        if (ch == m_active) {
            m_lblFps++;
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
        }
        c.pixItem->setPixmap(pix);
        c.scene->setSceneRect(0, 0, pix.width(), pix.height());
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
    if (fps == cmd || fps_gx == cmd) {
        qInfo() << "............" << data;
    }
    if (factoryparam == cmd) {
        if (cur().bRunning) {
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
        asyncWriteCmd(expmode, 0);
        asyncWriteCmd(gainmode, 0);
        asyncWriteCmd(metime, expValue);
        asyncWriteCmd(mgain, gainValue);
        cmdSleep(500);
        //[step.6]:.check.resutlt
        readCmd(expmode, m_pExpAndGain);
        readCmd(gainmode, m_pExpAndGain);
        //emit sndExpGainMode(expMode, gainMode);
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
    if (cur().bRunning) {
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

void MainWidget::rcvFrameInfo(int ch, const StCamInfo &info)
{
    if (ch == m_active) {
        ui->labelCaptureFps->setText(QString::number(info.fps, 'f', 2));
        ui->labelRES->setText(QString("%1 ∗ %2").arg(info.width).arg(info.height));
    }
}

// ===================== 多相机通道管理 =====================
CameraChannel &MainWidget::cur()
{
    if (m_active < 0 || m_active >= MAX_CHANNELS) {
        m_active = 0;
    }
    return m_channels[m_active];
}

void MainWidget::ensureChannelCamera(int ch)
{
    if (ch < 0 || ch >= MAX_CHANNELS) {
        return;
    }
    CameraChannel &c = m_channels[ch];
    if (!c.cam) {
        CommDevCfg devCfg;
        devCfg.cameType = c.camType;
        devCfg.platform = m_euPlatform;
        devCfg.channel = ch;
        c.cam = new Camera(devCfg, this);
        connect(c.cam, &Camera::sndOptLog, this, &MainWidget::rcvOptLog);
        connect(c.cam, &Camera::sndCmdRet, this, &MainWidget::rcvCmdRet);
    }
}

int MainWidget::pickChannelForDevice(const QString &videoNode)
{
    // 已绑定该设备的通道直接复用
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (m_channels[i].used && m_channels[i].camIndex.video_node == videoNode) {
            return i;
        }
    }
    // 否则绑定到“当前激活画面”：先点哪个格子，设备就进哪个格子
    return m_active;
}

void MainWidget::bindDeviceToChannel(int ch, const StCamIndex &idx)
{
    if (ch < 0 || ch >= MAX_CHANNELS) {
        return;
    }
    CameraChannel &c = m_channels[ch];
    // 目标格上挂的是另一台设备（或残留的旧 Camera 对象）时，先完整拆除：
    // 停采 + 排空帧队列（否则残留帧的 new[] 缓存永久泄漏）+ 销毁旧 Camera，
    // 避免沿用上一次的 Capture/寄存器/设备类型状态。
    if (c.camIndex.video_node != idx.video_node) {
        teardownChannel(ch);
    }
    c.camIndex = idx;
    c.used = true;
    c.camParam = StCamParam();
    c.camParam.videoNode = idx.video_node;
    c.camParam.subNode = idx.video_subnode;
}

void MainWidget::teardownChannel(int ch, bool bImmediate)
{
    if (ch < 0 || ch >= MAX_CHANNELS) {
        return;
    }
    CameraChannel &c = m_channels[ch];
    Camera *cam = c.cam;
    c.cam = nullptr;
    if (cam) {
        if (c.bRunning || cam->IsOpen()) {
            cam->CloseDevice();
        }
        c.bRunning = false;
        c.bShow = false;
        if (bImmediate) {
            // 退出阶段：事件循环已停，必须同步销毁，~Camera 才会把两个线程 join 掉
            delete cam;
        } else {
            cam->disconnect(this);
            cam->deleteLater();
        }
    }
    // 排空未消费的帧，每个 StImgInfo.buf 都是一次 new[]
    {
        std::lock_guard<std::mutex> lk(c.mtxPix);
        while (!c.queueSii.empty()) {
            StImgInfo tmp = c.queueSii.front();
            c.queueSii.pop();
            if (new_buf == tmp.strategy) {
                delete[] tmp.buf;
            }
        }
    }
    c.used = false;
    c.bShow = false;
    c.bRunning = false;
    c.camIndex = StCamIndex();
    c.camType = EuCamType::Unknown;
}

void MainWidget::pushDevCfgToDialogs(int ch)
{
    // 对话框在构造时只拿到一次 devCfg 快照，之后切换通道/探测出 GX 类型并不会自动更新，
    // 导致 GX 相机一直走 MV 分支（面板、寄存器命令格式全部错）。这里主动下发。
    CommDevCfg devCfg = {m_channels[ch].camType, m_euPlatform, ch};
    if (m_pImgAttr) {
        m_pImgAttr->setDevCfg(devCfg);
    }
    if (m_pExpAndGain) {
        m_pExpAndGain->setDevCfg(devCfg);
    }
    if (m_pTrigger) {
        m_pTrigger->setDevCfg(devCfg);
    }
    if (m_pCurAttr) {
        m_pCurAttr->setDevCfg(devCfg);
    }
    if (m_pWhiteBlance) {
        m_pWhiteBlance->setDevCfg(devCfg);
    }
    if (m_pImgProc) {
        m_pImgProc->setDevCfg(devCfg);
    }
    if (m_pRegister) {
        m_pRegister->setDevCfg(devCfg);
    }
}

void MainWidget::setActiveChannel(int ch)
{
    if (ch < 0 || ch >= MAX_CHANNELS) {
        return;
    }
    m_active = ch;
    CameraChannel &c = m_channels[ch];

    if (c.used) {
        // 先置为未知：探测失败时不会沿用上一个通道的类型（否则会用错寄存器表/错解码格式）
        m_euCamType = EuCamType::Unknown;
        setCamIndex(c.camIndex); // 写入只读 lineEdit 镜像 + 探测类型(m_euCamType/m_sysfsPath)
        c.camType = m_euCamType;
        if (EuCamType::Unknown != c.camType) {
            adjustAdaptive(); // 依据类型设置 m_register
            c.registerJson = m_register;
            readRegCfg();     // 重新加载寄存器表
            if (m_pRegister) {
                m_pRegister->setCfg(m_mapNameRegister, m_mapAddrRegister);
            }
        } else {
            qWarning() << "channel" << ch << "camera type unknown, keep old register config";
        }
        ensureChannelCamera(ch);
    } else {
        ui->lineEditEntityName->setText("");
        ui->lineEditI2cBus->setText("");
        ui->lineEditVideoNode->setText("");
        ui->lineEditSubNode->setText("");
        ui->lineEditMediaNode->setText("");
        ui->label_model->setText("");
        m_sysfsPath.clear();
    }

    pushDevCfgToDialogs(ch);
    updatePanelForActive();
    applyViewLayout();
}

void MainWidget::updatePanelForActive()
{
    if (EuCamType::MvCamera == m_euCamType) {
        ui->pushButtonImgProc->hide();
        ui->pushButtonWhiteBlance->hide();
    } else {
        ui->pushButtonImgProc->show();
        ui->pushButtonWhiteBlance->show();
    }
    syncButtonsForActive();
}

void MainWidget::syncButtonsForActive()
{
    CameraChannel &c = cur();
    if (c.bRunning) {
        ui->pushButtonCapture->setText(tr("停止采集"));
        ui->pushButtonCapture->setChecked(true);
        ui->pushButtonShow->setEnabled(true);
        ui->pushButtonSaveImg->setEnabled(true);
    } else {
        ui->pushButtonCapture->setText(tr("开始采集"));
        ui->pushButtonCapture->setChecked(false);
        ui->pushButtonShow->setEnabled(false);
        ui->pushButtonSaveImg->setEnabled(false);
    }
    if (c.bShow) {
        ui->pushButtonShow->setText(tr("停止显示"));
        ui->pushButtonShow->setChecked(true);
    } else {
        ui->pushButtonShow->setText(tr("开始显示"));
        ui->pushButtonShow->setChecked(false);
    }
}

void MainWidget::applyViewLayout()
{
    QGridLayout *g = ui->gridLayoutViews;
    if (!g) {
        return;
    }
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (m_channels[i].view) {
            g->removeWidget(m_channels[i].view);
        }
    }

    if (m_bFourView) {
        const int rows[MAX_CHANNELS] = {0, 0, 1, 1};
        const int cols[MAX_CHANNELS] = {0, 1, 0, 1};
        for (int i = 0; i < MAX_CHANNELS; ++i) {
            QGraphicsView *v = m_channels[i].view;
            if (!v) {
                continue;
            }
            g->addWidget(v, rows[i], cols[i]);
            v->setVisible(true);
        }
    } else {
        for (int i = 0; i < MAX_CHANNELS; ++i) {
            if (m_channels[i].view && i != m_active) {
                m_channels[i].view->setVisible(false);
            }
        }
        QGraphicsView *v = m_channels[m_active].view;
        if (v) {
            g->addWidget(v, 0, 0, 2, 2);
            v->setVisible(true);
        }
    }

    for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (m_channels[i].view && m_channels[i].view->isVisible()) {
            m_channels[i].gvSize = m_channels[i].view->size();
        }
    }
    // 未显示(未绑定/未开启显示)的可见通道铺背景图
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (m_channels[i].view && m_channels[i].view->isVisible() && !m_channels[i].bShow) {
            showChannelBg(i);
        }
    }
    // 高亮当前激活画面：四画面下用醒目边框凸显选中状态（单画面/全屏不画框）
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (!m_channels[i].view) {
            continue;
        }
        if (!m_bFourView) {
            m_channels[i].view->setStyleSheet(QString());
        } else if (i == m_active) {
            m_channels[i].view->setStyleSheet("QGraphicsView { border: 3px solid #000000; }");
        } else {
            m_channels[i].view->setStyleSheet("QGraphicsView { border: 1px solid #6f6f6f; }");
        }
    }
}

void MainWidget::toggleViewLayout()
{
    m_bFourView = !m_bFourView;
    applyViewLayout();
    ui->pushButtonLayout->setText(m_bFourView ? tr("单画面") : tr("四画面"));
}

// ===================== 全屏交互 =====================
void MainWidget::setChromeVisible(bool visible)
{
    ui->widgetTitle->setVisible(visible);
    ui->widgetLeft->setVisible(visible);
    ui->widgetRight->setVisible(visible);
    ui->widgetBottom->setVisible(visible);
    ui->widgetstatus->setVisible(visible);
}

void MainWidget::enterFullscreenSingle(int ch)
{
    if (ch < 0 || ch >= MAX_CHANNELS) {
        return;
    }
    m_prevGeom = saveGeometry();
    m_prevMax = isMaximized();
    m_prevFourView = m_bFourView;
    m_bFourView = false;
    ui->pushButtonLayout->setText(tr("四画面"));
    setActiveChannel(ch);
    setChromeVisible(false);
    showFullScreen();
    m_fs = FS_Single;
}

void MainWidget::enterFullscreenQuad()
{
    m_prevGeom = saveGeometry();
    m_prevMax = isMaximized();
    m_prevFourView = m_bFourView;
    m_bFourView = true;
    ui->pushButtonLayout->setText(tr("单画面"));
    applyViewLayout();
    setChromeVisible(false);
    showFullScreen();
    m_fs = FS_Quad;
}

void MainWidget::exitFullscreen()
{
    if (m_fs == FS_None) {
        return;
    }
    const bool bWasMax = m_prevMax;
    setChromeVisible(true);
    showNormal();
    if (!m_prevGeom.isEmpty()) {
        restoreGeometry(m_prevGeom);
    }
    // 进入前是最大化窗口时，restoreGeometry 不一定会把最大化状态带回来，显式还原
    if (bWasMax) {
        showMaximized();
    }
    m_fs = FS_None;
    m_prevGeom.clear();
    m_prevMax = false;
    // 回到进入全屏前的布局：退出全屏不该把用户原来的四画面布局弄没了
    m_bFourView = m_prevFourView;
    ui->pushButtonLayout->setText(m_bFourView ? tr("单画面") : tr("四画面"));
    applyViewLayout();
}

void MainWidget::toggleFullscreen()
{
    if (m_fs != FS_None) {
        exitFullscreen();
    } else if (m_bFourView) {
        enterFullscreenQuad();
    } else {
        enterFullscreenSingle(m_active);
    }
}
