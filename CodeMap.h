#ifndef CODEMAP_H
#define CODEMAP_H
#include "CommHeader.h"
#include "CommStuct.h"

class CodeMap : public QObject
{
    Q_OBJECT
public:
    CodeMap();

public:
    static QString getMapModel(int key);
    static QString getPixelFormat(int key);
    static QString getTrgMode(int key);
    static QString getTrgSrc(int key);
    static int getDataType(QString key);
    static int getRdWt(QString key);
    static int getUpdateTiming(QString key);

private:
    static const QMap<int, QString> &getMapTrgMode();
    static const QMap<int, QString> &getMapTrgSrc();

protected:
    inline static QMap<int, QString> mapModel = {{0x178, "MV-MIPI-IMX178M"},
                                                 {0x130, "MV-MIPI-SC130M"},
                                                 {0x265, "MV-MIPI-IMX265M"},
                                                 {0x178, "MV-MIPI-IMX178M"},
                                                 {0x130, "MV-MIPI-SC130M"},
                                                 {0x265, "MV-MIPI-IMX265M"},
                                                 {0x264, "MV-MIPI-IMX264M"},
                                                 {0x296, "MV-MIPI-IMX296M"},
                                                 {0x287, "MV-MIPI-IMX287M"},
                                                 {0x4002, "MV-MIPI-GMAX4002M"},
                                                 {0x8132, "RAW-MIPI-SC132M"},
                                                 {0x8462, "RAW-MIPI-IMX462M"},
                                                 {0x8535, "RAW-MIPI-SC535M"},
                                                 {0x8234, "RAW-MIPI-AR0234M"}};

    inline static QMap<int, QString> mapPixelFormat = {{0, "Mono8"},
                                                       {1, "Mono10"},
                                                       {2, "Mono12"},
                                                       {3, "Mono14(reserved)"},
                                                       {4, "UYVY"}};

    // inline static QMap<int, QString> mapTrgMode = {{Stream, tr("流模式")},
    //                                                {Normal, tr("普通触发")},
    //                                                {Rolling, tr("滚动快门多帧触发")},
    //                                                {Pulse, tr("电平触发")}};

    // inline static QMap<int, QString> mapTrgSrc = {{0, tr("软触发")}, {1, tr("硬触发")}};

    inline static QMap<QString, EuDataType> mapDataType = {{"int", _int_},
                                                           {"bool", _bool_},
                                                           {"enum", _enum_},
                                                           {"ascii", _ascii_}};

    inline static QMap<QString, EuRegRdWt> mapRdWt = {{"RW", RW}, {"RO", RO}, {"WO", WO}};

    inline static QMap<QString, EuUpdateTiming> mapUpdateTiming = {{"A", AnyTime},
                                                                   {"S", Standby},
                                                                   {"-", ReadOnly}};
};

#endif // CODEMAP_H
