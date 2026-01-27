#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QElapsedTimer>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
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

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWidget;
}
QT_END_NAMESPACE

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
    void rcvFrameInfo(const StCamInfo &info);
    void rcvOptLog(int ret, QString strMsg, bool bShow = true);
    void rcvCmdRet(const StCmdRet &ret);
    void rcvPixmap(const QPixmap &pix, QString strTS);
    void rcvSaveFinish(bool bFalg);
    void onCamStatus(EuCamStatus status);
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
    void sndShow(bool bShow);
    void sndSaveImg(QString strPath, int count);
    void sndPixmap(const QPixmap &pix, QString strTS);

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
    void adjustAdaptive();
    bool readRegCfg();
    void readSysCfg();
    void saveSysCfg();
    void openCam();
    void closeCam();
    void openShow();
    void closeShow();

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
    void showCtrlWidget();

protected:
    void workThread();

protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    // 重写事件过滤器
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::MainWidget *ui;

protected:
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
    bool m_bAuthorize = false;
    bool m_bThread = true;
    QSize m_gvSize;
    QSize m_gvSrcSize;
    QTranslator m_translator;
    QGraphicsScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_pixItem = nullptr;
    std::mutex m_mtxPix;
    std::queue<StImgInfo> m_queueSii;
    int m_nChachFrameSize = 3;
    int m_setFps = 30;
    int m_showFps = 0;

protected:
    std::mutex mtx;
    std::condition_variable cv;

protected:
    int m_optNum = 0;
    StCamParam m_camParam;
    //OperateCmd *m_optCmd = nullptr;
    Camera *m_cam = nullptr;
    bool m_camRunning = false;
    bool m_camImgShow = false;
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
