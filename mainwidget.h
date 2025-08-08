#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QTimer>
#include <QWidget>
#include "camera.h"
#include "commheader.h"
#include "curattrdlg.h"
#include "expgaindlg.h"
#include "framelesswidget.h"
#include "imgattrdlg.h"
#include "json/json.h"
#include "operatecmd.h"
#include "probedevice.h"
#include "registerdlg.h"
#include "triggerdlg.h"
#include <qtranslator.h>

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
public slots:
    void rcvCaptureImg(const StImgInfo &sii);
    void rcvFrameInfo(const StCamInfo &info);
    void rcvOptLog(int nCode, QString strMsg, bool bShow = true);
    void rcvCmdRet(const StCmdRet &ret);
    void rcvPixmap(const QPixmap &pix, QString strTS);
    void rcvSaveFinish(bool bFalg);
    void rcvStatus(int status);
protected slots:
    void writeCmd(EuCMD cmd, uint32_t data, bool bShow = true);
    void readCmd(
        EuCMD cmd, QObject *obj, bool bShow = true, bool async = true, uint32_t *data = nullptr);
protected slots:
    void rcvOneAEAG(int nAemaxtime, int nAgmaxgain, int nAatarget);
    void rcvRoiAndFps(const StImgAttrInfo &info);
    void rcvReg(const StRegInfo &info, bool bWrite, bool bShow = true);
    void rcvExportReg();
    void rcvImportReg();
signals:
    void sndUpdateUI();
    void sndShowFps(int fps);
    void sndShow(bool bShow);
    void sndSaveImg(QString strPath, int count);
    void sndPixmap(const QPixmap &pix, QString strTS);

    void sndSetROI();
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

protected:
    void initObj();
    void initCfg();
    void initGUI();
    void initBar();
    void initCNT();
    void initDef();
    void initCur();
    void abandoned();

protected:
    void readPlatform();
    void readCamera();
    bool readRegCfg();
    void readSysCfg();
    void saveSysCfg();
    void addDev(StDevInfo devInfo);
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

protected:
    bool readRegData(EuCMD cmd,
                     uint32_t reg,
                     QObject *obj,
                     bool showLog = true,
                     bool async = true,
                     uint32_t *data = nullptr);
    bool writeRegData(EuCMD cmd, uint32_t reg, uint32_t data, bool showLog = true);
    void cmdSleep(int n, bool bReal = true);
    void setData(StDevInfo devInfo);
    void setStus(bool bEnable = true);
    bool noEmpty();

    void showBGImg();

protected:
    void workThread();

protected:
    virtual void resizeEvent(QResizeEvent *event) override;

protected:
    void titleDblClick();

private:
    Ui::MainWidget *ui;

protected:
    EuPlatform m_platform = FW_DEF;
    QMap<QString, EuPlatform> mapPlatform;
    QMap<QString, QString> mapProbefile;

protected:
    QMenu *m_menuHelp = nullptr;
    QAction *act1 = nullptr;
    QAction *act2 = nullptr;
    QAction *act3 = nullptr;
    QMenu *m_menuPlatform = nullptr;
    ImgAttrDlg *m_pImgAttr = nullptr;
    ExpGainDlg *m_pExpAndGain = nullptr;
    TriggerDlg *m_pTrigger = nullptr;
    RegisterDlg *m_pRegister = nullptr;
    CurAttrDlg *m_pCurAttr = nullptr;

protected:
    bool m_bAuthorize = false;
    bool m_bThread = true;
    QSize m_gvSize;
    QTranslator m_translator;
    QGraphicsScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_pixItem = nullptr;
    std::mutex m_mtxPix;
    std::queue<StImgInfo> m_queueSii;
    int m_nChachFrameSize = 5;

protected:
    std::mutex mtx;
    std::condition_variable cv;

protected:
    int m_optNum = 0;
    OperateCmd *m_optCmd = nullptr;
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
    QMap<QString, StDevInfo> m_mapAutoDvInfo;
    QMap<QString, StDevInfo> m_mapManualDvInfo;
    QMap<int, QString> m_mapCmdReg;

private:
    int m_roiX = 0;
    int m_roiY = 0;
    int m_roiW = 0;
    int m_roiH = 0;
    int m_CamFps = 0;
    QString m_strPwd;

    int m_showFps = 0;
    volatile bool m_bFps = false;
    int m_seconds = 1;
    bool m_scale = true;
    QTimer *m_timer = nullptr;
    QString m_i2cAddr = "0x3b";
    std::deque<qint64> timestamps;
    QElapsedTimer timerFps;
    QElapsedTimer timer1;

    QElapsedTimer timer3;
};

#endif // MAINWIDGET_H
