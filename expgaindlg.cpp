#include "expgaindlg.h"
#include <QDebug>
#include <QSlider>
#include "codemap.h"
#include "qxhelp.h"
#include "qxtoast.h"
#include "ui_expgaindlg.h"

ExpGainDlg::ExpGainDlg(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ExpGainDlg)
{
    ui->setupUi(this);
    initGUI();
}

ExpGainDlg::~ExpGainDlg()
{
    delete ui;
}

void ExpGainDlg::rcvUpdateUi()
{
    ui->retranslateUi(this);
}

void ExpGainDlg::rcvCmdRet(const StCmdRet &ret)
{
    int exp_min = 16;
    int exp_max = 1000000;
    uint32_t data = ret.data;
    switch (ret.cmd) {
    case model: {
        QStringList ltExp;
        QStringList ltGain;
        QString modelName = codemap::getMapModel(data);
        ltExp << tr("手动");
        ltGain << tr("手动");
        ui->widget->setEnabled(true);
        ui->pushButtonOneAEAG->setEnabled(true);
        if ("MV" == modelName.mid(0, 2)) {
            ltExp << tr("自动");
            ltGain << tr("自动");
            ui->widget->setEnabled(true);
            ui->pushButtonOneAEAG->setEnabled(true);
        } else if (modelName.contains("RAW") && (modelName != "RAW-MIPI-AR0234M")) {
            ui->widget->setEnabled(false);
            ui->pushButtonOneAEAG->setEnabled(false);
        }
        if (modelName == "RAW-MIPI-AR0234M") {
            ltExp << tr("自动");
            ltGain << tr("自动");
            ui->pushButtonOneAEAG->setEnabled(false);
        }

        ui->comboBox_exp->clear();
        ui->comboBox_gain->clear();
        ui->comboBox_exp->addItems(ltExp);
        ui->comboBox_gain->addItems(ltGain);
    } break;
    case expmode: {
        switch (data) {
        case 0:
            ui->comboBox_exp->setCurrentIndex(0);
            break;
        case 1:
            break;
        case 2:
            ui->comboBox_exp->setCurrentIndex(1);
            break;
        default:
            break;
        }
    } break;
    case gainmode: {
        switch (data) {
        case 0:
            ui->comboBox_gain->setCurrentIndex(0);
            break;
        case 1:
            break;
        case 2:
            ui->comboBox_gain->setCurrentIndex(1);
            break;
        default:
            break;
        }
    } break;
    case exptime:
        ui->label_exptime->setText(QString::number(data));
        break;
    case curgain:
        ui->label_curgain->setText(QString::number(data / 10.0, 'f', 1));
        break;
    case aatarget:
        ui->spinBox_aatarget->setValue(data);
        break;
    case aemaxtime:
        ui->spinBox_aemaxtime->setValue(data);
        break;
    case agmaxgain:
        ui->spinBox_agmaxgain->setValue(data / 10.0);
        break;
    case metime:
        ui->spinBox_metime->setValue(data);
        break;
    case mgain:
        ui->spinBox_mgain->setValue(data / 10.0);
        break;
    case exptime_min: {
        if (data != 0xFFFFFFFF && data != 0) {
            exp_min = data;
        }
        ui->horizontalSlider_aemaxtime->setMinimum(exp_min);
        ui->spinBox_aemaxtime->setMinimum(exp_min);
        ui->horizontalSlider_metime->setMinimum(exp_min);
        ui->spinBox_metime->setMinimum(exp_min);
    } break;
    case exptime_max: {
        if (data != 0xFFFFFFFF && data != 0) {
            exp_max = data;
        }
        ui->horizontalSlider_aemaxtime->setMaximum(exp_max);
        ui->spinBox_aemaxtime->setMaximum(exp_max);
        ui->horizontalSlider_metime->setMaximum(exp_max);
        ui->spinBox_metime->setMaximum(exp_max);
    } break;
    default:
        break;
    }
}


void ExpGainDlg::rcvValue(int exp, int gain)
{
    ui->label_exptime->setText(QString::number(exp));
    ui->label_curgain->setText(QString::number(gain / 10.0));
}

void ExpGainDlg::rcvModel(int exp, int gain)
{
    if (0 == exp) {
        ui->comboBox_exp->setCurrentIndex(0);
    } else {
        ui->comboBox_exp->setCurrentIndex(1);
    }
    if (0 == gain) {
        ui->comboBox_gain->setCurrentIndex(0);
    } else {
        ui->comboBox_gain->setCurrentIndex(1);
    }
}

