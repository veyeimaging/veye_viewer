#ifndef CURATTRDLG_H
#define CURATTRDLG_H

#include <QDialog>
#include "commstuct.h"

namespace Ui {
class CurAttrDlg;
}

class CurAttrDlg : public QDialog
{
    Q_OBJECT

public:
    explicit CurAttrDlg(QWidget *parent = nullptr);
    ~CurAttrDlg();
public slots:
    void rcvUpdateUi();
    void rcvCmdRet(const StCmdRet &ret);

public:
    void initGUI();
    void initCNT();

private:
    Ui::CurAttrDlg *ui;
    int m_width = 0;
    int m_height = 0;
};

#endif // CURATTRDLG_H
