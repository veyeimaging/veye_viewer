#ifndef REGISTERDLG_H
#define REGISTERDLG_H

#include <QDialog>
#include "commheader.h"
#include "commstuct.h"
namespace Ui {
class RegisterDlg;
}

class RegisterDlg : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDlg(QWidget *parent = nullptr);
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
    void initGUI();
    void initCNT();

protected:
    void updateReg();

private:
    Ui::RegisterDlg *ui;
    QString m_strCfgPath;
    QMap<QString, StRegInfo> m_mapNameRegister;
    QMap<uint32_t, StRegInfo> m_mapAddrRegister;
};

#endif // REGISTERDLG_H