bool ExpGainDlg::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease /*|| event->type() == QEvent::Enter*/) {
        //QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
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
            if (spinBox->objectName() == ui->spinBox_aatarget->objectName()) {
                cmd = aatarget;
            } else if (spinBox->objectName() == ui->spinBox_aemaxtime->objectName()) {
                cmd = aemaxtime;
            } else if (spinBox->objectName() == ui->spinBox_metime->objectName()) {
                cmd = metime;
            } else {
                bFlag = false;
            }
            if (bFlag) {
                //qDebug() << "spinBox " << spinBox->objectName() << " send";
                emit sndCmd(cmd, value);
            }
        }
        QDoubleSpinBox *dSpinBox = qobject_cast<QDoubleSpinBox *>(obj);
        if (dSpinBox && dSpinBox->isEnabled()) {
            double value = dSpinBox->value();
            EuCMD cmd;
            bool bFlag = true;
            if (dSpinBox->objectName() == ui->spinBox_agmaxgain->objectName()) {
                cmd = agmaxgain;
            } else if (dSpinBox->objectName() == ui->spinBox_mgain->objectName()) {
                cmd = mgain;
            } else {
                bFlag = false;
            }
            if (bFlag) {
                //qDebug() << "spinBox " << dSpinBox->objectName() << " send " << value;
                emit sndCmd(cmd, value * 10);
            }
        }
        QSlider *slider = qobject_cast<QSlider *>(obj);
        if (slider && slider->isEnabled()) {
            int value = slider->value();
            EuCMD cmd;
            bool bFlag = true;
            if (slider->objectName() == ui->horizontalSlider_aatarget->objectName()) {
                cmd = aatarget;
            } else if (slider->objectName() == ui->horizontalSlider_aemaxtime->objectName()) {
                cmd = aemaxtime;
            } else if (slider->objectName() == ui->horizontalSlider_agmaxgain->objectName()) {
                cmd = agmaxgain;
            } else if (slider->objectName() == ui->horizontalSlider_metime->objectName()) {
                cmd = metime;
            } else if (slider->objectName() == ui->horizontalSlider_mgain->objectName()) {
                cmd = mgain;
            } else {
                bFlag = false;
            }
            if (bFlag) {
                //qDebug() << "QSlider " << slider->objectName() << " send " << value;
                emit sndCmd(cmd, value);
            }
        }
    }
    return QObject::eventFilter(obj, event);
}

