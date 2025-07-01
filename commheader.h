#ifndef COMMHEADER_H
#define COMMHEADER_H

#include <QApplication>
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QPalette>
#include <QScreen>
#include <QStandardPaths>
#include <QString>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include <fcntl.h>
#include <linux/fb.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/types.h>
#include <linux/v4l2-subdev.h>
#include <linux/videodev2.h>
#include <queue>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define QX_SUCCESS 0
#define QX_FAILURE (-1)
const int TIEM_OUT = 10;

#endif // COMMHEADER_H
