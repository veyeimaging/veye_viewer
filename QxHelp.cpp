#include "QxHelp.h"
#include <QDoubleValidator>
#include <QProcess>
#include <QWindow>
#include <fstream>
#include <unistd.h>

QxHelp::QxHelp()
{    
}

void QxHelp::setWindowIdentity(QWidget *widget, const QString &appName)
{
    if (!widget)
        return;

    // 1. 基础设置（必选）
    widget->setWindowTitle(QObject::tr(appName.toStdString().c_str()));
    widget->setObjectName(appName);
    QApplication::setApplicationName(appName);
    QApplication::setApplicationDisplayName(QObject::tr(appName.toStdString().c_str()));

// 2. Wayland 适配（RK3588主流）
#ifdef Q_OS_LINUX
    QWindow *window = widget->windowHandle();
    if (window) {
        // Wayland 核心：设置 app-id（彻底替代默认widget）
        window->setProperty("app-id", appName);
        // X11 核心：设置 WM_CLASS（双值均为appName）
        window->setProperty("WM_CLASS", QVariantList{appName, appName});
        // 强制刷新窗口属性
        window->requestUpdate();
    }
#endif
}

void QxHelp::SetStyle(const QString &styleName)
{
    QString filename = QString(":/image/%1.qss").arg(styleName);
    QFile file(filename);
    file.open(QFile::ReadOnly);
    if (file.isOpen()) {
        QString qss = QLatin1String(file.readAll());
        qApp->setStyleSheet(qss);
        file.close();
        qApp->setPalette(QPalette(QColor("#F0F0F0")));
    } else {
        qDebug() << "open qss file error";
    }
}

QByteArray QxHelp::ushortToByte(ushort i)
{
    QByteArray result;
    result.resize(2);
    result[1] = (uchar) (0x000000ff & i);
    result[0] = (uchar) ((0x0000ff00 & i) >> 8);
    return result;
}

void QxHelp::moveCenter(QWidget *win)
{
    QSize screenSize = QGuiApplication::primaryScreen()->availableSize();
    //double devicePixelRatio = QGuiApplication::primaryScreen()->devicePixelRatio();
    int x = (screenSize.width() - win->width()) / 2;
    int y = (screenSize.height() - win->height()) / 2;
    win->move(x, y);
}

void QxHelp::moveParentCenter(QWidget *parent, QWidget *widget)
{
    int x = (parent->width() - widget->width()) / 2;
    int y = (parent->height() - widget->height()) / 2;
    widget->move(x, y);
}

void QxHelp::makeFolderTail(QString &strDir)
{
    if (!strDir.isEmpty()) {
        if ('/' != strDir.at(strDir.length() - 1)) {
            strDir += "/";
        }
    }
}

QString QxHelp::makeFolder(QString createDir)
{
    if (createDir.isEmpty()) {
        return createDir;
    }
    QDir dir(createDir);
    if (dir.exists(createDir)) {
        return createDir;
    }

    QString parentDir = makeFolder(createDir.mid(0, createDir.lastIndexOf('/')));
    QString dirName = createDir.mid(createDir.lastIndexOf('/') + 1);
    QDir parentPath(parentDir);
    if (!dirName.isEmpty()) {
        parentPath.mkpath(dirName);
    }
    return parentDir + "/" + dirName;
}

bool QxHelp::removeFolder(const QString &path)
{
    if (path.isEmpty()) {
        return false;
    }
    QDir dir(path);
    if (!dir.exists()) {
        return true;
    }
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    QFileInfoList fileList = dir.entryInfoList(); // 获取所有的文件信息
    foreach (QFileInfo file, fileList)            //遍历文件信息
    {
        if (file.isFile()) {
            file.dir().remove(file.fileName());
        } else {
            removeFolder(file.absoluteFilePath());
        }
    }
    return dir.rmdir(dir.absolutePath()); // 删除文件夹
}

QString QxHelp::getBinPath()
{
    return QApplication::applicationDirPath();
}

string QxHelp::json2String(Json::Value json)
{
    Json::FastWriter fast_writer;
    string strJson = fast_writer.write(json);
    int nJsonSize = strJson.length() - 1;
    strJson[nJsonSize] = '\0';
    return strJson;
}

QString QxHelp::json2QString(Json::Value json)
{
    return QString::fromStdString(json2String(json));
}

bool QxHelp::readJson(QString strJsonFile, Json::Value &json)
{
    bool bRet = false;

    ifstream in((char *) strJsonFile.toStdString().c_str(), ios::binary);
    if (in.is_open()) {
        Json::Reader jsonReader;
        if (jsonReader.parse(in, json)) {
            bRet = true;
        } else {
            qWarning() << "jsonReader.parse " << strJsonFile << " failed";
        }
        in.close();
    }
    return bRet;
}

bool QxHelp::writeJson(QString strJsonFile, Json::Value &json)
{
    bool bRet = false;
    ofstream fout((char *) strJsonFile.toStdString().c_str(), ios::binary);
    if (fout.is_open()) {
        fout << json.toStyledString() << endl;
        fout.close();
        bRet = true;
    } else {
        qWarning() << "writeJson " << strJsonFile << " failed";
    }
    return bRet;
}

QString QxHelp::getCurTime()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}

int QxHelp::showMsgBoxQuesion(QString title, QString msg)
{
    MsgBox *dlg = new MsgBox();
    dlg->SetMessage(title, msg, 1);
    return dlg->exec();
}

int QxHelp::showMsgBoxInfo(QString title, QString msg)
{
    MsgBox *dlg = new MsgBox();
    dlg->SetMessage(title, msg, 0);
    return dlg->exec();
}

