#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QElapsedTimer>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QTranslator>
#include <QWidget>
#include "Camera.h"
#include "CommHeader.h"
#include "CurAttrDlg.h"
#include "ExpGainDlg.h"
#include "FramelessWidget.h"
#include "ImgAttrDlg.h"
#include "ImgProcDlg.h"
#include "RegisterDlg.h"
#include "TriggerDlg.h"
#include "UYVYRender.h"
#include "WhiteBlanceDlg.h"
#include "json/json.h"
#include <atomic>
#include <mutex>
#include <queue>
#include <thread>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWidget;
}
QT_END_NAMESPACE

// 单路相机通道：拥有独立的 Camera/Capture、帧队列与渲染视图
struct CameraChannel {
    int index = 0;
    bool used = false;     // 是否已绑定设备
    bool bRunning = false; // 采集中
    bool bShow = false;    // 显示中

    EuCamType camType = EuCamType::Unknown; // 激活该通道时探测得到，未知时不能渲染
    QString registerJson = "mv_register.json";
    StCamIndex camIndex;
    StCamParam camParam;

    Camera *cam = nullptr; // 每通道独立 Camera(含 Capture + i2c cmd 线程)

    // 渲染
    QGraphicsView *view = nullptr;
    QGraphicsScene *scene = nullptr;
    QGraphicsPixmapItem *pixItem = nullptr;
    QSize gvSize; // 该 view 当前尺寸(用于缩放)

    // 帧队列
    std::mutex mtxPix;
    std::queue<StImgInfo> queueSii;
};

class MainWidget : public FramelessWidget
{
    Q_OBJECT
public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

public:
    void pushData(const StImgInfo &sii);
    bool isCamRunning();
public slots:
    void onTitleDblClick();
    void rcvFrameInfo(int ch, const StCamInfo &info);
    void rcvOptLog(int ret, QString strMsg, bool bShow = true);
    void rcvCmdRet(const StCmdRet &ret);
    void rcvPixmap(int ch, const QPixmap &pix, QString strTS);
    void rcvSaveFinish(bool bFalg);
    void onCamStatus(int ch, EuCamStatus status);
protected slots:
    void asyncWriteCmd(EuCMD cmd, uint32_t data, bool bShow = true);
    void asyncReadCmd(EuCMD cmd, QObject *obj, bool bShow = true);

protected:
    uint32_t readCmd(EuCMD cmd, QObject *obj, bool bShow = true, bool async = true);
protected slots:
    void rcvOneAEAG(int nAemaxtime, int nAgmaxgain, int nAatarget);
    void onSetRoiAndFps(const StImgAttrInfo &info);
    void rcvReg(const StRegInfo &info, bool bWrite, bool bShow = true);
    void rcvExportReg();
    void rcvImportReg();
signals:
    void sndUpdateUI();
    void sndPixmap(int ch, const QPixmap &pix, QString strTS);

    void sndSetROI(bool read);
    void sndImgAttrCmdRet(const StCmdRet &ret);
    void sndRet2EG(EuCMD cmd, uint32_t &data);
    void sndExpAndGainCmdRet(const StCmdRet &ret);
    void sndExpGainValue(int expValue, int gainValue);
    void sndExpGainMode(int exp, int gain);
    void setRetExpAndGain(int exp, int gain, bool needCmd);
    // void sndRetTrg(EuCMD cmd, uint32_t &data);
    void sndTriggerCmdRet(const StCmdRet &ret);
    void sndRet2Reg(const QString strRet);
    void sndRegisterCmdRet(const StCmdRet &ret);
    void sndCurAttrCmdRet(const StCmdRet &ret);
    void sndWBCmdRet(const StCmdRet &ret);
    void sndImgProcCmdRet(const StCmdRet &ret);

protected:
    void initCfg();
    void initObj();
    void setupUi();
    void transUi();

protected:
    void readPlatform();
    void readCamIndex();
    bool setSysfsPath(const QString &i2cBus);
    void adjustAdaptive();
    bool readRegCfg();
    void readSysCfg();
    void saveSysCfg();
    void openCam();
    void closeCam();
    void openShow();
    void closeShow();

    // ------------------ 多相机通道管理 ------------------
    CameraChannel &cur();
    // 懒创建/确保通道相机存在(devCfg 携带通道与类型)
    void ensureChannelCamera(int ch);
    // 解除绑定：停采、排空帧队列、销毁该通道 Camera（设备变更/重新检索/退出时使用）
    // bImmediate=true 用于退出阶段（事件循环已停，deleteLater 不会被处理）
    void teardownChannel(int ch, bool bImmediate = false);
    // 把激活通道的设备类型/平台/通道号下发给各参数对话框
    void pushDevCfgToDialogs(int ch);
    // 绑定设备到指定通道(更新 camIndex/camType/registerJson 等)
    void bindDeviceToChannel(int ch, const StCamIndex &idx);
    // 设置激活通道并刷新控制面板/按钮/视图布局
    void setActiveChannel(int ch);
    // 选目标通道：已绑定该设备的通道优先；否则返回当前激活通道。
    // 这是“先点目标格再选设备”的设计（四画面下点空格即可把设备放到该格），
    // 因此不会自动挑空闲格；切换设备时若目标格已有别的设备，就地替换。
    int pickChannelForDevice(const QString &videoNode);
    // 依据布局模式(单/四画面)重排视图
    void applyViewLayout();
    // 切换单画面/四画面
    void toggleViewLayout();
    // 刷新激活通道对应的参数面板控件
    void updatePanelForActive();
    // 采集/显示按钮文本与状态同步激活通道
    void syncButtonsForActive();

