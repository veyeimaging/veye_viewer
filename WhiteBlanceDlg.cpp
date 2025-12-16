#include "WhiteBlanceDlg.h"
#include <QDebug>
#include <QRegularExpression>
#include "CodeMap.h"
#include "QxHelp.h"
#include "QxToast.h"
#include "ui_WhiteBlanceDlg.h"

WhiteBlanceDlg::WhiteBlanceDlg(CommDevCfg devCfg, QWidget *parent)
    : QDialog(parent)
    , m_devCfg(devCfg)
    , ui(new Ui::WhiteBlanceDlg)
{
    ui->setupUi(this);
    setupUi();
}

WhiteBlanceDlg::~WhiteBlanceDlg()
{
    delete ui;
}

void WhiteBlanceDlg::rcvUpdateUi()
{
    ui->retranslateUi(this);
    ui->comboBoxWbMode->clear();
    ui->comboBoxWbMode->addItem(tr("手动白平衡"), MWB);
    ui->comboBoxWbMode->addItem(tr("自动白平衡"), AWB);

    {
        // const QRegularExpression labelRegex(
        //     "^label(X|Y|W|H|FpsMv|FpsGx|MaxFpsGx|Resolution|ReadMode|CurExp|CurGain|"
        //     "Aatarget|MaxExp|MaxGain|ManualExp|ManualGain|_trgexp_delay|_trgnum|"
        //     "_trginterval|_trgfilter_time|_trgdelay|_roi_w_h|_fps|_MaxFps|_pixelformat|"
        //     "ReadMode2|Addr|Value|ColorTemp|CurRGain|CurBGain|AWbColorTempMin|"
        //     "AWbColorTempMax|MBbRGain|MBbBGain|Saturation|Contrast|Hue)$");

        // const QList<QLabel *> targetLabels = this->findChildren<QLabel *>();
        // for (QLabel *lbl : targetLabels) {
        //     if (lbl) {
        //         QString str = lbl->text();
        //         QFontMetrics fontWidth(lbl->font());
        //         QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, 120);
        //         lbl->setText(elideNote);
        //         lbl->setToolTip(str);
        //     }
        // }
    }
    QxHelp::batchSetLabelElidedText(this, QRegularExpression(), Qt::ElideRight, 120);
}

void WhiteBlanceDlg::rcvCmdRet(const StCmdRet &ret)
{
    int32_t data = ret.data;

    switch (ret.cmd) {
    case wbmode: {
        if (data == MWB) {
            ui->comboBoxWbMode->setCurrentIndex(0);
        } else if (data == AWB) {
            ui->comboBoxWbMode->setCurrentIndex(1);
        }
        updateStatus();
    } break;
    case awbcolortempmin:
        ui->spinBoxAWbColorTempMin->setValue(data);
        break;
    case awbcolortempmax:
        ui->spinBoxAWbColorTempMax->setValue(data);
        break;
    case mwbrgain:
        ui->spinBoxMBbRGain->setValue(data);
        break;
    case mwbbgain:
        ui->spinBoxMBbBGain->setValue(data);
        break;
    case colortemp:
        ui->labelColorTempValue->setText(QString::number(data));
        break;
    case currgain:
        ui->labelCurRGainValue->setText(QString::number(data));
        break;
    case curbgain:
        ui->labelCurBGainValue->setText(QString::number(data));
        break;
    default:
        break;
    }
}

bool WhiteBlanceDlg::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        QSpinBox *spinBox = qobject_cast<QSpinBox *>(obj);
        if (spinBox && spinBox->isEnabled()) {
            int value = spinBox->value();
            EuCMD cmd;
            bool bFlag = true;
            if (spinBox->objectName() == ui->spinBoxAWbColorTempMin->objectName()) {
                cmd = awbcolortempmin;
            } else if (spinBox->objectName() == ui->spinBoxAWbColorTempMax->objectName()) {
                cmd = awbcolortempmax;
            } else if (spinBox->objectName() == ui->spinBoxMBbBGain->objectName()) {
                cmd = mwbbgain;
            } else if (spinBox->objectName() == ui->spinBoxMBbRGain->objectName()) {
                cmd = mwbrgain;
            } else {
                bFlag = false;
            }
            if (bFlag) {
                emit sndCmd(cmd, value);
            }
            qDebug() << "WhiteBlanceDlg" << obj->objectName();
        }
        QSlider *slider = qobject_cast<QSlider *>(obj);
        if (slider && slider->isEnabled()) {
            int value = slider->value();
            EuCMD cmd;
            bool bFlag = true;
            if (slider->objectName() == ui->horizontalSliderAWbColorTempMin->objectName()) {
                cmd = awbcolortempmin;
            } else if (slider->objectName() == ui->horizontalSliderAWbColorTempMax->objectName()) {
                cmd = awbcolortempmax;
            } else if (slider->objectName() == ui->horizontalSliderMBbBGain->objectName()) {
                cmd = mwbbgain;
            } else if (slider->objectName() == ui->horizontalSliderMBbRGain->objectName()) {
                cmd = mwbrgain;
            } else {
                bFlag = false;
            }
            if (bFlag) {
                emit sndCmd(cmd, value);
            }
        }
    }
    return QObject::eventFilter(obj, event);
}

