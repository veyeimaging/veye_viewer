#ifndef IMGPROCDLG_H
#define IMGPROCDLG_H

#include <QDialog>
#include <QMouseEvent>
#include "CommStuct.h"

namespace Ui {
class ImgProcDlg;
}

class ImgProcDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ImgProcDlg(CommDevCfg devCfg, QWidget *parent = nullptr);
    ~ImgProcDlg();
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
private:
    Ui::ImgProcDlg *ui;
    CommDevCfg m_devCfg;
};

#endif // IMGPROCDLG_H
