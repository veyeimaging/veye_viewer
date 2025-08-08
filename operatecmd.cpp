#include "operatecmd.h"
#include <arpa/inet.h>

struct sensor_regs
{
    uint16_t reg;
    uint32_t data;
};

static int i2c_rd(int fd, uint8_t i2c_addr, uint16_t reg, uint32_t *values, uint32_t n)
{
    int err;
    int i = 0;
    uint8_t buf[2] = {static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xff)};
    uint8_t bufout[8] = {0};
    struct i2c_rdwr_ioctl_data msgset;
    struct i2c_msg msgs[2] = {
        {
            .addr = i2c_addr,
            .flags = 0,
            .len = 2,
            .buf = buf,
        },
        {
            .addr = i2c_addr,
            .flags = I2C_M_RD,
            .len = 4,
            .buf = bufout,
        },
    };

    msgset.msgs = msgs;
    msgset.nmsgs = 2;

    err = ioctl(fd, I2C_RDWR, &msgset);
    if (err != msgset.nmsgs) {
        //printf("Read i2c err %d\n", err);
        return -1;
    }
    for (i = 0; i < n; ++i) {
        *values = ntohl(*(uint32_t *) bufout);
        //fprintf(stderr, "addr %04x : value %08x \n", reg + i, values[i]);
    }

    return 0;
}

static int send_regs(int fd, const struct sensor_regs *regs, int num_regs)
{
    int ret = 0;
    for (int i = 0; i < num_regs; i++) {
        unsigned char msg[8] = {
            static_cast<unsigned char>(regs[i].reg >> 8),
            static_cast<unsigned char>(regs[i].reg),
            static_cast<unsigned char>(regs[i].data >> 24),
            static_cast<unsigned char>(regs[i].data >> 16),
            static_cast<unsigned char>(regs[i].data >> 8),
            static_cast<unsigned char>(regs[i].data),
        };
        int len = 6;

        if (write(fd, msg, len) != len) {
            fprintf(stderr, "Failed to write register index %d,please try again\r\n", regs[i].reg);
            ret = -1;
        } else {
            //fprintf(stderr, " write register data %d\r\n", regs[i].data);
        }
    }
    return ret;
}

static int i2c_write_bytes(int fd, uint8_t slave_addr, uint8_t reg_addr, uint8_t *values, uint8_t len)
{
    uint8_t *outbuf = NULL;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];

    outbuf = (uint8_t *) malloc(len + 1);
    if (!outbuf) {
        printf("Error: No memory for buffer\n");
        return -1;
    }

    outbuf[0] = reg_addr;
    memcpy(outbuf + 1, values, len);

    messages[0].addr = slave_addr;
    messages[0].flags = 0;
    messages[0].len = len + 1;
    messages[0].buf = outbuf;

    /* Transfer the i2c packets to the kernel and verify it worked */
    packets.msgs = messages;
    packets.nmsgs = 1;
    if (ioctl(fd, I2C_RDWR, &packets) < 0) {
        printf("Error: Unable to send data");
        free(outbuf);
        return -1;
    }

    free(outbuf);

    return 0;
}

OperateCmd::OperateCmd(QObject *parent)
    : QObject{parent}
{
    m_bThread = true;
    std::thread tSave([this]() { cmdThread(); });
    tSave.detach();
}

OperateCmd::~OperateCmd()
{
    m_bThread = false;
}

void OperateCmd::openDev(QString &deviceName, int nType, int &nBufType)
{
    QString strMsg;
    int fd = open(deviceName.toStdString().c_str(), O_RDWR);
    if (-1 != fd) {
        qDebug() << "open " << deviceName << "OK";
        switch (nType) {
        case 0:            
            break;
        case 1: {
            struct v4l2_capability cap;
            int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
            if (ret < 0) {
                perror("获取功能失败");
            } else {
                qDebug() << "...capabilities..ok..." << ret;
                if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
                    nBufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                } else if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) {
                    nBufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
                } else {
                    qDebug() << "capabilities........no...........CAPTURE";
                }
            }
        }
        default:
            break;
        }
        strMsg = "open " + deviceName + " OK";
        emit sndOptLog(0, strMsg);
        close(fd);
    } else {
        strMsg = "open " + deviceName + " " + strerror(errno);
        qDebug() << strMsg;
        emit sndOptLog(fd, strMsg);
    }
}

