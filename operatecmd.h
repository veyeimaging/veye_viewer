#ifndef OPERATECMD_H
#define OPERATECMD_H
#include <QObject>
#include <QThread>
#include "codemap.h"
#include "commheader.h"
#include "commstuct.h"
#include <mutex>

class OperateCmd : public QObject
{
    Q_OBJECT
public:
    explicit OperateCmd(QObject *parent = nullptr);
    ~OperateCmd();
signals:
    void sndOptLog(int nCode, QString strInfo, bool show = true);
    void sndCmdRet(const StCmdRet &ret);

public:
    //nType: 0:i2c, 1:media, 2:video, 3:subNode
    void openDev(QString &deviceName, int nType, int &nBufType);

public:
    void runSystemCmd(QString strCmd);

    void systemCmd(QString strCmd);

    void mediaRunCmd(QString mediaNode, QString strParam);

    void asynReadData(StI2CCmd cmd);
    void syncReadData(QString i2c, uint32_t addr, uint32_t reg, uint32_t &data, bool showLog = true);
    void asynWriteDate(StI2CCmd cmd);

protected:
    int readData(QString i2c, uint32_t addr, uint32_t reg, uint32_t &data, bool showLog = true);

    int writeDate(QString i2c, uint32_t addr, uint32_t reg, uint32_t data, bool showLog = true);

protected:
    int system_ex(const char *cmdstring, unsigned int timeout);
    int runSystem(const char *cmdstring, char *buff, int size);

protected:
    void cmdSleep(int n, bool bReal = true);
    void cmdThread();

protected:
    bool m_bThread = true;

protected:
    std::list<StI2CCmd> m_listCmd;
    std::mutex m_mtx;
};

#endif // OPERATECMD_H
