// #include "OperateCmd.h"
// #include <arpa/inet.h>

// #define WRITE_HEAD 0xAB
// #define PRE_READ_HEAD 0xBC
// #define POST_READ_HEAD 0xAC
// #define DIR_READ_HEAD 0xDE

// static uint8_t xor8(const uint8_t *data, uint32_t len)
// {
//     if (data == nullptr || len == 0) {
//         return 0;
//     }
//     uint8_t checksum = 0;
//     const uint8_t *p = data;
//     while (len--) {
//         checksum ^= *p++;
//     }
//     return checksum;
// }

// static int gxcam_readl_reg(int fd, uint8_t i2c_addr, uint16_t reg, int32_t *val)
// {
//     struct preread_regs regs;
//     uint8_t checksum = 0;
//     uint8_t buf[3] = {POST_READ_HEAD,
//                       static_cast<uint8_t>(reg >> 8),
//                       static_cast<uint8_t>(reg & 0xff)};
//     uint8_t bufout[8] = {0};

//     regs.pre_head = PRE_READ_HEAD;
//     regs.reg = htons(reg);
//     regs.xor_check = xor8((uint8_t *) &regs, 3);

//     struct i2c_rdwr_ioctl_data msgset;
//     struct i2c_msg msg = {
//         .addr = i2c_addr,
//         .flags = 0,
//         .len = sizeof(regs),
//         .buf = (uint8_t *) &regs,
//     };

//     msgset.msgs = &msg;
//     msgset.nmsgs = 1;

//     if (ioctl(fd, I2C_RDWR, &msgset) != msgset.nmsgs) {
//         printf("Write i2c err \n");
//         return -1;
//     }

//     usleep(20000);

//     struct i2c_msg msgs[2] = {
//         {
//             .addr = i2c_addr,
//             .flags = 0,
//             .len = 3,
//             .buf = buf,
//         },
//         {
//             .addr = i2c_addr,
//             .flags = I2C_M_RD,
//             .len = 6,
//             .buf = bufout,
//         },
//     };
//     msgset.msgs = msgs;
//     msgset.nmsgs = 2;

//     if (ioctl(fd, I2C_RDWR, &msgset) != msgset.nmsgs) {
//         printf("Read i2c err\n");
//         return -1;
//     }

//     checksum = xor8(bufout, 5);
//     if (checksum != bufout[5]) {
//         printf("%s: Read register 0x%02x checksum error\n", __func__, reg);
//         return -EIO;
//     }
//     *val = ntohl(*(int32_t *) bufout);
//     return 0;
// }

// static int gxcam_writel_reg(int fd, uint8_t i2c_addr, uint16_t reg, uint32_t val)
// {
//     struct write_regs wr;

//     wr.head = WRITE_HEAD;
//     wr.reg = htons(reg);
//     wr.data = htonl(val);
//     wr.xor_check = xor8((uint8_t *) &wr, sizeof(wr) - 1);
//     /*	return write(fd, &wr,sizeof(wr));*/
//     struct i2c_rdwr_ioctl_data msgset;
//     struct i2c_msg msg = {
//         .addr = i2c_addr,
//         .flags = 0,
//         .len = sizeof(wr),
//         .buf = (uint8_t *) &wr,
//     };

//     msgset.msgs = &msg;
//     msgset.nmsgs = 1;

//     if (ioctl(fd, I2C_RDWR, &msgset) != msgset.nmsgs) {
//         printf("Write i2c err \n");
//         return -1;
//     }
//     return 0;
// }

// static int mvcam_readl_reg(int fd, uint8_t i2c_addr, uint16_t reg, int32_t *val)
// {
//     int err;
//     int i = 0;
//     uint8_t buf[2] = {static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xff)};
//     uint8_t bufout[8] = {0};
//     struct i2c_rdwr_ioctl_data msgset;
//     struct i2c_msg msgs[2] = {
//         {
//             .addr = i2c_addr,
//             .flags = 0,
//             .len = 2,
//             .buf = buf,
//         },
//         {
//             .addr = i2c_addr,
//             .flags = I2C_M_RD,
//             .len = 4,
//             .buf = bufout,
//         },
//     };

//     msgset.msgs = msgs;
//     msgset.nmsgs = 2;

//     err = ioctl(fd, I2C_RDWR, &msgset);
//     if (err != msgset.nmsgs) {
//         return -1;
//     }
//     *val = ntohl(*(int32_t *) bufout);
//     return 0;
// }

// static int mvcam_writel_reg(int fd, uint8_t i2c_addr, uint16_t reg, uint32_t val)
// {
//     int ret = 0;

