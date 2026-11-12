#ifndef TRIGGERDLG_H
#define TRIGGERDLG_H

#include <QDialog>
#include <QMouseEvent>
#include "CommStuct.h"

namespace Ui {
class TriggerDlg;
}

class TriggerDlg : public QDialog
{
    Q_OBJECT

public:
    explicit TriggerDlg(CommDevCfg devCfg, QWidget *parent = nullptr);
    ~TriggerDlg();
    // 设备类型/平台/通道号会随“选设备、切通道”变化，不能只在构造时快照一次
    void setDevCfg(CommDevCfg devCfg)
    {
        m_devCfg = devCfg;
    }
public slots:
    void rcvUpdateUi();
    void rcvCmdRet(const StCmdRet &ret);
signals:
    void sndCmd(EuCMD cmd, uint32_t data, bool bShow = true);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

protected:
    void setupUi();    
    void updateStatus();
    QMap<int, int> m_mapDataIndex;

private:
    Ui::TriggerDlg *ui;
    CommDevCfg m_devCfg;
};

#endif // TRIGGERDLG_H