bool QxHelp::writeFile(char *buf, int nLen, QString file)
{
    bool bRet = false;
    std::ofstream ofs(file.toStdString(), ios::out | ios::binary);
    if (ofs.is_open()) {
        ofs.write(buf, nLen);
        ofs.close();
        bRet = true;
    }
    return bRet;
}

bool QxHelp::readFirstLine(QString file, QString &strValue)
{
    bool bRet = false;
    std::ifstream ifs(file.toStdString());
    if (ifs.is_open()) {
        std::string strLine;
        getline(ifs, strLine);
        strValue = QString::fromStdString(strLine);
        ifs.close();
        bRet = true;
    } else {
        qCritical() << "open " << file << " failed";
    }
    return bRet;
}

bool QxHelp::hasWritePermission(const char *path)
{
    if (access(path, W_OK) == 0) {
        return true;
    } else {
        return false;
    }
}

void QxHelp::detectPlatform()
{
    qDebug() << "Build CPU Architecture:" << QSysInfo::buildCpuArchitecture();
    qDebug() << "Current CPU Architecture:" << QSysInfo::currentCpuArchitecture();
    qDebug() << "Kernel Type:" << QSysInfo::kernelType();
    qDebug() << "Kernel Version:" << QSysInfo::kernelVersion();
    qDebug() << "Product Name:" << QSysInfo::productType();
    qDebug() << "Product Version:" << QSysInfo::productVersion();
}

QString QxHelp::getModel()
{
    QFile file("/proc/device-tree/model");
    if (!file.exists())
        return "Unknown";

    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        QString model = stream.readAll().trimmed();
        file.close();
        return model;
    }
    return "Unknown";
}

bool QxHelp::fileExists(const string &filename)
{
    return access(filename.c_str(), F_OK) == 0;
}

bool QxHelp::isDirectoryExists(const QString &path)
{
    QFileInfo fileInfo(path);
    // exists() 判断路径是否存在，isDir() 判断是否为目录
    return fileInfo.exists() && fileInfo.isDir();
}

int QxHelp::runCmd(const QString &cmd, QString &output, int timeout_ms)
{
    QProcess process;
    process.start(cmd);                         // 启动进程
    if (!process.waitForFinished(timeout_ms)) { // 等待完成，超时返回 false
        process.kill();
        return -2; // 超时
    }

    output = process.readAllStandardOutput();     // 读取 stdout
    QString err = process.readAllStandardError(); // 读取 stderr（可选合并）
    if (!err.isEmpty())
        output += "\n错误：" + err;

    return process.exitCode(); // 命令退出码
}

/**
 * @brief 初始化浮点数输入框（QLineEdit）
 * @param lineEdit 目标输入框指针（不可为nullptr）
 * @param minVal 最小值（默认：0.00）
 * @param maxVal 最大值（默认：999.99）
 * @param decimalDigits 保留小数位数（默认：2）
 * @param defaultVal 默认值（默认：100.00）
 * @param placeholder 输入提示文本（默认：自动生成，如"请输入0.00~999.99之间的数字（保留2位小数）"）
 */
void QxHelp::setDoubleLineEdit(QLineEdit *lineEdit,
                               double minVal,
                               double maxVal,
                               int decimalDigits,
                               double defaultVal,
                               const QString &placeholder)
{
    if (!lineEdit)
        return; // 防止空指针崩溃

    // 1. 配置浮点数验证器（核心）
    QDoubleValidator *validator = new QDoubleValidator(minVal, maxVal, decimalDigits, lineEdit);
    validator->setNotation(QDoubleValidator::StandardNotation); // 避免科学计数法（如1e+02）
    validator->setLocale(QLocale::C); // 固定小数点为"."，不受系统区域设置影响（兼容全球环境）
    lineEdit->setValidator(validator);

    // 2. 设置输入提示（默认自动生成，支持自定义）
    // QString tip = placeholder;
    // if (tip.isEmpty()) {
    //     tip = QString("请输入%1~%2之间的数字（保留%3位小数）")
    //               .arg(minVal, 0, 'f', decimalDigits) // 格式化最小值（保留指定小数位）
    //               .arg(maxVal, 0, 'f', decimalDigits) // 格式化最大值
    //               .arg(decimalDigits);
    // }
    // lineEdit->setPlaceholderText(tip);

    // 3. 自动计算最大输入长度（避免输入过长无效值）
    int integerLength = QString::number(static_cast<long long>(maxVal)).length(); // 整数部分位数
    int totalLength = integerLength + decimalDigits
                      + (decimalDigits > 0 ? 1 : 0); // 总长度=整数位+小数位+小数点（可选）
    lineEdit->setMaxLength(totalLength);

    // 4. 设置默认值（自动格式化小数位数）
    // QString defaultText = QString::number(defaultVal, 'f', decimalDigits);
    // lineEdit->setText(defaultText);
}

void QxHelp::setLabelElidedText(QLabel *lbl,
                                Qt::TextElideMode elideMode,
                                int displayWidth,
                                int padding)
{
    if (lbl) {
        QString str = lbl->text();
        QFontMetrics fontWidth(lbl->font());
        QString elideNote = fontWidth.elidedText(str, elideMode, displayWidth - padding);
        lbl->setText(elideNote);
        lbl->setToolTip(str);
    }
}

void QxHelp::batchSetLabelElidedText(QWidget *parent,
                                     const QRegularExpression &labelRegex,
                                     Qt::TextElideMode elideMode,
                                     int displayWidth,
                                     int padding)
{
    if (!parent) {
        return;
    }

    // 查找父控件下所有匹配的Label
    const QList<QLabel *> targetLabels = parent->findChildren<QLabel *>(labelRegex);
    for (QLabel *lbl : targetLabels) {
        setLabelElidedText(lbl, elideMode, displayWidth, padding);
    }
}
