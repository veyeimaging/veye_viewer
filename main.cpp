#include <QApplication>
#include <QLocale>
#include <QSurfaceFormat>
#include <QTranslator>
#include "mainwidget.h"
#include "qxhelp.h"

#define LOG_PATH QCoreApplication::applicationDirPath() + "/log/"

void logMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static mutex mutex;
    mutex.lock();

    QString text;
    switch (type) {
    case QtDebugMsg:
        text = QString("Debug");
        break;
    case QtInfoMsg:
        text = QString("Info");
        break;
    case QtWarningMsg:
        text = QString("Warning");
        break;
    case QtCriticalMsg:
        text = QString("Critical");
        break;
    case QtFatalMsg:
        text = QString("Fatal");
    }

    QString curDataTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss::zzz");
    QString curData = QString("%1").arg(curDataTime);
    QString strInfo = QString("%1-%2").arg(QString(context.file)).arg(context.line);
    QString strContext = QString("%1 - %2: %3: %4").arg(curData, text, strInfo, msg);
    QxHelp::makeFolder(LOG_PATH);
    QString strTime = LOG_PATH + QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString logName = strTime + ".log";
    QFile file(logName);
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream text_stream(&file);
    text_stream << strContext << "\r\n";
    file.flush();
    file.close();
    mutex.unlock();
}

int main(int argc, char *argv[])
{
    QSurfaceFormat format;
    format.setVersion(4, 5);                        // 设置 OpenGL 版本
    format.setProfile(QSurfaceFormat::CoreProfile); // 设置为 Core Profile
    format.setSwapInterval(1);                      // 设置垂直同步
    format.setDepthBufferSize(24);                  // 深度缓冲大小
    format.setStencilBufferSize(8);                 // 模板缓冲大小
    format.setSamples(4);                           // 抗锯齿采样数
    QSurfaceFormat::setDefaultFormat(format);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
#endif
    qInstallMessageHandler(logMessageHandler);
    QApplication a(argc, argv);
    QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
    QFont font("Noto Sans CJK SC"); // 指定简体中文字体
    a.setFont(font);

    QxHelp::SetStyle("app");

    MainWidget w;
    w.show();
    return a.exec();
}