void OperateCmd::runSystemCmd(QString strCmd)
{
    int nLen = 1024;
    char szBuf[1024] = {0};
    int code = runSystem((char *) strCmd.toStdString().c_str(), szBuf, nLen);
    emit sndOptLog(code, strCmd);
    if (0 != code) {
        emit sndOptLog(code, szBuf);
    }
}

void OperateCmd::systemCmd(QString strCmd)
{
    int code = system(strCmd.toStdString().c_str());
    emit sndOptLog(code, strCmd);
    if (0 != code) {
        emit sndOptLog(code, strerror(errno));
    }
}

void OperateCmd::mediaRunCmd(QString mediaNode, QString strParam)
{
    QString strCmd = "media-ctl -d ";
    strCmd += mediaNode;
    strCmd += " --set-v4l2 ";
    strCmd += strParam;
    int code = system(strCmd.toStdString().c_str());
    emit sndOptLog(code, strCmd);
    if (0 != code) {
        emit sndOptLog(code, strerror(errno));
    }
}

void OperateCmd::asynReadData(StI2CCmd cmd)
{
    m_mtx.lock();
    m_listCmd.push_back(cmd);
    m_mtx.unlock();
}

void OperateCmd::syncReadData(QString i2c, uint32_t addr, uint32_t reg, uint32_t &data, bool showLog)
{
    readData(i2c, addr, reg, data, showLog);
}

void OperateCmd::asynWriteDate(StI2CCmd cmd)
{
    m_mtx.lock();
    m_listCmd.push_back(cmd);
    m_mtx.unlock();
}

int OperateCmd::readData(QString i2c, uint32_t addr, uint32_t reg, uint32_t &data, bool showLog)
{
    int nRet = -1;
    QString strMsg;
    QString strReg = QString("0x%1").arg(reg, 2, 16, QLatin1Char('0'));
    int fd = open(i2c.toStdString().c_str(), O_RDWR);
    if (-1 != fd) {
        if (0 == ioctl(fd, I2C_SLAVE_FORCE, addr)) {
            uint32_t values;
            uint32_t num = 1;
            if (QX_SUCCESS == i2c_rd(fd, addr, reg, &data, num)) {
                QString strData = QString("0x%1").arg(data, 4, 16, QLatin1Char('0'));
                strMsg = QString("i2c_read regAddr:%1\tdata:[hex]:%2--[dec]:%3")
                             .arg(strReg, strData)
                             .arg(data);
                emit sndOptLog(0, strMsg, showLog);
            } else {
                strMsg = QString("i2c_read regAddr %1 failed").arg(strReg);
                emit sndOptLog(-1, strMsg, showLog);
            }
            nRet = 0;
        } else {
            strMsg = QString("i2c_read I2C_SLAVE_FORCE %1 %2").arg(i2c, strerror(errno));
            emit sndOptLog(errno, strMsg, showLog);
        }
        close(fd);
    } else {
        strMsg = QString("i2c_read open %1 %2").arg(i2c, strerror(errno));
        emit sndOptLog(fd, strMsg, showLog);
    }
    return nRet;
}

