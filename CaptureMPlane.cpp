#include "CaptureMPlane.h"
#include "MainWidget.h"

CaptureMPlane::CaptureMPlane(CommDevCfg devCfg, QObject *parent)
    : Capture{devCfg, parent}
{

}

CaptureMPlane::~CaptureMPlane() {}

void CaptureMPlane::startStream(int fd, StCamParam &param)
{
    Capture::startStream(fd, param);
    m_bRunning = true;
    start();
}

void CaptureMPlane::stopStream()
{
    Capture::stopStream();
    m_bRunning = false;
    wait();
}

void CaptureMPlane::initBuf()
{
    m_v4l2Fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(m_fd, VIDIOC_G_FMT, &m_v4l2Fmt) == -1) {
        perror("ERROR VIDIOC_G_FMT:");
    }

    m_v4l2Fmt.fmt.pix_mp.width = m_stCamParam.w;
    m_v4l2Fmt.fmt.pix_mp.height = m_stCamParam.h;
    if (m_devCfg.cameType == EuCamType::MvCamera) {
        //m_v4l2Fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_GREY;
    } else if (m_devCfg.cameType == EuCamType::GxCamera) {
        m_v4l2Fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_UYVY;
    }

    if (ioctl(m_fd, VIDIOC_S_FMT, &m_v4l2Fmt) == -1) {
        perror("ERROR VIDIOC_S_FMT:");
    }

    // if (ioctl(m_fd, VIDIOC_G_FMT, &m_v4l2Fmt) == -1) {
    //     perror("ERROR VIDIOC_G_FMT:");
    // }


    m_v4l2ReqBuf.count = VIDEO_BUFFER_COUNT; //帧缓冲数量
    m_v4l2ReqBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    m_v4l2ReqBuf.memory = V4L2_MEMORY_MMAP; //内存映射方式
    if (ioctl(m_fd, VIDIOC_REQBUFS, &m_v4l2ReqBuf) == -1) {
        perror("ERROR VIDIOC_REQBUFS:");   
    }

    num_planes = m_v4l2Fmt.fmt.pix_mp.num_planes;
    m_buffers = (StMplaneBuffer *) calloc(m_v4l2ReqBuf.count, sizeof(StMplaneBuffer));
    for (int i = 0; i < m_v4l2ReqBuf.count; ++i) {
        v4l2_plane *planes = (v4l2_plane *) calloc(num_planes, sizeof(v4l2_plane));
        StMmapAddr *addrs = (StMmapAddr *) calloc(num_planes, sizeof(StMmapAddr));

        m_v4l2Buf.index = i;
        m_v4l2Buf.memory = V4L2_MEMORY_MMAP;
        m_v4l2Buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        m_v4l2Buf.m.planes = planes;
        m_v4l2Buf.length = num_planes;
        //从内核空间查询一空间做映射
        if (ioctl(m_fd, VIDIOC_QUERYBUF, &m_v4l2Buf) == -1) {
            perror("ERROR VIDIOC_QUERYBUF:");
        }

        m_buffers[i].planes = planes;
        m_buffers[i].addrs = addrs;

        for (int j = 0; j < num_planes; ++j) {
            m_buffers[i].addrs[j].start = mmap(nullptr,
                                               m_v4l2Buf.m.planes[j].length,
                                               PROT_READ | PROT_WRITE,
                                               MAP_SHARED,
                                               m_fd,
                                               m_v4l2Buf.m.planes[j].m.mem_offset);
            if (MAP_FAILED == m_buffers[i].addrs[j].start) {
                perror("MAP_FAILED == buffers[i].plane_start[j].start...........");
            }
        }
        if (ioctl(m_fd, VIDIOC_QBUF, &m_v4l2Buf) == -1) {
            perror("ERROR VIDIOC_QBUF:");
        }
    }

    //for (int i = 0; i < m_v4l2ReqBuf.count; i++) {
    int bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(m_fd, VIDIOC_STREAMON, &bufType) == -1) {
        perror("ERROR VIDIOC_STREAMON:");
    }
    //}
}

void CaptureMPlane::freeBuf()
{
    //for (int i = 0; i < m_v4l2ReqBuf.count; i++) {
    int bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(m_fd, VIDIOC_STREAMOFF, &bufType) == -1) {
        perror("VIDIOC_STREAMOFF");
    }
    //}

    for (int i = 0; i < m_v4l2ReqBuf.count; i++) {
        m_v4l2Buf.index = i;
        ioctl(m_fd, VIDIOC_QBUF, &m_v4l2Buf);
        for (int j = 0; j < num_planes; ++j) {
            munmap(m_buffers[i].addrs[j].start, m_buffers[i].planes[j].length);
        }
    }

    for (int i = 0; i < m_v4l2ReqBuf.count; ++i) {
        free(m_buffers[i].planes);
        m_buffers[i].planes = nullptr;
        free(m_buffers[i].addrs);
        m_buffers[i].addrs = nullptr;
    }
    free(m_buffers);
    m_buffers = nullptr;

    memset(&m_v4l2ReqBuf, 0, sizeof(m_v4l2ReqBuf));
    m_v4l2ReqBuf.count = 0; //帧缓冲数量
    m_v4l2ReqBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    m_v4l2ReqBuf.memory = V4L2_MEMORY_MMAP; //内存映射方式
    if (0 != ioctl(m_fd, VIDIOC_REQBUFS, &m_v4l2ReqBuf)) {
        perror("ERROR: failed to VIDIOC_REQBUFS");
    }
    qDebug() << "...freeBuf....";
}

