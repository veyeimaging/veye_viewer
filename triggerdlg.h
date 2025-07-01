#ifndef TRIGGERDLG_H
#define TRIGGERDLG_H

#include <QDialog>
#include <QMouseEvent>
#include "commstuct.h"

namespace Ui {
class TriggerDlg;
}

class TriggerDlg : public QDialog
{
    Q_OBJECT

public:
    explicit TriggerDlg(QWidget *parent = nullptr);
    ~TriggerDlg();
public slots:
    void rcvUpdateUi();
    void rcvCmdRet(const StCmdRet &ret);
    void rcvRet(EuCMD cmd, uint32_t data);
signals:
    void sndCmd(EuCMD cmd, uint32_t data, bool bShow = true);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

protected:
    void initGUI();
    void initCNT();
    void updateStatus();
    QMap<int, int> m_mapDataIndex;

private:
    Ui::TriggerDlg *ui;
};

#endif // TRIGGERDLG_H