void WhiteBlanceDlg::setupUi()
{
    setFocusPolicy(Qt::NoFocus);

    ui->comboBoxWbMode->addItem(tr("手动白平衡"), MWB);
    ui->comboBoxWbMode->addItem(tr("自动白平衡"), AWB);

    ui->spinBoxAWbColorTempMin->installEventFilter(this);
    ui->spinBoxAWbColorTempMax->installEventFilter(this);
    ui->spinBoxMBbBGain->installEventFilter(this);
    ui->spinBoxMBbRGain->installEventFilter(this);

    ui->horizontalSliderAWbColorTempMin->installEventFilter(this);
    ui->horizontalSliderAWbColorTempMax->installEventFilter(this);
    ui->horizontalSliderMBbBGain->installEventFilter(this);
    ui->horizontalSliderMBbRGain->installEventFilter(this);

    ui->horizontalSliderMBbBGain->setRange(0, 4095);
    ui->spinBoxMBbBGain->setRange(0, 4095);
    ui->horizontalSliderMBbRGain->setRange(0, 4095);
    ui->spinBoxMBbRGain->setRange(0, 4095);

    ui->spinBoxAWbColorTempMin->setRange(1000, 15000);
    ui->horizontalSliderAWbColorTempMin->setRange(1000, 15000);
    ui->spinBoxAWbColorTempMax->setRange(1000, 15000);
    ui->horizontalSliderAWbColorTempMax->setRange(1000, 15000);

    QxHelp::batchSetLabelElidedText(this, QRegularExpression(), Qt::ElideRight, 120);

    connect(ui->comboBoxWbMode, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        int cmd = ui->comboBoxWbMode->currentData().toInt();
        emit sndCmd(wbmode, cmd);
        updateStatus();
    });

    connect(ui->horizontalSliderAWbColorTempMin,
            &QSlider::valueChanged,
            ui->spinBoxAWbColorTempMin,
            &QSpinBox::setValue);
    connect(ui->spinBoxAWbColorTempMin,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSliderAWbColorTempMin,
            &QSlider::setValue);

    connect(ui->spinBoxAWbColorTempMin, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBoxAWbColorTempMin->value();
        emit sndCmd(awbcolortempmin, value);
    });

    connect(ui->horizontalSliderAWbColorTempMax,
            &QSlider::valueChanged,
            ui->spinBoxAWbColorTempMax,
            &QSpinBox::setValue);
    connect(ui->spinBoxAWbColorTempMax,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSliderAWbColorTempMax,
            &QSlider::setValue);
    connect(ui->spinBoxAWbColorTempMax, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBoxAWbColorTempMax->value();
        emit sndCmd(awbcolortempmax, value);
    });

    connect(ui->horizontalSliderMBbBGain,
            &QSlider::valueChanged,
            ui->spinBoxMBbBGain,
            &QSpinBox::setValue);
    connect(ui->spinBoxMBbBGain,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSliderMBbBGain,
            &QSlider::setValue);

    connect(ui->spinBoxMBbBGain, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBoxMBbBGain->value();
        emit sndCmd(mwbbgain, value);
    });

    connect(ui->horizontalSliderMBbRGain,
            &QSlider::valueChanged,
            ui->spinBoxMBbRGain,
            &QSpinBox::setValue);
    connect(ui->spinBoxMBbRGain,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSliderMBbRGain,
            &QSlider::setValue);

    connect(ui->spinBoxMBbRGain, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBoxMBbRGain->value();
        emit sndCmd(mwbrgain, value);
    });
}

void WhiteBlanceDlg::updateStatus()
{
    int curModel = ui->comboBoxWbMode->currentData().toInt();
    ui->widget_A->setEnabled(false);
    ui->widget_M->setEnabled(false);
    switch (curModel) {
    case MWB: {
        ui->widget_M->setEnabled(true);
    } break;
    case AWB: {
        ui->widget_A->setEnabled(true);
    } break;
    default:
        break;
    }
}
