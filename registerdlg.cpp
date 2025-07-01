#include "registerdlg.h"
#include <QStyleFactory>
#include "json/json.h"
#include "qxhelp.h"
#include "ui_registerdlg.h"
#include <qabstractitemview.h>

RegisterDlg::RegisterDlg(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegisterDlg)
{
    ui->setupUi(this);
    //initCfg();
    initGUI();
    initCNT();
}

RegisterDlg::~RegisterDlg()
{
    delete ui;
}

void RegisterDlg::setCfg(QMap<QString, StRegInfo> nameReg, QMap<uint32_t, StRegInfo> addrReg)
{
    m_mapNameRegister = nameReg;
    m_mapAddrRegister = addrReg;
    updateReg();
}

void RegisterDlg::rcvUpdateUi()
{
    ui->retranslateUi(this);
    updateReg();
}

void RegisterDlg::rcvCmdRet(const StCmdRet &ret)
{
    QString strRet = (QString("0x%1").arg(ret.data, 8, 16, QLatin1Char('0')));
    ui->lineEditValue->setText(strRet);
}

void RegisterDlg::initGUI()
{
    ui->comboBoxReg->setMaxVisibleItems(20);
    ui->comboBoxReg->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->textEdit_description->setEnabled(false);
    ui->textEdit_description->autoFormatting();
}

void RegisterDlg::initCNT()
{
    connect(ui->comboBoxReg, &QComboBox::currentTextChanged, this, [this](const QString &value) {
        if (m_mapNameRegister.contains(value)) {
            StRegInfo info = m_mapNameRegister[value];
            if (info.rd_wt == "RO") {
                ui->pushButtonWrite->setEnabled(false);
            } else {
                ui->pushButtonWrite->setEnabled(true);
            }
            ui->lineEditRegAddr->setText(info.address);
            ui->lineEditRegAddr->setEnabled(false);
            ui->textEdit_description->setPlainText(info.description);
        } else {
            ui->lineEditRegAddr->setText("");
            ui->lineEditRegAddr->setEnabled(true);
        }
        ui->lineEditValue->setText("");
    });

    connect(ui->pushButtonRead, &QPushButton::clicked, this, [this]() {
        if (0 == ui->comboBoxReg->currentIndex()) {
            QString strReg;
            strReg = ui->lineEditRegAddr->text();
            QString strValue = ui->lineEditValue->text();
            StRegInfo info;
            info.address = strReg;
            info.data = strValue;
            emit sndReg(info, false);
        } else {
            QString strName = ui->comboBoxReg->currentText();
            QString strAddr = ui->comboBoxReg->currentData().toString();
            if (m_mapNameRegister.contains(strName)) {
                StRegInfo info = m_mapNameRegister[strName];
                emit sndReg(info, false);
            }
        }
    });
    connect(ui->pushButtonWrite, &QPushButton::clicked, this, [this]() {
        QString strValue = ui->lineEditValue->text();
        if (0 == ui->comboBoxReg->currentIndex()) {
            StRegInfo info;
            info.address = ui->lineEditRegAddr->text();
            info.data = strValue;
            info.rd_wt = "RW";
            emit sndReg(info, true);
        } else {
            QString strName = ui->comboBoxReg->currentText();
            QString strAddr = ui->comboBoxReg->currentData().toString();
            if (m_mapNameRegister.contains(strName)) {
                StRegInfo info = m_mapNameRegister[strName];
                info.data = strValue;
                emit sndReg(info, true);
            }
        }
    });

    connect(ui->pushButtonImport, &QPushButton::clicked, this, [this]() { emit sndImport(); });
    connect(ui->pushButtonExport, &QPushButton::clicked, this, [this]() { emit sndExport(); });

    connect(ui->pushButton_factoryparam, &QPushButton::clicked, this, [this]() {
        if (QxHelp::showMsgBoxQuesion(tr("提示"),
                                      tr("恢复默认值将停止预览并重启相机，是否继续？"))) {
            emit sndCmd(factoryparam, 1);
        }
    });

    connect(ui->pushButton_paramsave, &QPushButton::clicked, this, [this]() {
        emit sndCmd(paramsave, 1);
    });
}

// bool RegisterDlg::loadRegCfg(QString strJson)
// {
//     bool bRet = false;
//     {
//         Json::Value jsonRoot;
//         if (QxHelp::readJson(strJson, jsonRoot)) {
//             m_mapNameRegister.clear();
//             m_mapAddrRegister.clear();
//             int total = jsonRoot.size();
//             for (int i = 0; i < total; ++i) {
//                 StRegInfo reg;
//                 Json::Value jsonNode = jsonRoot[i];
//                 reg.address = jsonNode["address"].asString().c_str();
//                 reg.rd_wt = jsonNode["rd_wt"].asString().c_str();
//                 reg.rd_wt = reg.rd_wt.toUpper();
//                 reg.name = jsonNode["name"].asString().c_str();
//                 reg.raw_support = jsonNode["raw_support"].asString().c_str();
//                 reg.raw_support = reg.raw_support.toUpper();
//                 reg.data_type = jsonNode["data_type"].asString().c_str();
//                 reg.data_type = reg.data_type.toLower();
//                 reg.update_timing = jsonNode["update_timing"].asString().c_str();
//                 reg.update_timing = reg.update_timing.toUpper();
//                 reg.description = jsonNode["description"].asString().c_str();
//                 m_mapNameRegister.insert(reg.name, reg);
//                 m_mapAddrRegister.insert(reg.address, reg);
//                 bRet = true;
//             }
//             updateReg();
//         } else {
//             qDebug() << "loadRegCfg failed!!!";
//         }
//     }
//     return bRet;
// }

// bool RegisterDlg::saveRegCfg(QString strJson)
// {
//     Json::Value jsonRoot;
//     for (const auto &it : qAsConst(m_mapNameRegister)) {
//         Json::Value jsonNode;

//         jsonNode["address"] = it.address.toStdString();
//         jsonNode["rd_wt"] = it.rd_wt.toStdString();
//         jsonNode["name"] = it.name.toStdString();
//         jsonNode["raw_support"] = it.raw_support.toStdString();
//         jsonNode["data_type"] = it.data_type.toStdString();
//         jsonNode["update_timing"] = it.update_timing.toStdString();
//         jsonNode["description"] = it.description.toStdString();
//         jsonRoot.append(jsonNode);
//     }
//     return QxHelp::writeJson(strJson, jsonRoot);
// }

void RegisterDlg::updateReg()
{
    int index = 0;
    ui->comboBoxReg->clear();
    for (auto &it : qAsConst(m_mapAddrRegister)) {
        if (0 == index) {
            ui->comboBoxReg->addItem(tr("自定义"));
        } else {
            ui->comboBoxReg->addItem(it.name, it.address);
        }
        index++;
    }
    ui->comboBoxReg->setCurrentIndex(0);
}
