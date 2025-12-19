#include <QApplication>
#include <QLocale>
#include <QMutex>
#include <QSurfaceFormat>
#include <QTranslator>
#include <QWindow>
#include "MainWidget.h"
#include "QxHelp.h"
#include "Version.h"
#include <cstdio>

#define LOG_PATH QCoreApplication::applicationDirPath() + "/log/"

// -------------------------- 日志清理函数（不变） --------------------------
void cleanExpiredLogFiles(const QString &logDirPath, int keepDays = 7)
{
    QDir logDir(logDirPath);
    if (!logDir.exists()) {
        qInfo() << "Log directory does not exist, no need to clean. Path:" << logDirPath;
        return;
    }

    QStringList filter;
    filter << "????-??-??.log";
    logDir.setNameFilters(filter);

    QDate currentDate = QDate::currentDate();
    QDate deadlineDate = currentDate.addDays(-keepDays);
    qInfo() << "Start cleaning expired logs. Keep days:" << keepDays
            << "| Deadline date:" << deadlineDate.toString("yyyy-MM-dd");

    int deletedCount = 0;
    QFileInfoList logFileList = logDir.entryInfoList(QDir::Files, QDir::Time);
    for (const QFileInfo &fileInfo : std::as_const(logFileList)) {
        QString fileName = fileInfo.fileName();
        QString dateStr = fileName.left(fileName.lastIndexOf('.'));
        QDate fileDate = QDate::fromString(dateStr, "yyyy-MM-dd");

        if (!fileDate.isValid()) {
            qWarning() << "Invalid log file name format, skip. File:" << fileName;
            continue;
        }

        if (fileDate < deadlineDate) {
            QFile file(fileInfo.absoluteFilePath());
            if (file.remove()) {
                qInfo() << "Deleted expired log file. File:" << fileName
                        << "| Create date:" << fileDate.toString("yyyy-MM-dd");
                deletedCount++;
            } else {
                qCritical() << "Failed to delete expired log file. File:" << fileName;
            }
        }
    }

    qInfo() << "Log clean completed. Total deleted files:" << deletedCount;
}

// -------------------------- 带颜色和实时输出的日志处理器 --------------------------
static QMutex logMutex;

// ANSI颜色码（控制台日志颜色）：格式为 \033[颜色码m，重置为 \033[0m
#define ANSI_COLOR_RESET "\033[0m"
#define ANSI_COLOR_GREEN "\033[32m"  // Info 绿色
#define ANSI_COLOR_YELLOW "\033[33m" // Warning 黄色
#define ANSI_COLOR_RED "\033[31m"    // Critical/Fatal 红色
#define ANSI_COLOR_CYAN "\033[36m"   // Debug 青色

void logMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    logMutex.lock();

    // 1. 日志级别+颜色码（控制台用）+ 文件用级别文本
    QString levelText;
    QString consoleColorCode; // 控制台日志的颜色码
    switch (type) {
    case QtDebugMsg:
        levelText = "DEBUG";
        consoleColorCode = ANSI_COLOR_CYAN;
        break;
    case QtInfoMsg:
        levelText = "INFO ";
        consoleColorCode = ANSI_COLOR_GREEN;
        break;
    case QtWarningMsg:
        levelText = "WARN ";
        consoleColorCode = ANSI_COLOR_YELLOW;
        break;
    case QtCriticalMsg:
        levelText = "CRIT ";
        consoleColorCode = ANSI_COLOR_RED;
        break;
    case QtFatalMsg:
        levelText = "FATAL";
        consoleColorCode = ANSI_COLOR_RED;
        break;
    }

    // 2. 格式化时间（精确到毫秒）
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    // 3. 提取文件名和行号（修复类型问题，直接截断）
    const int MAX_FILENAME_LEN = 20;
    QString fileStr = "unknown_file";
    if (context.file) {
        QString fullFilePath = QString::fromLocal8Bit(context.file);
        QFileInfo fileInfo(fullFilePath);
        fileStr = fileInfo.fileName();
        // 直接截断，避免 std::min 类型问题
        if (fileStr.length() > MAX_FILENAME_LEN) {
            fileStr = fileStr.left(MAX_FILENAME_LEN - 3) + "...";
        }
    }
    QString lineStr = (context.line > 0) ? QString("%1").arg(context.line, 4, 10, QChar('0'))
                                         : "----";

    // 4. 强化去引号逻辑（全局替换日期中的引号）
    QString cleanMsg = msg.trimmed();
    // 关键：全局替换所有双引号为空字符串（解决中间嵌套引号问题）
    cleanMsg.replace("\"", "");

    // 5. 构建日志字符串（文件日志+控制台日志）
    QString fileLogLine = QString("[%1][%2][%3:%4] %5")
                              .arg(timeStr)
                              .arg(levelText)
                              .arg(fileStr)
                              .arg(lineStr)
                              .arg(cleanMsg);
    QString consoleLogLine = QString("%1[%2][%3][%4:%5] %6%7")
                                 .arg(consoleColorCode)
                                 .arg(timeStr)
                                 .arg(levelText)
                                 .arg(fileStr)
                                 .arg(lineStr)
                                 .arg(cleanMsg)
                                 .arg(ANSI_COLOR_RESET);

    // 6. 写入日志文件（不变）
    QDir logDir(LOG_PATH);
    if (!logDir.exists())
        logDir.mkpath(".");
    QString logFileName = LOG_PATH + QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".log";
    QFile file(logFileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream textStream(&file);
        textStream << fileLogLine << "\n";
        textStream.flush();
        file.close();
    }

    // 7. 控制台实时输出（修复不及时问题：加 fflush）
    fprintf(stderr, "%s\n", consoleLogLine.toLocal8Bit().constData());
    fflush(stderr); // 强制刷新缓冲区，确保日志实时输出（关键！）

    logMutex.unlock();
}

int main(int argc, char *argv[])
{
    // QSurfaceFormat format;
    // format.setVersion(4, 5);                        // 设置 OpenGL 版本
    // format.setProfile(QSurfaceFormat::CoreProfile); // 设置为 Core Profile
    // format.setSwapInterval(1);                      // 设置垂直同步
    // format.setDepthBufferSize(24);                  // 深度缓冲大小
    // format.setStencilBufferSize(8);                 // 模板缓冲大小
    // format.setSamples(4);                           // 抗锯齿采样数
    // QSurfaceFormat::setDefaultFormat(format);

    qInstallMessageHandler(logMessageHandler);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#else
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    qputenv("QT_QPA_PLATFORM", "xcb");

    QApplication a(argc, argv);
    // 修复1：设置应用程序名称
    QApplication::setApplicationName(APP_NAME);
    // 修复2：显式设置应用程序显示名
    QApplication::setApplicationDisplayName(QObject::tr(APP_NAME));
    qDebug() << QApplication::applicationDisplayName();
    // 日志清理（启动+定时）
    const int logCleanlInterval = 24 * 60 * 60 * 1000;
    const int cleanExpiredDay = 30;
    cleanExpiredLogFiles(LOG_PATH, cleanExpiredDay);
    QTimer *logCleanTimer = new QTimer(&a);
    logCleanTimer->setInterval(logCleanlInterval);
    QObject::connect(logCleanTimer, &QTimer::timeout, [=]() {
        cleanExpiredLogFiles(LOG_PATH, cleanExpiredDay);
    });
    logCleanTimer->start();
    QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
    QFont font("Noto Sans CJK SC"); // 指定简体中文字体
    a.setFont(font);
    QxHelp::SetStyle("app");
    MainWidget w;
    w.show();
    QTimer::singleShot(50, &w, [&]() { // 延迟50ms确保窗口初始化完成
        QxHelp::setWindowIdentity(&w, APP_NAME);
    });
    return a.exec();
}
