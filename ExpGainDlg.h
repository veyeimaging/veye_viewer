#ifndef EXPGAINDLG_H
#define EXPGAINDLG_H

#include <QDialog>
#include <QEvent>
#include "CommStuct.h"

namespace Ui {
class ExpGainDlg;
}

class ExpGainDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ExpGainDlg(CommDevCfg devCfg, QWidget *parent = nullptr);
    ~ExpGainDlg();
    // 设备类型/平台/通道号会随“选设备、切通道”变化，不能只在构造时快照一次
    void setDevCfg(CommDevCfg devCfg)
    {
        m_devCfg = devCfg;
    }
signals:
    void sndCmd(EuCMD cmd, uint32_t data, bool bShow = true);
    void sndOneAEAG(int nAemaxtime, int nAgmaxgain, int nAatarget);
public slots:
    void rcvUpdateUi();
    void rcvCmdRet(const StCmdRet &ret);
    void rcvValue(int exp, int gain);
    void rcvModel(int exp, int gain);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

protected:
    void setupUi();

private:
    Ui::ExpGainDlg *ui;
    bool m_bSetting = false;
    CommDevCfg m_devCfg;
};

#endif // EXPGAINDLG_H