//     unsigned char msg[8] = {
//         static_cast<unsigned char>(reg >> 8),
//         static_cast<unsigned char>(reg),
//         static_cast<unsigned char>(val >> 24),
//         static_cast<unsigned char>(val >> 16),
//         static_cast<unsigned char>(val >> 8),
//         static_cast<unsigned char>(val),
//     };
//     int len = 6;

//     if (write(fd, msg, len) != len) {
//         fprintf(stderr, "Failed to write register index %d,please try again\r\n", reg);
//         ret = -1;
//     } else {
//         //fprintf(stderr, " write register data %d\r\n", regs[i].data);
//     }

//     return ret;
// }

// static int readl_reg(EuCamType camType, int fd, uint8_t i2c_addr, uint16_t reg, int32_t *val)
// {
//     int ret = QX_FAILURE;
//     switch (camType) {
//     case EuCamType::GxCamera:
//         ret = gxcam_readl_reg(fd, i2c_addr, reg, val);
//         break;
//     case EuCamType::MvCamera:
//         ret = mvcam_readl_reg(fd, i2c_addr, reg, val);
//         break;
//     default:
//         break;
//     }
//     return ret;
// }

// static int writel_reg(EuCamType camType, int fd, uint8_t i2c_addr, uint16_t reg, uint32_t val)
// {
//     int ret = QX_FAILURE;
//     switch (camType) {
//     case EuCamType::GxCamera:
//         ret = gxcam_writel_reg(fd, i2c_addr, reg, val);
//         break;
//     case EuCamType::MvCamera:
//         ret = mvcam_writel_reg(fd, i2c_addr, reg, val);
//         break;
//     default:
//         break;
//     }
//     return ret;
// }

// OperateCmd::OperateCmd(QObject *parent)
//     : QObject{parent}
// {
//     m_bThread = true;
//     std::thread tSave([this]() { cmdThread(); });
//     tSave.detach();
// }

// OperateCmd::~OperateCmd()
// {
//     m_bThread = false;
// }

// void OperateCmd::runSystemCmd(QString strCmd)
// {
//     // StI2CCmd cmd;
//     // cmd.sysCmd = strCmd;
//     // cmd.cmdType = 1;
//     // m_mtx.lock();
//     // m_listCmd.push_back(cmd);
//     // m_mtx.unlock();

//     int nLen = 1024;
//     char szBuf[1024] = {0};
//     m_mtx.lock();
//     int ret = runSystem((char *) strCmd.toStdString().c_str(), szBuf, nLen);
//     m_mtx.unlock();
//     emit sndOptLog(ret, strCmd);
//     if (0 != ret) {
//         emit sndOptLog(ret, szBuf);
//     }
// }

// void OperateCmd::asynReadData(StI2CCmd cmd)
// {
//     m_mtx.lock();
//     m_listCmd.push_back(cmd);
//     m_mtx.unlock();
// }

// void OperateCmd::asynWriteDate(StI2CCmd cmd)
// {
//     m_mtx.lock();
//     m_listCmd.push_back(cmd);
//     m_mtx.unlock();
// }

// void OperateCmd::syncReadData(StI2CCmd &cmd)
// {
//     m_mtx.lock();
//     readData(cmd);
//     m_mtx.unlock();
// }

// int OperateCmd::readData(StI2CCmd &cmd)
// {
//     int ret = QX_FAILURE;
//     QString strMsg;
//     QString strReg = QString("0x%1").arg(cmd.reg, 2, 16, QLatin1Char('0'));
//     int fd = open(cmd.i2c.toStdString().c_str(), O_RDWR);
//     if (-1 != fd) {
//         if (0 == ioctl(fd, I2C_SLAVE_FORCE, cmd.addr)) {
//             if (QX_SUCCESS == readl_reg(cmd.camType, fd, cmd.addr, cmd.reg, &cmd.data)) {
//                 QString strData = QString("0x%1").arg(cmd.data, 4, 16, QLatin1Char('0'));
//                 strMsg = QString("i2c_read regAddr:%1\tdata:[hex]:%2--[dec]:%3")
//                              .arg(strReg, strData)
//                              .arg(cmd.data);
//                 emit sndOptLog(0, strMsg, cmd.showlog);
//             } else {
//                 strMsg = QString("i2c_read regAddr %1 failed").arg(strReg);
//                 emit sndOptLog(-1, strMsg, cmd.showlog);
//             }
//             ret = QX_SUCCESS;
//         } else {
//             strMsg = QString("i2c_read I2C_SLAVE_FORCE %1 %2").arg(cmd.i2c, strerror(errno));
//             emit sndOptLog(errno, strMsg, cmd.showlog);
//         }
//         close(fd);
//     } else {
//         strMsg = QString("i2c_read open %1 %2").arg(cmd.i2c, strerror(errno));
//         emit sndOptLog(fd, strMsg, cmd.showlog);
//     }
//     if (cmd.showlog) {
//         qDebug() << strMsg;
//     }
//     return ret;
// }