    // ------------------ 全屏交互 ------------------
    void enterFullscreenSingle(int ch);
    void enterFullscreenQuad();
    void exitFullscreen();
    void toggleFullscreen();
    void setChromeVisible(bool visible);

protected:
    void readSysfs();
    void getBaseInfo();
    void getImgAttr();
    void getExpAndGain();
    void getTrigger();
    void getCurAttr();
    void getWB();
    void getImgProc();

protected:
    uint32_t readRegData(
        EuCMD cmd, uint32_t reg, QObject *obj, bool showlog = true, bool async = true);
    bool writeRegData(EuCMD cmd, uint32_t reg, uint32_t data, bool showLog = true);
    void cmdSleep(int n, bool bReal = true);
    void setCamIndex(StCamIndex camIndex);
    bool noEmpty();

    void showBGImg();
    void showChannelBg(int ch);
    void showCtrlWidget();

protected:
    void workThread();

protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;
    // 重写事件过滤器
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::MainWidget *ui;

protected:
    // 以下三个为“激活通道”镜像，供控制面板/i2c 读写使用（切换激活通道时同步）
    EuCamType m_euCamType = EuCamType::MvCamera;
    QString m_platform = "";
    EuPlatform m_euPlatform = EuPlatform::None;
    QString m_sysfsPath;
    QString m_register = "mv_register.json";

protected:
    QMap<QString, EuPlatform> mapPlatform;
    QMap<QString, QString> mapProbefile;

protected:
    ImgAttrDlg *m_pImgAttr = nullptr;
    ExpGainDlg *m_pExpAndGain = nullptr;
    TriggerDlg *m_pTrigger = nullptr;
    RegisterDlg *m_pRegister = nullptr;
    CurAttrDlg *m_pCurAttr = nullptr;
    WhiteBlanceDlg *m_pWhiteBlance = nullptr;
    ImgProcDlg *m_pImgProc = nullptr;

protected:
    // ------------------ 多相机通道数组 ------------------
    CameraChannel m_channels[MAX_CHANNELS];
    int m_active = 0;
    bool m_bFourView = false; // false=单画面, true=四画面
    enum EuFsState { FS_None, FS_Single, FS_Quad };
    EuFsState m_fs = FS_None;
    QByteArray m_prevGeom;
    bool m_prevMax = false;
    bool m_prevFourView = false; // 进入全屏前的布局模式，退出时还原

protected:
    bool m_bAuthorize = false;
    // 帧处理线程：标志必须原子可见，且必须在析构时 join
    std::atomic<bool> m_bThread{true};
    std::thread m_threadWork;
    QTranslator m_translator;
    int m_nChachFrameSize = 3;
    int m_setFps = 30;
    int m_showFps = 0;

protected:
    std::mutex mtx;
    std::condition_variable cv;

protected:
    int m_optNum = 0;
    QImage m_bkImage;
    bool m_bFirst = true;
    int m_trgoneCount = 0;
    EuCMD m_lastCmd;

protected:
    int m_language = 0;
    QString m_strCfgPath;
    Json::Value m_jsonSysCfg;
    QMap<QString, StRegInfo> m_mapNameRegister;
    QMap<uint32_t, StRegInfo> m_mapAddrRegister;
    QMap<QString, StCamIndex> m_mapAutoCamIndex;
    QMap<int, QString> m_mapCmdReg;

private:
    int m_roiX = 0;
    int m_roiY = 0;
    int m_roiW = 0;
    int m_roiH = 0;
    int m_CamFps = 0;
    uint32_t m_gxVideoModeCap = 0; // GX VideoMode capability (0x005C)
    QString m_strPwd;

    int m_lblFps = 0;
    volatile bool m_bFps = false;
    int m_seconds = 1;
    bool m_scale = true;
    QTimer *m_timer = nullptr;
    QString m_i2cAddr = "0x3b";
    std::deque<qint64> timestamps;
    //QElapsedTimer timerFps;
    QElapsedTimer timer1;

    QElapsedTimer timer3;
    //UYVYRenderer *m_renderer = nullptr;

private:
    CurPage m_curIndex = CurPage::ImgAttrDlg;
    QVector<QPushButton *> m_btnCtrls;
    QVector<QWidget *> m_winCtrls;
    bool isFullScreen = false; // 标记是否全屏
};

#endif // MAINWIDGET_H
