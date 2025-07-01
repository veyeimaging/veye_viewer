#include "triggerdlg.h"
#include <QDebug>
#include "codemap.h"
#include "qxtoast.h"
#include "ui_triggerdlg.h"

TriggerDlg::TriggerDlg(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TriggerDlg)
{
    ui->setupUi(this);
    initGUI();
    initCNT();
}

TriggerDlg::~TriggerDlg()
{
    delete ui;
}

void TriggerDlg::rcvUpdateUi()
{
    ui->retranslateUi(this);
}

void TriggerDlg::rcvCmdRet(const StCmdRet &ret)
{
    uint32_t data = ret.data;

    switch (ret.cmd) {
    case trgmodecap: {
        ui->comboBox_trgmode->clear();
        m_mapDataIndex.clear();
        int index = 0;
        if (data & 1) {
            ui->comboBox_trgmode->addItem(tr("流模式"), Stream);
            m_mapDataIndex[Stream] = index++;
        }
        if (data & (1 << 1)) {
            ui->comboBox_trgmode->addItem(tr("普通触发"), Normal);
            m_mapDataIndex[Normal] = index++;
        }
        if (data & (1 << 3)) {
            ui->comboBox_trgmode->addItem(tr("电平触发"), Pulse);
            m_mapDataIndex[Pulse] = index++;
        }
        if (data & (1 << 2)) {
            ui->comboBox_trgmode->addItem(tr("滚动快门多帧触发"), Rolling);
            m_mapDataIndex[Rolling] = index++;
        }
        ui->comboBox_trgsrc->clear();
        ui->comboBox_trgsrc->addItem(tr("软触发"));
        ui->comboBox_trgsrc->addItem(tr("硬触发"));
    }
    case trgmode:
        if (m_mapDataIndex.contains(data)) {
            int index = m_mapDataIndex[data];
            ui->comboBox_trgmode->setCurrentIndex(index);
        }
        break;
    case trgsrc:
        ui->comboBox_trgsrc->setCurrentIndex(data);
        break;
    case trgexp_delay:
        ui->spinBox_trgexp_delay->setValue(data);
        break;
    case trgnum:
        ui->spinBox_trgnum->setValue(data);
        break;
    case trginterval:
        ui->spinBox_trginterval->setValue(data);
        break;
    case trgfilter_time:
        ui->spinBox_trgfilter_time->setValue(data);
        break;
    case trgdelay:
        ui->spinBox_trgdelay->setValue(data);
        break;
    default:
        break;
    }
    updateStatus();
}

void TriggerDlg::rcvRet(EuCMD cmd, uint32_t data)
{
    switch (cmd) {
    case trgmode:
        ui->comboBox_trgmode->setCurrentIndex(data);
        break;
    case trgsrc:
        ui->comboBox_trgsrc->setCurrentIndex(data);
        break;
    case trgexp_delay:
        ui->spinBox_trgexp_delay->setValue(data);
        break;
    case trgnum:
        ui->spinBox_trgnum->setValue(data);
        break;
    case trginterval:
        ui->spinBox_trginterval->setValue(data);
        break;
    case trgfilter_time:
        ui->spinBox_trgfilter_time->setValue(data);
        break;
    case trgdelay:
        ui->spinBox_trgdelay->setValue(data);
        break;
    default:
        break;
    }
}