void ExpGainDlg::initGUI()
{
    setFocusPolicy(Qt::NoFocus);

    ui->pushButtonOneAEAG->setFocusPolicy(Qt::NoFocus);

    ui->spinBox_aatarget->installEventFilter(this);
    ui->spinBox_aemaxtime->installEventFilter(this);
    ui->spinBox_agmaxgain->installEventFilter(this);
    ui->spinBox_metime->installEventFilter(this);
    ui->spinBox_mgain->installEventFilter(this);

    ui->horizontalSlider_aatarget->installEventFilter(this);
    ui->horizontalSlider_aemaxtime->installEventFilter(this);
    ui->horizontalSlider_agmaxgain->installEventFilter(this);
    ui->horizontalSlider_metime->installEventFilter(this);
    ui->horizontalSlider_mgain->installEventFilter(this);

    connect(ui->comboBox_exp, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        if (0 == index) {
            emit sndCmd(expmode, 0);
        } else {
            emit sndCmd(expmode, 2);
        }
    });
    connect(ui->comboBox_gain, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        if (0 == index) {
            emit sndCmd(gainmode, 0);
        } else {
            emit sndCmd(gainmode, 2);
        }
    });

    connect(ui->horizontalSlider_aatarget,
            &QSlider::valueChanged,
            ui->spinBox_aatarget,
            &QSpinBox::setValue);
    connect(ui->spinBox_aatarget,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSlider_aatarget,
            &QSlider::setValue);
    // connect(ui->horizontalSlider_aatarget, &QSlider::sliderReleased, this, [this]() {
    //     int value = ui->horizontalSlider_aatarget->value();
    //     qDebug() << "horizontalSlider_aatarget snd " << value;
    //     emit sndCmd(aatarget, value);
    // });
    connect(ui->spinBox_aatarget, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBox_aatarget->value();
        emit sndCmd(aatarget, value);
    });

    connect(ui->horizontalSlider_aemaxtime,
            &QSlider::valueChanged,
            ui->spinBox_aemaxtime,
            &QSpinBox::setValue);
    connect(ui->spinBox_aemaxtime,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSlider_aemaxtime,
            &QSlider::setValue);
    // connect(ui->horizontalSlider_aemaxtime, &QSlider::sliderReleased, this, [this]() {
    //     int value = ui->horizontalSlider_aemaxtime->value();
    //     qDebug() << "................" << ui->spinBox_aemaxtime->value();
    //     emit sndCmd(aemaxtime, value);
    // });
    connect(ui->spinBox_aemaxtime, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBox_aemaxtime->value();
        emit sndCmd(aemaxtime, value);
    });

    connect(ui->horizontalSlider_agmaxgain, &QSlider::valueChanged, this, [this](int value) {
        ui->spinBox_agmaxgain->setValue(value / 10.0);
    });
    connect(ui->spinBox_agmaxgain,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            [this](double value) { ui->horizontalSlider_agmaxgain->setValue(value * 10); });

    // connect(ui->horizontalSlider_agmaxgain, &QSlider::sliderReleased, this, [this]() {
    //     int value = ui->horizontalSlider_agmaxgain->value();
    //     emit sndCmd(agmaxgain, value);
    // });
    connect(ui->spinBox_agmaxgain, &QDoubleSpinBox::editingFinished, this, [this]() {
        double value = ui->spinBox_agmaxgain->value();
        emit sndCmd(agmaxgain, value * 10);
    });

    connect(ui->horizontalSlider_metime,
            &QSlider::valueChanged,
            ui->spinBox_metime,
            &QSpinBox::setValue);
    connect(ui->spinBox_metime,
            QOverload<int>::of(&QSpinBox::valueChanged),
            ui->horizontalSlider_metime,
            &QSlider::setValue);
    // connect(ui->horizontalSlider_metime, &QSlider::sliderReleased, this, [this]() {
    //     int value = ui->horizontalSlider_metime->value();
    //     emit sndCmd(metime, value);
    // });
    connect(ui->spinBox_metime, &QSpinBox::editingFinished, this, [this]() {
        int value = ui->spinBox_metime->value();
        emit sndCmd(metime, value);
    });

    connect(ui->horizontalSlider_mgain, &QSlider::valueChanged, this, [this](int value) {
        ui->spinBox_mgain->setValue(value / 10.0);
    });
    connect(ui->spinBox_mgain,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            [this](double value) { ui->horizontalSlider_mgain->setValue(value * 10); });

    // connect(ui->horizontalSlider_mgain, &QSlider::sliderReleased, this, [this]() {
    //     int value = ui->horizontalSlider_mgain->value();
    //     qDebug() << value;
    //     emit sndCmd(mgain, value);
    // });
    connect(ui->spinBox_mgain, &QDoubleSpinBox::editingFinished, this, [this]() {
        double value = ui->spinBox_mgain->value();
        emit sndCmd(mgain, value * 10);
    });

    ui->horizontalSlider_aatarget->setRange(1, 255);
    ui->spinBox_aatarget->setRange(1, 255);

    ui->horizontalSlider_aemaxtime->setRange(16, 1000000);
    ui->spinBox_aemaxtime->setRange(16, 1000000);

    ui->horizontalSlider_agmaxgain->setRange(0, 480);
    ui->spinBox_agmaxgain->setRange(0.0, 48.0);
    ui->spinBox_agmaxgain->setDecimals(1);
    ui->spinBox_agmaxgain->setSingleStep(0.1);

    ui->horizontalSlider_metime->setRange(16, 1000000);
    ui->spinBox_metime->setRange(16, 1000000);

    ui->horizontalSlider_mgain->setRange(0, 480);
    ui->spinBox_mgain->setRange(0.0, 48.0);
    ui->spinBox_mgain->setDecimals(1);
    ui->spinBox_mgain->setSingleStep(0.1);

    connect(ui->pushButtonOneAEAG, &QPushButton::clicked, this, [this]() {
        int nAatarget = ui->spinBox_aatarget->value();
        int nAemaxtime = ui->spinBox_aemaxtime->value();
        int nAgmaxgain = ui->spinBox_agmaxgain->value() * 10;
        emit sndOneAEAG(nAemaxtime, nAgmaxgain, nAatarget);
    });
}
