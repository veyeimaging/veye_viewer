#include "ImgProcDlg.h"
#include <QDebug>
#include "CodeMap.h"
#include "QxToast.h"
#include "ui_ImgProcDlg.h"

ImgProcDlg::ImgProcDlg(CommDevCfg devCfg, QWidget *parent)
    : QDialog(parent)
    , m_devCfg(devCfg)
    , ui(new Ui::ImgProcDlg)
{
    ui->setupUi(this);
    setupUi();
}

ImgProcDlg::~ImgProcDlg()
{
    delete ui;
}

void ImgProcDlg::rcvUpdateUi()
{
    ui->retranslateUi(this);
}

void ImgProcDlg::rcvCmdRet(const StCmdRet &ret)
{
    int32_t data = ret.data;

    switch (ret.cmd) {
    case saturation:
        ui->spinBoxSaturation->setValue(data);
        break;
    case contrast:
        ui->spinBoxContrast->setValue(data);
        break;
    case hue:
        ui->spinBoxHue->setValue(data);
        break;   
    default:
        break;
    }
    updateStatus();
}

bool ImgProcDlg::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        //QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QSpinBox *spinBox = qobject_cast<QSpinBox *>(obj);
        if (spinBox && spinBox->isEnabled()) {
            int value = spinBox->value();
            EuCMD cmd;
            bool bFlag = true;
            if (spinBox->objectName() == ui->spinBoxSaturation->objectName()) {
                cmd = saturation;
            } else if (spinBox->objectName() == ui->spinBoxContrast->objectName()) {
                cmd = contrast;
            } else if (spinBox->objectName() == ui->spinBoxHue->objectName()) {
                cmd = hue;
            } else {
                bFlag = false;
            }
            if (bFlag) {
                emit sndCmd(cmd, value);
            }
        }
        QSlider *slider = qobject_cast<QSlider *>(obj);
        if (slider && slider->isEnabled()) {
            int value = slider->value();
            EuCMD cmd;
            bool bFlag = true;
            if (slider->objectName() == ui->horizontalSliderSaturation->objectName()) {
                cmd = saturation;
            } else if (slider->objectName() == ui->horizontalSliderContrast->objectName()) {
                cmd = contrast;
            } else if (slider->objectName() == ui->horizontalSliderHue->objectName()) {
                cmd = hue;
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

void ImgProcDlg::setupUi()
{
    setWindowTitle("ImgProc");
    setFocusPolicy(Qt::NoFocus);

    ui->spinBoxSaturation->installEventFilter(this);
    ui->spinBoxContrast->installEventFilter(this);
    ui->spinBoxHue->installEventFilter(this);

    ui->horizontalSliderSaturation->installEventFilter(this);
    ui->horizontalSliderContrast->installEventFilter(this);
    ui->horizontalSliderHue->installEventFilter(this);

    ui->horizontalSliderSaturation->setRange(0, 100);
    ui->horizontalSliderContrast->setRange(0, 100);
    ui->horizontalSliderHue->setRange(0, 100);

    connect(ui->horizontalSliderSaturation,
            &QSlider::valueChanged,
            ui->spinBoxSaturation,
            &QSpinBox::setValue);
    connect(ui->spinBoxSaturation,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSliderSaturation,
            &QSlider::setValue);

    connect(ui->spinBoxSaturation, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBoxSaturation->value();
        emit sndCmd(saturation, value);
    });

    connect(ui->horizontalSliderContrast,
            &QSlider::valueChanged,
            ui->spinBoxContrast,
            &QSpinBox::setValue);
    connect(ui->spinBoxContrast,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSliderContrast,
            &QSlider::setValue);
    connect(ui->spinBoxContrast, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBoxContrast->value();
        emit sndCmd(contrast, value);
    });

    connect(ui->horizontalSliderHue, &QSlider::valueChanged, ui->spinBoxHue, &QSpinBox::setValue);
    connect(ui->spinBoxHue,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSliderHue,
            &QSlider::setValue);

    connect(ui->spinBoxHue, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBoxHue->value();
        emit sndCmd(hue, value);
    });
}

void ImgProcDlg::updateStatus() {}
