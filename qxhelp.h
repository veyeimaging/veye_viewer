#ifndef QXHELP_H
#define QXHELP_H
#include "commheader.h"
#include "json/json.h"
#include "msgbox.h"
#include "qxtoast.h"

using namespace std;

class QxHelp
{
public:
    QxHelp();

public:
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
};

#endif // QXHELP_H