// int OperateCmd::writeData(StI2CCmd &cmd)
// {
//     int ret = -1;
//     QString strMsg;
//     QString strReg = QString("0x%1").arg(cmd.reg, 2, 16, QLatin1Char('0'));
//     QString strData = QString("0x%1").arg(cmd.data, 4, 16, QLatin1Char('0'));
//     int fd = open(cmd.i2c.toStdString().c_str(), O_RDWR);
//     if (-1 != fd) {
//         if (ioctl(fd, I2C_SLAVE_FORCE, cmd.addr) < 0) {
//             strMsg = QString("i2c_wirte I2C_SLAVE_FORCE %1 %2 %3")
//                          .arg(cmd.i2c, strReg, strerror(errno));
//             emit sndOptLog(errno, strMsg, cmd.showlog);
//             close(fd);
//             return ret;
//         }
//         if (QX_SUCCESS == writel_reg(cmd.camType, fd, cmd.addr, cmd.reg, cmd.data)) {
//             ret = 0;
//             strMsg = QString("i2c_wirte regAddr:%1\tdata:[hex]:%2--[dec]:%3")
//                          .arg(strReg, strData)
//                          .arg(cmd.data);
//             emit sndOptLog(0, strMsg, cmd.showlog);
//         } else {
//             strMsg = QString("i2c_wirte regAddr %1 failed").arg(strReg);
//             emit sndOptLog(-1, strMsg, cmd.showlog);
//         }
//         close(fd);

//     } else {
//         strMsg = QString("i2c_wirte open %1 %2").arg(cmd.i2c, strerror(errno));
//         emit sndOptLog(fd, strMsg, cmd.showlog);
//     }
//     if (cmd.showlog) {
//         qDebug() << strMsg;
//     }
//     return ret;
// }

// void OperateCmd::systemCmd(QString strCmd)
// {
//     int code = system(strCmd.toStdString().c_str());
//     emit sndOptLog(code, strCmd);
//     if (0 != code) {
//         emit sndOptLog(code, strerror(errno));
//     }
// }
// //system函数扩展，加入超时值(0表示永久等待)超时时返回-2，其他情况返回不变。
// int OperateCmd::system_ex(const char *cmdstring, unsigned int timeout)
// {
//     pid_t pid;
//     int status;
//     struct sigaction ignore, saveintr, savequit;
//     sigset_t chldmask, savemask;

//     //精度换成十分之一秒
//     timeout *= 10;
//     if (timeout == 0)
//         timeout = 0xFFFFFFFF;

//     if (cmdstring == NULL)
//         return (1); /* always a command processor with UNIX */

//     ignore.sa_handler = SIG_IGN; /* ignore SIGINT and SIGQUIT */
//     sigemptyset(&ignore.sa_mask);
//     ignore.sa_flags = 0;
//     if (sigaction(SIGINT, &ignore, &saveintr) < 0)
//         return (-1);
//     if (sigaction(SIGQUIT, &ignore, &savequit) < 0)
//         return (-1);
//     sigemptyset(&chldmask); /* now block SIGCHLD */
//     sigaddset(&chldmask, SIGCHLD);
//     if (sigprocmask(SIG_BLOCK, &chldmask, &savemask) < 0)
//         return (-1);

//     if ((pid = fork()) < 0) {
//         return -1;         /* probably out of processes */
//     } else if (pid == 0) { /* child */
//         /* restore previous signal actions & reset signal mask */
//         sigaction(SIGINT, &saveintr, NULL);
//         sigaction(SIGQUIT, &savequit, NULL);
//         sigprocmask(SIG_SETMASK, &savemask, NULL);
//         /*通常exec会放在fork() 函数的子进程部分, 来替代子进程执行啦, 执行成功后子程序就会消失,  但是执行失败的话, 必须用exit()函数来让子进程退出!*/
//         execl("/bin/sh",
//               "sh",
//               "-c",
//               cmdstring,
//               (char *) 0); /*exec函数会取代执行它的进程,  也就是说, 一旦exec函数执行成功, 它就不会返回了, 进程结束.   但是如果exec函数执行失败, 它会返回失败的信息,  而且进程继续执行后面的代码!*/
//         _exit(127);        /* exec error */
//     }

