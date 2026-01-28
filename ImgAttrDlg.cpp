#include "ImgAttrDlg.h"
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include "MainWidget.h"
#include "QxHelp.h"
#include "QxToast.h"
#include "ui_ImgAttrDlg.h"

ImgAttrDlg::ImgAttrDlg(CommDevCfg devCfg, QWidget *parent)
    : QDialog(parent)
    , m_devCfg(devCfg)
    , ui(new Ui::ImgAttrDlg)
{
    ui->setupUi(this);
    setupUi();
}

ImgAttrDlg::~ImgAttrDlg()
{
    delete ui;
}

void ImgAttrDlg::rcvUpdateUi()
{
    ui->retranslateUi(this);
}

void ImgAttrDlg::rcvSetROI(bool read)
{
    onSetRoiAndFpsData(read);
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
        if (data > 0) {
            ui->lineEditFPSMV->setText(QString::number(data / 100.0));
        }
        break;
    case fps_gx:
        if (data > 0) {
            ui->lineEditFPSGX->setText(QString::number(data / 10000.0));
        }
        break;
    case videomode:
        ui->comboBoxModeNum->activated(data);
        break;
    case videomodenum: {
        if ((data < 8) && (data > 0)) {
            ui->comboBoxModeNum->clear();
            for (uint32_t i = 0; i < data; ++i) {
                QString item = QString(tr("模式 ")) + QString::number(i + 1);
                if ((ui->comboBoxModeNum->findText(item) < 0) && (item != "0 ∗ 0")) {
                    ui->comboBoxModeNum->addItem(item);
                }
                emit readCmd(EuCMD(videomodenum + i * 2 + 1), this, true);
                emit readCmd(EuCMD(videomodenum + i * 2 + 2), this, true);
            }
            emit readCmd(videomode, this, true);
        }
    } break;
    case videomodewh1:
    case videomodewh2:
    case videomodewh3:
    case videomodewh4:
    case videomodewh5:
    case videomodewh6:
    case videomodewh7:
    case videomodewh8: {    
        if (data > 0) {
            const auto [w, h] = parseWidthHeight(data);
            int key = ret.cmd - videomodewh1;
            readMode rm;
            if (m_readModes.contains(key)) {
                rm = m_readModes[key];
            }
            rm.width = QString::number(w);
            rm.height = QString::number(h);
            m_readModes[key] = rm;
        }

    } break;
    case videomodeparam1:
    case videomodeparam2:
    case videomodeparam3:
    case videomodeparam4:
    case videomodeparam5:
    case videomodeparam6:
    case videomodeparam7:
    case videomodeparam8: {
        if (data > 0) {
            int key = ret.cmd - videomodeparam1;
            readMode rm;
            if (m_readModes.contains(key)) {
                rm = m_readModes[key];
            }
            const auto [mode, fps] = getParam(data);
            rm.fps = QString::number(fps);
            switch (mode) {
            case 0:
                rm.mode = tr("普通");
                break;
            case 1:
                rm.mode = "2x2 binning";
                break;
            case 2:
                rm.mode = "subsampling";
                break;
            default:
                break;
            }
            m_readModes[key] = rm;
        }
    } break;
    default:
        break;
    }
}

void ImgAttrDlg::setupUi()
{
    ui->lineEditX->setText("0");
    ui->lineEditY->setText("0");
    ui->lineEditW->setText("0");
    ui->lineEditH->setText("0");
    ui->lineEditFPSMV->setText("0");
    ui->lineEditFPSGX->setText("0");
    ui->widgetMV->hide();
    ui->widgetGX->hide();
    switch (m_devCfg.cameType) {
    case EuCamType::GxCamera:
        ui->widgetGX->show();
        break;
    case EuCamType::MvCamera:
        ui->widgetMV->show();
        break;
    default:
        break;
    }

    connect(ui->pushButtonSetRoiAndFps, &QPushButton::clicked, this, [this]() {
        onSetRoiAndFpsData(true);
    });
    connect(ui->pushButtonSetResolution, &QPushButton::clicked, this, [this]() {
        onSetRoiAndFpsData(true);
    });

    connect(ui->comboBoxModeNum, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        int key = index * 2;
        if (m_readModes.contains(key)) {
            readMode rm = m_readModes.value(key);
            ui->labelReadModeValue->setText(rm.mode);
            ui->labelFps->setText(rm.fps);
            ui->labelResolutionValue->setText(rm.width + " ∗ " + rm.height);
            QxHelp::setDoubleLineEdit(ui->lineEditFPSGX, 0.00, rm.fps.toInt());
            ui->comboBoxModeNum->setCurrentIndex(index);
        }
    });
}

void ImgAttrDlg::onSetRoiAndFpsData(bool read)
{
    StImgAttrInfo info;
    info.read = read;
    if (EuCamType::MvCamera == m_devCfg.cameType) {
        info.strX = ui->lineEditX->text();
        info.strY = ui->lineEditY->text();
        info.strW = ui->lineEditW->text();
        info.strH = ui->lineEditH->text();
        info.strFps = ui->lineEditFPSMV->text();
        if (!info.strX.isEmpty() && !info.strY.isEmpty() && !info.strW.isEmpty()
            && !info.strH.isEmpty() && !info.strFps.isEmpty()) {
            info.strFps = QString::number(info.strFps.toFloat(), 'f', 0);
            qDebug() << info.strFps;
            emit setRoiAndFpsData(info);
        } else {
            QxToast::showTip(tr("请设置合理的ROI和PFS"));
        }
    } else if (EuCamType::GxCamera == m_devCfg.cameType) {
        info.strX = "0";
        info.strY = "0";
        int key = ui->comboBoxModeNum->currentIndex() * 2;
        if (m_readModes.contains(key)) {
            info.strW = m_readModes[key].width;
            info.strH = m_readModes[key].height;
            info.strFps = ui->lineEditFPSGX->text();
        }
        if (!info.strW.isEmpty() && !info.strH.isEmpty() && !info.strFps.isEmpty()) {
            info.strFps = QString::number(info.strFps.toFloat(), 'f', 0);
            emit setRoiAndFpsData(info);
        } else {
            QxToast::showTip(tr("请设置合理分辨率和帧率"));
        }
    }
}

std::pair<uint8_t, uint16_t> ImgAttrDlg::getParam(uint32_t data)
{
    const uint8_t mode = static_cast<uint8_t>((data >> 16) & 0x00FF);
    const uint16_t fps = static_cast<uint16_t>(data & 0xFFFF);

    return {mode, fps};
}

std::pair<uint16_t, uint16_t> ImgAttrDlg::parseWidthHeight(uint32_t data)
{
    // - 宽度：高16位 → 右移16位（>> 16）
    // - 高度：低16位 → 与0xFFFF按位与（& 0xFFFF）
    const uint16_t width = static_cast<uint16_t>((data >> 16) & 0xFFFF);
    const uint16_t height = static_cast<uint16_t>(data & 0xFFFF);

    return {width, height};
}
