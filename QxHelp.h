#ifndef QXHELP_H
#define QXHELP_H
#include <QLabel>
#include <QRegularExpression>
#include <QStringList>
#include "CommHeader.h"
#include "MsgBox.h"
#include "QxToast.h"
#include "json/json.h"

using namespace std;

class QxHelp
{
public:
    QxHelp();

public:
    static void setWindowIdentity(QWidget *widget, const QString &appName);

    static void SetStyle(const QString &styleName);

    static QByteArray ushortToByte(ushort i);

    static void moveCenter(QWidget *win);

    static void moveParentCenter(QWidget *parent, QWidget *widget);

    static void makeFolderTail(QString &strDir);

    static QString makeFolder(QString createDir);

    static bool removeFolder(const QString &path);

    static QString getBinPath();

    static string json2String(Json::Value json);

    static QString json2QString(Json::Value json);

    static bool readJson(QString strJsonFile, Json::Value &json);

    static bool writeJson(QString strJsonFile, Json::Value &json);

    static QString getCurTime();

    static int showMsgBoxQuesion(QString title, QString msg);

    static int showMsgBoxInfo(QString title, QString msg);

    static bool writeFile(char *buf, int nLen, QString file);

    static bool readFirstLine(QString file, QString &strLine);

    static bool hasWritePermission(const char *path);

    static void detectPlatform();

    static QString getModel();

    static bool fileExists(const std::string &filename);

    static bool isDirectoryExists(const QString &path);

    static int runCmd(const QString &cmd, QString &output, int timeout_ms);

    // 把一行命令拆成 program + arguments（支持单/双引号分组），
    // 供 QProcess 直接执行，不再经过 /bin/sh，避免 shell 注入与引号规则差异。
    static QStringList splitCmdLine(const QString &cmd);

    // 校验来自设备/配置文件的字符串是否可安全用作命令参数（白名单字符集）。
    static bool isSafeCmdToken(const QString &s, int maxLen = 256);

    static void setDoubleLineEdit(QLineEdit *lineEdit,
                                  double minVal = 0.00,
                                  double maxVal = 999.99,
                                  int decimalDigits = 2,
                                  double defaultVal = 100.00,
                                  const QString &placeholder = "");
    static void setLabelElidedText(QLabel *lbl,
                                   Qt::TextElideMode elideMode = Qt::ElideRight,
                                   int displayWidth = 120,
                                   int padding = 8);

    static void batchSetLabelElidedText(QWidget *parent,
                                        const QRegularExpression &labelRegex,
                                        Qt::TextElideMode elideMode = Qt::ElideRight,
                                        int displayWidth = 120,
                                        int padding = 8);
};

#endif // QXHELP_H