void CaptureMPlane::run()
{
    qDebug() << "...............run.....................enter";
    QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
    initBuf();
    qDebug() << "...............run.....................initBuf";
    int nBytesperline = 0;
    unsigned char *pBuf = nullptr;
    int strategy = new_buf;
    fd_set readfds;
    timeval timeout;
    struct timeval ts;
    emit sndStatus(1);
    bool bNeedSndShow = false;

    while (m_bRunning) {
        FD_ZERO(&readfds);
        FD_SET(m_fd, &readfds);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        int nfds = m_fd + 1;
        int ready = select(nfds, &readfds, nullptr, nullptr, &timeout);
        if (-1 == ready) {
            qDebug() << "read data error!" << errno;
        } else if (0 == ready) {
            qDebug() << "read data time out!";
        } else {
            if (FD_ISSET(m_fd, &readfds)) {
                while (0 == ioctl(m_fd, VIDIOC_DQBUF, &m_v4l2Buf)) {
                    for (int j = 0; j < num_planes; j++) {
                        nBytesperline = m_v4l2Fmt.fmt.pix_mp.plane_fmt[j].bytesperline;
                        m_width = m_v4l2Fmt.fmt.pix_mp.width;
                        m_height = m_v4l2Fmt.fmt.pix_mp.height;
                        pBuf = (unsigned char *) m_buffers[m_v4l2Buf.index].addrs[j].start;
                        int len = m_buffers[m_v4l2Buf.index].planes[j].length;
                        ts = m_v4l2Buf.timestamp;
                        QString strTS = QString::asprintf("%ld.%06ld", ts.tv_sec, ts.tv_usec);                
                        m_nFps++;
                        if (m_bShow) {
                            StImgInfo sii;
                            sii.buf = new unsigned char[len];
                            memcpy(sii.buf, pBuf, len);
                            sii.strategy = strategy;
                            sii.len = len;
                            sii.fps = m_nFps;
                            sii.nBytesperline = nBytesperline;
                            sii.width = m_width;
                            sii.height = m_height;
                            sii.strTS = strTS;
                            if (m_mainWidget) {
                                MainWidget *pMain = qobject_cast<MainWidget *>(m_mainWidget);
                                if (pMain) {
                                    pMain->pushData(sii);
                                }
                            }
                            bNeedSndShow = true;
                        } else {
                            if (bNeedSndShow) {
                                bNeedSndShow = false;
                                emit sndStatus(2);
                            }
                        }
                        if (m_bSave && m_nSaveCount > 0) {
                            QString strIndex = strTS + "_" + QString::number(++m_saveIndex);
                            QString strTime = QDateTime::currentDateTime().toString(
                                "yyyyMMddHHmmss");
                            QString strFilename;
                            if (m_devCfg.cameType == EuCamType::MvCamera) {
                                strFilename = m_strSavePah
                                              + QString("%1_%2x%3_%4.RAW")
                                                    .arg(strTime)
                                                    .arg(nBytesperline)
                                                    .arg(m_height)
                                                    .arg(strIndex);
                            } else if (m_devCfg.cameType == EuCamType::GxCamera) {
                                strFilename = m_strSavePah
                                              + QString("%1_%2x%3_%4.YUV")
                                                    .arg(strTime)
                                                    .arg(nBytesperline)
                                                    .arg(m_height)
                                                    .arg(strIndex);
                            }

                            m_mtxSaveImg.lock();
                            StSaveImg si;       
                            si.buf = new unsigned char[len];
                            memcpy(si.buf, pBuf, len);
                            si.len = len;
                            si.width = nBytesperline;
                            si.height = m_height;
                            si.path = m_strSavePah;
                            si.filename = strFilename;
                            m_listSaveImg.push_back(si);
                            m_nSaveCount--;                     
                            m_mtxSaveImg.unlock();
                        }
                        if (m_nSaveCount <= 0) {
                            m_bSave = false;
                        }
                    }
                    //读取数据后将缓冲区放入队列
                    if (0 != ioctl(m_fd, VIDIOC_QBUF, &m_v4l2Buf)) {
                        perror("ERROR: VIDIOC_QBUF ");
                        break;
                    }
                }
            }
        }
        msleep(1);
    }
    freeBuf();
    qDebug() << "...............run.....................freeBuf";
    emit sndStatus(0);
    qDebug() << "...............run.....................leave";
}
