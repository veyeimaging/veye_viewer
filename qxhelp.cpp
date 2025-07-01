#include "qxhelp.h"
#include <fstream>

QxHelp::QxHelp()
{    
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
            qWarning() << "readJson-parse " << strJsonFile << " failed";
        }
        in.close();
    } else {
        qWarning() << "readJson " << strJsonFile << " failed";
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
        qDebug() << "open error:" << file;
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