//     /* parent */
//     int ret = 0;
//     while (timeout-- > 0 && (ret = waitpid(pid, &status, WNOHANG)) == 0)
//         usleep(100 * 1000);

//     /* restore previous signal actions & reset signal mask */
//     if (sigaction(SIGINT, &saveintr, NULL) < 0)
//         return (-1);
//     if (sigaction(SIGQUIT, &savequit, NULL) < 0)
//         return (-1);
//     if (sigprocmask(SIG_SETMASK, &savemask, NULL) < 0)
//         return (-1);

//     if (ret < 0)
//         return -1;

//     if (ret > 0)
//         return status;

//     kill(pid, SIGKILL);
//     waitpid(pid, &status, 0);
//     return -2;
// }

// int OperateCmd::runSystem(const char *cmdstring, char *buff, int size)
// {
//     char cmd_string[256] = {0};
//     char tmpfile[200] = {0};
//     char tmp_buf[100] = {0};
//     int fd;
//     int tmp_fd;
//     int nbytes;

//     memset(buff, 0, size);

//     if ((cmdstring == NULL) || (strlen(cmdstring) > (sizeof(tmpfile) + 8))
//         || ((strlen(cmdstring) + strlen(tmpfile) + 5) > sizeof(cmd_string))) {
//         printf("cmd is too long or NULL!\n");
//         return -1;
//     }
//     sscanf(cmdstring, "%[a-Z]", tmp_buf);
//     sprintf(tmpfile, "/tmp/%s-XXXXXX", tmp_buf);

//     tmp_fd = mkstemp(tmpfile);
//     if (tmp_fd < 0) {
//         printf("mkstemp failed\n");
//         return -1;
//     }
//     close(tmp_fd);

//     sprintf(cmd_string, "%s > %s 2>&1", cmdstring, tmpfile);
//     if (system_ex(cmd_string, 20) < 0) {
//         printf("run \"%s\" ret < 0!\n", cmd_string);
//     }

//     fd = open(tmpfile, O_RDONLY);
//     if (fd < 0) {
//         printf("open %s failed!\n", tmpfile);
//         nbytes = -1;
//     } else {
//         nbytes = read(fd, buff, size - 1);
//         close(fd);
//     }

//     if ('\n' == buff[nbytes - 1]) {
//         buff[nbytes - 1] = '\0';
//     }

//     memset(cmd_string, 0, sizeof(cmd_string));
//     sprintf(cmd_string, "rm -rf /tmp/%s-*", tmp_buf);
//     system_ex(cmd_string, 20);

//     return nbytes;
// }

// void OperateCmd::cmdSleep(int n, bool bReal)
// {
//     if (bReal) {
//         QThread::msleep(n);
//     }
//     emit sndOptLog(0, QString("sleep %1 ms").arg(n), true);
// }

// void OperateCmd::cmdThread()
// {
//     while (m_bThread) {
//         int count = 0;
//         StI2CCmd cmd;
//         {
//             m_mtx.lock();
//             count = m_listCmd.size();
//             if (count > 0) {
//                 cmd = m_listCmd.front();
//                 m_listCmd.pop_front();
//             }
//             m_mtx.unlock();
//         }
//         if (count > 0) {
//             if (0 == cmd.cmdType) {
//                 if (cmd.bWrite) {
//                     writeData(cmd);
//                     if (cmd.cmd == def_cmd) {
//                         cmdSleep(10);
//                     }
//                     if (cmd.cmd == trgmode) {
//                         QThread::msleep(100);
//                     }
//                 } else {
//                     QString strReg = QString("0x%1").arg(cmd.reg, 2, 16, QLatin1Char('0'));
//                     readData(cmd);
//                     StCmdRet ret;
//                     ret.cmd = cmd.cmd;
//                     ret.data = cmd.data;
//                     ret.obj = cmd.obj;
//                     ret.show = cmd.showlog;
//                     ret.strReg = strReg;
//                     emit sndCmdRet(ret);
//                 }
//             } else {
//                 QString strCmd = cmd.sysCmd;
//                 int nLen = 1024;
//                 char szBuf[1024] = {0};
//                 m_mtx.lock();
//                 int ret = runSystem((char *) strCmd.toStdString().c_str(), szBuf, nLen);
//                 m_mtx.unlock();
//                 emit sndOptLog(ret, strCmd);
//                 if (0 != ret) {
//                     emit sndOptLog(ret, szBuf);
//                 }
//                 qDebug() << strCmd;
//             }
//         }

//         QThread::msleep(1);
//     }
// }
