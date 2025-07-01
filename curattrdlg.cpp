#include "curattrdlg.h"
#include "codemap.h"
#include "ui_curattrdlg.h"

CurAttrDlg::CurAttrDlg(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CurAttrDlg)
{
    ui->setupUi(this);
    initGUI();
    initCNT();
}

CurAttrDlg::~CurAttrDlg()
{
    delete ui;
}

void CurAttrDlg::rcvUpdateUi()
{
    ui->retranslateUi(this);
}

void CurAttrDlg::rcvCmdRet(const StCmdRet &ret)
{
    uint32_t data = ret.data;
    switch (ret.cmd) {
    case roi_w:
        m_width = data;
        ui->lineEdit_roi_w_h->setText(QString("%1 X %2").arg(m_width).arg(m_height));
        break;
    case roi_h:
        m_height = data;
        ui->lineEdit_roi_w_h->setText(QString("%1 X %2").arg(m_width).arg(m_height));
        break;
    case fps:
        ui->lineEdit_fps->setText(QString::number(data / 100.0));
        break;
    case pixelformat:
        ui->lineEdit_pixelformat->setText(codemap::getPixelFormat(data));
        break;
    case trgmode:
        ui->lineEdit_trgmode->setText(codemap::getTrgMode(data));
        {
            switch (data) {
            case Stream:
                ui->lineEdit_trgmode->setText(tr("流模式"));
                break;
            case Normal:
                ui->lineEdit_trgmode->setText(tr("普通触发"));
                break;
            case Rolling:
                ui->lineEdit_trgmode->setText(tr("滚动快门多帧触发"));
                break;
            case Pulse:
                ui->lineEdit_trgmode->setText(tr("电平触发"));
                break;
            default:
                break;
            }
        }
        break;
    case trgsrc:
        //ui->lineEdit_trgsrc->setText(codemap::getTrgSrc(data));
        if (0 == data) {
            ui->lineEdit_trgsrc->setText(tr("软触发"));
        } else {
            ui->lineEdit_trgsrc->setText(tr("硬触发"));
        }
        break;
    default:
        break;
    }
}

void CurAttrDlg::initGUI()
{
    ui->lineEdit_roi_w_h->setEnabled(false);
    ui->lineEdit_fps->setEnabled(false);
    ui->lineEdit_pixelformat->setEnabled(false);
    ui->lineEdit_trgmode->setEnabled(false);
    ui->lineEdit_trgsrc->setEnabled(false);
}

void CurAttrDlg::initCNT() {}