int OperateCmd::writeDate(QString i2c, uint32_t addr, uint32_t reg, uint32_t data, bool showLog)
{
    int nRet = -1;
    QString strMsg;

    QString strReg = QString("0x%1").arg(reg, 2, 16, QLatin1Char('0'));
    QString strData = QString("0x%1").arg(data, 4, 16, QLatin1Char('0'));
    int fd = open(i2c.toStdString().c_str(), O_RDWR);
    if (-1 != fd) {
        struct sensor_regs regs;
        regs.reg = reg;
        regs.data = data;
        //qDebug() << "................" << regs.data;
        if (ioctl(fd, I2C_SLAVE_FORCE, addr) < 0) {
            strMsg = QString("i2c_wirte I2C_SLAVE_FORCE %1 %2 %3").arg(i2c, strReg, strerror(errno));
            emit sndOptLog(errno, strMsg, showLog);
            close(fd);
            return nRet;
        }
        if (QX_SUCCESS == send_regs(fd, &regs, 1)) {
            nRet = 0;
            strMsg = QString("i2c_wirte regAddr:%1\tdata:[hex]:%2--[dec]:%3")
                         .arg(strReg, strData)
                         .arg(data);
            emit sndOptLog(0, strMsg, showLog);
        } else {
            strMsg = QString("i2c_wirte regAddr %1 failed").arg(strReg);
            emit sndOptLog(-1, strMsg, showLog);
        }
        // uint8_t buffer[1024] = {0};
        // buffer[0] = data;
        // qDebug() << "i2c buffer:" << buffer[0];
        // if (QX_SUCCESS == i2c_write_bytes(fd, device_addr, reg, buffer, 1)) {
        //     nRet = 0;
        //     qDebug() << "i2c WriteDate:" << data;
        // }

        close(fd);
    } else {
        strMsg = QString("i2c_wirte open %1 %2").arg(i2c, strerror(errno));
        emit sndOptLog(fd, strMsg, showLog);
    }
    return nRet;
}

//system函数扩展，加入超时值(0表示永久等待)
//超时时返回-2，其他情况返回不变。
/* with appropriate signal handling */
int OperateCmd::system_ex(const char *cmdstring, unsigned int timeout)
{
    pid_t pid;
    int status;
    struct sigaction ignore, saveintr, savequit;
    sigset_t chldmask, savemask;

    //精度换成十分之一秒
    timeout *= 10;
    if (timeout == 0)
        timeout = 0xFFFFFFFF;

    if (cmdstring == NULL)
        return (1); /* always a command processor with UNIX */

    ignore.sa_handler = SIG_IGN; /* ignore SIGINT and SIGQUIT */
    sigemptyset(&ignore.sa_mask);
    ignore.sa_flags = 0;
    if (sigaction(SIGINT, &ignore, &saveintr) < 0)
        return (-1);
    if (sigaction(SIGQUIT, &ignore, &savequit) < 0)
        return (-1);
    sigemptyset(&chldmask); /* now block SIGCHLD */
    sigaddset(&chldmask, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &chldmask, &savemask) < 0)
        return (-1);

    if ((pid = fork()) < 0) {
        return -1;         /* probably out of processes */
    } else if (pid == 0) { /* child */
        /* restore previous signal actions & reset signal mask */
        sigaction(SIGINT, &saveintr, NULL);
        sigaction(SIGQUIT, &savequit, NULL);
        sigprocmask(SIG_SETMASK, &savemask, NULL);
        /*通常exec会放在fork() 函数的子进程部分, 来替代子进程执行啦, 执行成功后子程序就会消失,  但是执行失败的话, 必须用exit()函数来让子进程退出!*/
        execl("/bin/sh",
              "sh",
              "-c",
              cmdstring,
              (char *) 0); /*exec函数会取代执行它的进程,  也就是说, 一旦exec函数执行成功, 它就不会返回了, 进程结束.   但是如果exec函数执行失败, 它会返回失败的信息,  而且进程继续执行后面的代码!*/
        _exit(127);        /* exec error */
    }

    /* parent */
    int ret = 0;
    while (timeout-- > 0 && (ret = waitpid(pid, &status, WNOHANG)) == 0)
        usleep(100 * 1000);

    /* restore previous signal actions & reset signal mask */
    if (sigaction(SIGINT, &saveintr, NULL) < 0)
        return (-1);
    if (sigaction(SIGQUIT, &savequit, NULL) < 0)
        return (-1);
    if (sigprocmask(SIG_SETMASK, &savemask, NULL) < 0)
        return (-1);

    if (ret < 0)
        return -1;

    if (ret > 0)
        return status;

    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);
    return -2;
}

