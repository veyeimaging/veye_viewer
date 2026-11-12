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
    // 设备类型/平台/通道号会随“选设备、切通道”变化，不能只在构造时快照一次
    void setDevCfg(CommDevCfg devCfg)
    {
        m_devCfg = devCfg;
    }
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
