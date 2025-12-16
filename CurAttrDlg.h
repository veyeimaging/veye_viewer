#ifndef CURATTRDLG_H
#define CURATTRDLG_H

#include <QDialog>
#include "CommStuct.h"

namespace Ui {
class CurAttrDlg;
}

class CurAttrDlg : public QDialog
{
    Q_OBJECT

public:
    explicit CurAttrDlg(CommDevCfg devCfg, QWidget *parent = nullptr);
    ~CurAttrDlg();
public slots:
    void rcvUpdateUi();
    void rcvCmdRet(const StCmdRet &ret);

public:
    void setupUi();

private:
    Ui::CurAttrDlg *ui;
    int m_width = 0;
    int m_height = 0;
    CommDevCfg m_devCfg;
};

#endif // CURATTRDLG_H
