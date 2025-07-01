#include "imgattrdlg.h"
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include "qxtoast.h"
#include "ui_imgattrdlg.h"

ImgAttrDlg::ImgAttrDlg(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ImgAttrDlg)
{
    ui->setupUi(this);
    initGUI();
    initCNT();
}

ImgAttrDlg::~ImgAttrDlg()
{
    delete ui;
}

void ImgAttrDlg::rcvUpdateUi()
{
    ui->retranslateUi(this);
}

void ImgAttrDlg::rcvCmdRet(const StCmdRet &ret)
{
    uint32_t data = ret.data;
    switch (ret.cmd) {
    case roi_x:
        ui->lineEditX->setText(QString::number(data));
        break;
    case roi_y:
        ui->lineEditY->setText(QString::number(data));
        break;
    case roi_w:
        ui->lineEditW->setText(QString::number(data));
        break;
    case roi_h:
        ui->lineEditH->setText(QString::number(data));
        break;
    case fps:
        ui->lineEditFPS->setText(QString::number(data / 100.0));
        break;
    default:
        break;
    }
}

void ImgAttrDlg::initGUI()
{
    ui->lineEditX->setText("0");
    ui->lineEditY->setText("0");
    ui->lineEditW->setText("0");
    ui->lineEditH->setText("0");
    ui->lineEditFPS->setText("0");
}

void ImgAttrDlg::initCNT()
{
    connect(ui->pushButtonSetRoiAndFps, &QPushButton::clicked, this, [this]() {
        StImgAttrInfo info;
        info.strX = ui->lineEditX->text();
        info.strY = ui->lineEditY->text();
        info.strW = ui->lineEditW->text();
        info.strH = ui->lineEditH->text();
        info.strFps = ui->lineEditFPS->text();
        if (!info.strX.isEmpty() && !info.strY.isEmpty() && !info.strW.isEmpty()
            && !info.strH.isEmpty() && !info.strFps.isEmpty()) {
            info.strFps = QString::number((int) info.strFps.toFloat());
            qDebug() << info.strFps;
            emit sndData(info);
        } else {
            QxToast::showTip(tr("请设置合理的ROI和PFS"));
        }
    });
}
