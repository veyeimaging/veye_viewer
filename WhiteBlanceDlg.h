#ifndef WHITEBLANCEDLG_H
#define WHITEBLANCEDLG_H

#include <QDialog>
#include <QMouseEvent>
#include "CommStuct.h"

namespace Ui {
class WhiteBlanceDlg;
}

class WhiteBlanceDlg : public QDialog
{
    Q_OBJECT

public:
    explicit WhiteBlanceDlg(CommDevCfg devCfg, QWidget *parent = nullptr);
    ~WhiteBlanceDlg();
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
    Ui::WhiteBlanceDlg *ui;
    CommDevCfg m_devCfg;
};

#endif // WHITEBLANCEDLG_H
