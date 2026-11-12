#ifndef CAPTURE_H
#define CAPTURE_H

#include <QObject>
#include <QThread>
#include <atomic>
#include <thread>
#include "CommHeader.h"
#include "CommStuct.h"

#define VIDEO_BUFFER_COUNT 4

typedef struct _tagMmapAddr
{
    void *start;
} StMmapAddr;

typedef struct _tagMplaneBuffer
{
    StMmapAddr *addrs;
    v4l2_plane *planes;
} StMplaneBuffer;

typedef struct _tagSplaneBuffer
{
    void *start;
    size_t length;
} StSplaneBuffer;

class Capture : public QThread
{
    Q_OBJECT
public:
    explicit Capture(CommDevCfg devCfg, QObject *parent = nullptr);
    ~Capture();

public:
    void setMainWidget(QObject *mainWidget);
public:
    virtual void startStream(int fd, StCamParam &param);
    virtual void stopStream();
signals:
    void sndCaptureImg(const StImgInfo &sii);
    void sndFrameInfo(const StCamInfo &sci);
    void sndSaveFinish(bool bFlag);
    void sndStatus(EuCamStatus status);
public slots:
    void rcvSaveImg(QString strPath, int count);
    void rcvShow(bool bShow);
protected:
    virtual void initBuf() = 0;
    virtual void freeBuf() = 0;

protected:
    void saveThread();
    // 停止并 join 存盘线程，同时排空待写队列（stopStream / 析构都会调用）
    void stopSaveThread();

protected:
    // 停止标志必须原子可见；线程句柄必须保存并在 stopStream/析构时 join，
    // 否则 saveThread 会在 Capture 析构后继续访问 m_mtxSaveImg/m_listSaveImg 并在已析构的 QObject 上 emit。
    std::atomic<bool> m_bThread{true};
    std::thread m_threadSave;

protected:
    std::mutex m_mtxSaveImg;
    std::list<StSaveImg> m_listSaveImg;

protected:
    QObject *m_mainWidget = nullptr;
    volatile bool m_bRunning = true;
    int m_fd = -1;
    StCamParam m_stCamParam;
    CommDevCfg m_devCfg;
    int m_x = 0;
    int m_y = 0;
    int m_width = 0;
    int m_height = 0;
    int m_nFps = 0;
    struct v4l2_buffer m_v4l2Buf = {0};
    struct v4l2_crop m_v4l2Crop = {0};
    struct v4l2_cropcap m_v4l2CropCap = {0};
    struct v4l2_format m_v4l2Fmt = {0};
    struct v4l2_requestbuffers m_v4l2ReqBuf = {0};

    QString m_strSavePah;
    int m_nSaveCount = 0;
    int m_saveIndex = 0;
    bool m_bSave = false;
    bool m_bShow = false;
    QTimer *m_timer = nullptr;
};

#endif // CAPTURE_H
