#ifndef REGISTERDLG_H
#define REGISTERDLG_H

#include <QDialog>
#include "CommHeader.h"
#include "CommStuct.h"
namespace Ui {
class RegisterDlg;
}

class RegisterDlg : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDlg(CommDevCfg devCfg, QWidget *parent = nullptr);
    ~RegisterDlg();

public:
    void setCfg(QMap<QString, StRegInfo> nameReg, QMap<uint32_t, StRegInfo> addrReg);
signals:
    void sndReg(const StRegInfo &info, bool bWrite, bool bShow = true);
    void sndCmd(EuCMD cmd, uint32_t data, bool bShow = true);
    void sndExport();
    void sndImport();
public slots:
    void rcvUpdateUi();
    void rcvCmdRet(const StCmdRet &ret);

protected:
    void setupUi();

protected:
    void updateReg();

private:
    Ui::RegisterDlg *ui;
    QString m_strCfgPath;
    QMap<QString, StRegInfo> m_mapNameRegister;
    QMap<uint32_t, StRegInfo> m_mapAddrRegister;
    CommDevCfg m_devCfg;
};

#endif // REGISTERDLG_H
