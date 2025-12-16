#include "CurAttrDlg.h"
#include "CodeMap.h"
#include "ui_CurAttrDlg.h"

CurAttrDlg::CurAttrDlg(CommDevCfg devCfg, QWidget *parent)
    : QDialog(parent)
    , m_devCfg(devCfg)
    , ui(new Ui::CurAttrDlg)
{
    ui->setupUi(this);
    setupUi();
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
        ui->lineEdit_roi_w_h->setText(QString("%1 ∗ %2").arg(m_width).arg(m_height));
        break;
    case roi_h:
        m_height = data;
        ui->lineEdit_roi_w_h->setText(QString("%1 ∗ %2").arg(m_width).arg(m_height));
        break;
    case fps:
        ui->lineEdit_fps->setText(QString::number(data / 100.0));
        break;
    case fps_gx:
        ui->lineEdit_fps->setText(QString::number(data / 10000.0));
        break;
    case maxfps:
        ui->lineEditMaxFps->setText(QString::number(data / 100.0));
        break;
    case pixelformat:
        ui->lineEdit_pixelformat->setText(CodeMap::getPixelFormat(data));
        break;
    case trgmode:
        ui->lineEdit_trgmode->setText(CodeMap::getTrgMode(data));
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
            case CamSync:
                ui->lineEdit_trgmode->setText(tr("多相机同步模式"));
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
    case readmode:
        //ui->lineEdit_trgsrc->setText(codemap::getTrgSrc(data));
        {
            QString mode;
            switch (data) {
            case 0:
                mode = tr("普通");
                break;
            case 1:
                mode = "2x2 binning";
                break;
            case 2:
                mode = "subsampling";
                break;
            default:
                break;
            }
            ui->lineEditReadMode->setText(mode);
        }
        break;
    default:
        break;
    }
}

void CurAttrDlg::setupUi()
{
    ui->lineEdit_roi_w_h->setEnabled(false);
    ui->lineEdit_fps->setEnabled(false);
    ui->lineEdit_pixelformat->setEnabled(false);
    ui->lineEdit_trgmode->setEnabled(false);
    ui->lineEdit_trgsrc->setEnabled(false);
    ui->lineEditReadMode->setEnabled(false);
    ui->lineEditMaxFps->setEnabled(false);

    ui->label_trgsrc->hide();
    ui->lineEdit_trgsrc->hide();

    if (EuCamType::MvCamera == m_devCfg.cameType) {
        ui->label_MaxFps->hide();
        ui->lineEditMaxFps->hide();
        ui->labelReadMode2->hide();
        ui->lineEditReadMode->hide();
    }
}