bool TriggerDlg::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QSpinBox *spinBox = qobject_cast<QSpinBox *>(obj);
        if (spinBox && spinBox->isEnabled()) {
            // QStyleOptionSpinBox opt;
            // opt.initFrom(spinBox);
            // QRect upRect = spinBox->style()->subControlRect(QStyle::CC_SpinBox,
            //                                                 &opt,
            //                                                 QStyle::SC_SpinBoxUp,
            //                                                 spinBox);
            // QRect downRect = spinBox->style()->subControlRect(QStyle::CC_SpinBox,
            //                                                   &opt,
            //                                                   QStyle::SC_SpinBoxDown,
            //                                                   spinBox);

            // if (upRect.contains(mouseEvent->pos())) {
            // } else if (downRect.contains(mouseEvent->pos())) {
            // }
            int value = spinBox->value();
            EuCMD cmd;
            bool bFlag = true;
            if (spinBox->objectName() == ui->spinBox_trgdelay->objectName()) {
                cmd = trgdelay;
            } else if (spinBox->objectName() == ui->spinBox_trgexp_delay->objectName()) {
                cmd = trgexp_delay;
            } else if (spinBox->objectName() == ui->spinBox_trgfilter_time->objectName()) {
                cmd = trgfilter_time;
            } else if (spinBox->objectName() == ui->spinBox_trginterval->objectName()) {
                cmd = trginterval;
            } else if (spinBox->objectName() == ui->spinBox_trgnum->objectName()) {
                cmd = trgnum;
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
            if (slider->objectName() == ui->horizontalSlider_trgdelay->objectName()) {
                cmd = trgdelay;
            } else if (slider->objectName() == ui->horizontalSlider_trgexp_delay->objectName()) {
                cmd = trgexp_delay;
            } else if (slider->objectName() == ui->horizontalSlider_trgfilter_time->objectName()) {
                cmd = trgfilter_time;
            } else if (slider->objectName() == ui->horizontalSlider_trginterval->objectName()) {
                cmd = trginterval;
            } else if (slider->objectName() == ui->horizontalSlider_trgnum->objectName()) {
                cmd = trgnum;
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

void TriggerDlg::initGUI()
{
    setWindowTitle("TriggerMode");
    setFocusPolicy(Qt::NoFocus);

    ui->spinBox_trgdelay->installEventFilter(this);
    ui->spinBox_trgexp_delay->installEventFilter(this);
    ui->spinBox_trgfilter_time->installEventFilter(this);
    ui->spinBox_trginterval->installEventFilter(this);
    ui->spinBox_trgnum->installEventFilter(this);

    ui->horizontalSlider_trgdelay->installEventFilter(this);
    ui->horizontalSlider_trgexp_delay->installEventFilter(this);
    ui->horizontalSlider_trgfilter_time->installEventFilter(this);
    ui->horizontalSlider_trginterval->installEventFilter(this);
    ui->horizontalSlider_trgnum->installEventFilter(this);

    ui->label_trgfilter_time->hide();
    ui->horizontalSlider_trgfilter_time->hide();
    ui->spinBox_trgfilter_time->hide();

    int index = 0;

    ui->comboBox_trgmode->addItem(tr("流模式"), Stream);
    m_mapDataIndex[Stream] = index++;
    ui->comboBox_trgmode->addItem(tr("普通触发"), Normal);
    m_mapDataIndex[Normal] = index++;
    ui->comboBox_trgmode->addItem(tr("电平触发"), Pulse);
    m_mapDataIndex[Pulse] = index++;
    ui->comboBox_trgmode->addItem(tr("滚动快门多帧触发"), Rolling);
    m_mapDataIndex[Rolling] = index++;

    ui->comboBox_trgsrc->addItem(tr("软触发"));
    ui->comboBox_trgsrc->addItem(tr("硬触发"));

    ui->horizontalSlider_trgexp_delay->setRange(0, 1000000);
    ui->spinBox_trgexp_delay->setRange(0, 1000000);

    ui->horizontalSlider_trgnum->setRange(1, 255);
    ui->spinBox_trgnum->setRange(1, 255);

    ui->horizontalSlider_trginterval->setRange(0, 0xFFFFFF);
    ui->spinBox_trginterval->setRange(0, 0xFFFFFF);

    ui->horizontalSlider_trgfilter_time->setRange(0, 1000000);
    ui->spinBox_trgfilter_time->setRange(0, 1000000);

    ui->horizontalSlider_trgdelay->setRange(0, 1000000);
    ui->spinBox_trgdelay->setRange(0, 1000000);

    //resize(580, 240);
}

void TriggerDlg::initCNT()
{
    connect(ui->comboBox_trgmode, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        int cmd = ui->comboBox_trgmode->currentData().toInt();
        emit sndCmd(trgmode, cmd);
        updateStatus();
        //qDebug() << ".....trgmode snd " << cmd << index;
    });
    connect(ui->comboBox_trgsrc, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        if (Pulse == ui->comboBox_trgmode->currentData().toInt() && 0 == index) {
            QxToast::showTip(tr("不支持该模式"), nativeParentWidget());
            ui->comboBox_trgsrc->setCurrentIndex(1);
        } else {
            emit sndCmd(trgsrc, index);
        }
        updateStatus();
    });

    connect(ui->pushButton_trgone, &QPushButton::clicked, this, [this]() {
        emit sndCmd(trgone, 1);
    });

    connect(ui->horizontalSlider_trgexp_delay,
            &QSlider::valueChanged,
            ui->spinBox_trgexp_delay,
            &QSpinBox::setValue);
    connect(ui->spinBox_trgexp_delay,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSlider_trgexp_delay,
            &QSlider::setValue);
    // connect(ui->horizontalSlider_trgexp_delay, &QSlider::sliderReleased, this, [this]() {
    //     int value = ui->horizontalSlider_trgexp_delay->value();
    //     emit sndCmd(trgexp_delay, value);
    // });
    connect(ui->spinBox_trgexp_delay, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBox_trgexp_delay->value();
        emit sndCmd(trgexp_delay, value);
    });

    connect(ui->horizontalSlider_trgnum,
            &QSlider::valueChanged,
            ui->spinBox_trgnum,
            &QSpinBox::setValue);
    connect(ui->spinBox_trgnum,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSlider_trgnum,
            &QSlider::setValue);
    // connect(ui->horizontalSlider_trgnum, &QSlider::sliderReleased, this, [this]() {
    //     int value = ui->horizontalSlider_trgnum->value();
    //     emit sndCmd(trgnum, value);
    // });
    connect(ui->spinBox_trgnum, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBox_trgnum->value();
        emit sndCmd(trgnum, value);
    });

    connect(ui->horizontalSlider_trginterval,
            &QSlider::valueChanged,
            ui->spinBox_trginterval,
            &QSpinBox::setValue);
    connect(ui->spinBox_trginterval,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSlider_trginterval,
            &QSlider::setValue);
    // connect(ui->horizontalSlider_trginterval, &QSlider::sliderReleased, this, [this]() {
    //     int value = ui->horizontalSlider_trginterval->value();
    //     emit sndCmd(trginterval, value);
    // });
    connect(ui->spinBox_trginterval, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBox_trginterval->value();
        emit sndCmd(trginterval, value);
    });

    connect(ui->horizontalSlider_trgfilter_time,
            &QSlider::valueChanged,
            ui->spinBox_trgfilter_time,
            &QSpinBox::setValue);
    connect(ui->spinBox_trgfilter_time,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSlider_trgfilter_time,
            &QSlider::setValue);
    // connect(ui->horizontalSlider_trgfilter_time, &QSlider::sliderReleased, this, [this]() {
    //     int value = ui->horizontalSlider_trgfilter_time->value();
    //     emit sndCmd(trgfilter_time, value);
    // });
    connect(ui->spinBox_trgfilter_time, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBox_trgfilter_time->value();
        emit sndCmd(trgfilter_time, value);
    });

    connect(ui->horizontalSlider_trgdelay,
            &QSlider::valueChanged,
            ui->spinBox_trgdelay,
            &QSpinBox::setValue);
    connect(ui->spinBox_trgdelay,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSlider_trgdelay,
            &QSlider::setValue);
    // connect(ui->horizontalSlider_trgdelay, &QSlider::sliderReleased, this, [this]() {
    //     int value = ui->horizontalSlider_trgdelay->value();
    //     emit sndCmd(trgdelay, value);
    // });
    connect(ui->spinBox_trgdelay, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBox_trgdelay->value();
        emit sndCmd(trgdelay, value);
    });
}

void TriggerDlg::updateStatus()
{
    int curModel = ui->comboBox_trgmode->currentData().toInt();
    int curSrc = ui->comboBox_trgsrc->currentIndex();
    ui->comboBox_trgsrc->setEnabled(true);
    ui->pushButton_trgone->setEnabled(true);

    switch (curModel) {
    case Stream: {
        ui->comboBox_trgsrc->setEnabled(false);
        ui->pushButton_trgone->setEnabled(false);
    }
    case Normal: {
        if (0 != curSrc) {
            ui->pushButton_trgone->setEnabled(false);
        }
    } break;
    case Rolling: {
        if (0 != curSrc) {
            ui->pushButton_trgone->setEnabled(false);
        }
    } break;
    case Pulse: {
        ui->comboBox_trgsrc->setCurrentIndex(1);
        ui->comboBox_trgsrc->setEnabled(false);
        ui->pushButton_trgone->setEnabled(false);
    } break;
    default:
        break;
    }
}
