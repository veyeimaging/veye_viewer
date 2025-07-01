#include "capture.h"
#include "commstuct.h"
#include "qxhelp.h"

Capture::Capture(QObject *parent)
    : QThread{parent}
{
    qRegisterMetaType<timeval>("timeval");
    qRegisterMetaType<StCamInfo>("StCamInfo");
}

void Capture::setMainWidget(QObject *mainWidget)
{
    m_mainWidget = mainWidget;
}

void Capture::startStream(int fd, StCamParam &param)
{
    m_bThread = true;
    std::thread tSave([this]() { saveThread(); });
    tSave.detach();

    m_fd = fd;
    m_stCamParam = param;
}

void Capture::stopStream()
{
    m_bThread = false;
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

void Capture::rcvShowFps(int fps)
{
    qDebug() << "rcvShowFPS ...................." << fps;
    m_showFPS = fps;
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