/**
   * 增强的system函数，能够返回system调用的输出
   *
   * @param[in] cmdstring 调用外部程序或脚本的命令串
   * @param[out] buf 返回外部命令的结果的缓冲区
   * @param[in] len 缓冲区buf的长度
   *
   * @return 0: 成功; -1: 失败 
   */
int OperateCmd::runSystem(const char *cmdstring, char *buff, int size)
{
    char cmd_string[256] = {0};
    char tmpfile[200] = {0};
    char tmp_buf[100] = {0};
    int fd;
    int tmp_fd;
    int nbytes;

    memset(buff, 0, size);

    if ((cmdstring == NULL) || (strlen(cmdstring) > (sizeof(tmpfile) + 8))
        || ((strlen(cmdstring) + strlen(tmpfile) + 5) > sizeof(cmd_string))) {
        printf("cmd is too long or NULL!\n");
        return -1;
    }
    sscanf(cmdstring, "%[a-Z]", tmp_buf); /*%[a-z] 表示匹配a到z中任意字符，贪婪性(尽可能多的匹配) */
    sprintf(tmpfile, "/tmp/%s-XXXXXX", tmp_buf);

    tmp_fd = mkstemp(tmpfile);
    if (tmp_fd < 0) {
        printf("mkstemp failed\n");
        return -1;
    }
    close(tmp_fd);

    sprintf(cmd_string,
            "%s > %s 2>&1",
            cmdstring,
            tmpfile); /*标准输出（1），标准错误（2）都输出到临时文件*/
    if (system_ex(cmd_string, 20) < 0) {
        printf("run \"%s\" ret < 0!\n", cmd_string);
    }

    fd = open(tmpfile, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed!\n", tmpfile);
        nbytes = -1;
    } else {
        nbytes = read(fd, buff, size - 1);
        close(fd);
    }

    if ('\n' == buff[nbytes - 1]) {
        buff[nbytes - 1] = '\0';
    }

    memset(cmd_string, 0, sizeof(cmd_string));
    sprintf(cmd_string, "rm -rf /tmp/%s-*", tmp_buf);
    system_ex(cmd_string, 20);

    return nbytes;
}

void OperateCmd::cmdSleep(int n, bool bReal)
{
    if (bReal) {
        QThread::msleep(n);
    }
    emit sndOptLog(0, QString("sleep %1 ms").arg(n), true);
}

void OperateCmd::cmdThread()
{
    QElapsedTimer timer;
    while (m_bThread) {
        int count = 0;
        StI2CCmd cmd;
        {
            m_mtx.lock();
            count = m_listCmd.size();
            if (count > 0) {
                cmd = m_listCmd.front();
                m_listCmd.pop_front();
            }
            m_mtx.unlock();
        }
        if (count > 0) {
            if (cmd.bWrite) {
                writeDate(cmd.i2c, cmd.addr, cmd.reg, cmd.data, cmd.showlog);
                if (cmd.cmd == def_cmd) {
                    cmdSleep(10);
                }
                if (cmd.cmd == trgmode) {
                    QThread::msleep(100);
                }
            } else {
                readData(cmd.i2c, cmd.addr, cmd.reg, cmd.data, cmd.showlog);
                StCmdRet ret;
                ret.cmd = cmd.cmd;
                ret.data = cmd.data;
                ret.obj = cmd.obj;
                ret.show = cmd.showlog;
                //timer.restart();
                emit sndCmdRet(ret);

                //qint64 elapsedTime = timer.elapsed(); // 获取经过的时间，单位为微秒
                //if (elapsedTime > TIEM_OUT) {
                //qDebug() << "cmdThread " << elapsedTime << "microseconds";
                //}
            }
        }
        QThread::msleep(1);
    }
}
