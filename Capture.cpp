#include "Capture.h"
#include "CommStuct.h"
#include "QxHelp.h"

Capture::Capture(CommDevCfg devCfg, QObject *parent)
    : QThread{parent}
    , m_devCfg(devCfg)
{
    qRegisterMetaType<timeval>("timeval");
    qRegisterMetaType<StCamInfo>("StCamInfo");
    qRegisterMetaType<EuCamStatus>("EuCamStatus");
    m_timer = new QTimer(this);
}

Capture::~Capture()
{
    // 先停掉后台存盘线程并 join，再释放本对象；否则 saveThread 会访问已析构的成员。
    stopSaveThread();

    if (m_timer) {
        m_timer->stop();

        m_timer->deleteLater();
        m_timer = nullptr;
    }
}

void Capture::stopSaveThread()
{
    m_bThread = false;
    if (m_threadSave.joinable()) {
        m_threadSave.join();
    }
    // 排空未写完的存盘请求，避免残留帧缓存泄漏
    std::lock_guard<std::mutex> lock(m_mtxSaveImg);
    for (StSaveImg &si : m_listSaveImg) {
        delete[] si.buf;
        si.buf = nullptr;
    }
    m_listSaveImg.clear();
}

void Capture::setMainWidget(QObject *mainWidget)
{
    m_mainWidget = mainWidget;
}

void Capture::startStream(int fd, StCamParam &param)
{
    // 上一轮存盘线程可能还未退出（stop 后马上 start），先 join 避免两个存盘线程同时写盘
    stopSaveThread();
    m_bThread = true;
    m_threadSave = std::thread([this]() { saveThread(); });

    m_fd = fd;
    m_stCamParam = param;
    if (!m_timer->isActive()) {
        connect(m_timer, &QTimer::timeout, this, [this]() {
            StCamInfo info;
            info.fps = m_nFps * 1.0;
            info.height = m_height;
            info.width = m_width;
            info.left = m_x;
            info.top = m_y;
            emit sndFrameInfo(info);
            m_nFps = 0;
        });
        m_timer->start(1000);
    }
}

void Capture::stopStream()
{
    // 等存盘线程真正把队列处理完再返回，与下面子类的 wait() 保持同样的生命周期语义
    stopSaveThread();
}

void Capture::rcvSaveImg(QString strPath, int count)
{
    QxHelp::makeFolder(strPath);
    QxHelp::makeFolderTail(strPath);
    m_strSavePah = strPath;
    m_nSaveCount = count;
    m_saveIndex = 0;
    m_bSave = true;
}

void Capture::rcvShow(bool bShow)
{
    m_bShow = bShow;
}

void Capture::saveThread()
{
    int nCount = 1000;
    while (m_bThread) {
        StSaveImg si;
        int size = 0;
        m_mtxSaveImg.lock();
        size = m_listSaveImg.size();
        if (size > 0) {
            si = m_listSaveImg.front();
            m_listSaveImg.pop_front();
        }
        m_mtxSaveImg.unlock();

        if (size > 0) {
            nCount = 0;
            if (QxHelp::writeFile((char *) si.buf, si.len, si.filename)) {
                qDebug() << "save.... " + si.filename + " OK";
            } else {
                qDebug() << "save.... " + si.filename + " error";
            }

            // si.img.save(si.filename + "_.png", "PNG");
            // QImage img = QImage(si.buf, si.width, si.height, QImage::Format_Grayscale8);
            // img.save(si.filename + ".png", "PNG");
            delete[] si.buf;
        } else {
            if (++nCount > 500 && !m_bSave) {
                if (501 == nCount) {
                    emit sndSaveFinish(true);             
                } else {
                    nCount = 502;
                }
            }
        }

        QThread::msleep(1);
    }
}
