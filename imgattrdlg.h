#ifndef IMGATTRDLG_H
#define IMGATTRDLG_H

#include <QDialog>
#include "commstuct.h"

namespace Ui {
class ImgAttrDlg;
}

class ImgAttrDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ImgAttrDlg(QWidget *parent = nullptr);
    ~ImgAttrDlg();
signals:
    void sndData(StImgAttrInfo &info);
public slots:
    void rcvUpdateUi();
    void rcvCmdRet(const StCmdRet &ret);

protected:
    void initGUI();
    void initCNT();

private:
    Ui::ImgAttrDlg *ui;
};

#endif // IMGATTRDLG_H
