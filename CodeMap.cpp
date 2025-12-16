#include "CodeMap.h"

CodeMap::CodeMap() {}

QString CodeMap::getMapModel(int key)
{
    if (mapModel.contains(key)) {
        return mapModel[key];
    } else {
        return "";
    }
}

QString CodeMap::getPixelFormat(int key)
{
    if (mapPixelFormat.contains(key)) {
        return mapPixelFormat[key];
    } else {
        return "";
    }
}

QString CodeMap::getTrgMode(int key)
{
    if (getMapTrgMode().contains(key)) {
        return getMapTrgMode()[key];
    } else {
        return "";
    }
}

QString CodeMap::getTrgSrc(int key)
{
    if (getMapTrgSrc().contains(key)) {
        return getMapTrgSrc()[key];
    } else {
        return "";
    }
}

int CodeMap::getDataType(QString key)
{
    if (mapDataType.contains(key)) {
        return mapDataType[key];
    } else {
        return -1;
    }
}

int CodeMap::getRdWt(QString key)
{
    if (mapRdWt.contains(key)) {
        return mapRdWt[key];
    } else {
        return -1;
    }
}

int CodeMap::getUpdateTiming(QString key)
{
    if (mapUpdateTiming.contains(key)) {
        return mapUpdateTiming[key];
    } else {
        return -1;
    }
}

// 实现静态方法
const QMap<int, QString> &CodeMap::getMapTrgMode()
{
    static QMap<int, QString> map = {{Stream, tr("流模式")},
                                     {Normal, tr("普通触发")},
                                     {Rolling, tr("滚动快门多帧触发")},
                                     {Pulse, tr("电平触发")}};
    return map;
}

const QMap<int, QString> &CodeMap::getMapTrgSrc()
{
    static QMap<int, QString> map = {{0, tr("软触发")}, {1, tr("硬触发")}};
    return map;
}
