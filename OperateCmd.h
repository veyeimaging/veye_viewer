// #ifndef OPERATECMD_H
// #define OPERATECMD_H
// #include <QObject>
// #include <QThread>
// #include "CodeMap.h"
// #include "CommHeader.h"
// #include "CommStuct.h"
// #include <arpa/inet.h>
// #include <ctype.h>
// #include <errno.h>
// #include <fcntl.h>
// #include <libgen.h>
// #include <linux/i2c-dev.h>
// #include <linux/i2c.h>
// #include <mutex>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/ioctl.h>
// #include <unistd.h>

// struct sensor_regs
// {
//     uint16_t reg;
//     uint32_t data;
// };

// #pragma pack(push, 1)
// struct preread_regs
// {
//     uint8_t pre_head;
//     uint16_t reg;
//     uint8_t xor_check;
// };
// struct write_regs
// {
//     uint8_t head;
//     uint16_t reg;
//     uint32_t data;
//     uint8_t xor_check;
// };
// #pragma pack(pop)

// class OperateCmd : public QObject
// {
//     Q_OBJECT
// public:
//     explicit OperateCmd(QObject *parent = nullptr);
//     ~OperateCmd();
// signals:
//     void sndOptLog(int ret, QString strInfo, bool show = true);
//     void sndCmdRet(const StCmdRet &ret);
// public:
//     void runSystemCmd(QString strCmd);
//     void asynReadData(StI2CCmd cmd);
//     void asynWriteDate(StI2CCmd cmd);
//     void syncReadData(StI2CCmd &cmd);

// protected:
//     int readData(StI2CCmd &cmd);
//     int writeData(StI2CCmd &cmd);
// protected:
//     void systemCmd(QString strCmd);
//     int system_ex(const char *cmdstring, unsigned int timeout);
//     int runSystem(const char *cmdstring, char *buff, int size);

// protected:
//     void cmdSleep(int n, bool bReal = true);
//     void cmdThread();
// protected:
//     bool m_bThread = true;

// protected:
//     std::list<StI2CCmd> m_listCmd;
//     std::mutex m_mtx;
// };

// #endif // OPERATECMD_H
