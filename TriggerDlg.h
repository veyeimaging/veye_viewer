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
