#ifndef COMMSTUCT_H
#define COMMSTUCT_H
#include <QObject>
#include <QString>

typedef enum _EuPlatform { Rockchip, RaspberryPi, RaspberryPi5, Jetson, FW_DEF } EuPlatform;

const int buf_max_size = 1024 * 1024 * 6;

typedef enum _strategy { change_img, v4l2_buf, new_buf } EuStrategy;

typedef struct _tagImgInfo
{
    int strategy; //1  img, 2 buf, 3 memcpy
    QImage img;
    int index;
    unsigned char *buf;
    // unsigned char buf[buf_max_size];
    int len;
    //int buf_size = buf_max_size;
    int width;
    int height;
    int fps;
    QString strTS;
} StImgInfo;
Q_DECLARE_METATYPE(StImgInfo);

typedef struct _tagSaveImg
{
    //QImage img;
    unsigned char *buf;
    //unsigned char buf[buf_max_size];
    //int buf_size = buf_max_size;
    int width;
    int height;
    int len;
    QString path;
    QString filename;
} StSaveImg;
Q_DECLARE_METATYPE(StSaveImg);

typedef struct _tagDevInfo
{
    QString i2c_bus;
    QString media_node;
    QString video_node;
    QString video_subnode;
    QString media_entity_name;
} StDevInfo;
Q_DECLARE_METATYPE(StDevInfo);

typedef enum _tagTrgMode { Stream, Normal, Rolling, Pulse } EuTrgMode;

typedef enum _tagRegRdWt { RW, RO, WO } EuRegRdWt;

typedef enum _tagDataType { _int_, _bool_, _enum_, _ascii_ } EuDataType;

typedef enum _tagUpdateTiming { AnyTime, Standby, ReadOnly } EuUpdateTiming;

typedef enum _tagCMD {
    def_cmd,
    aaroi_x,
    aaroi_y,
    aaroi_w,
    aaroi_h,

    model,
    serialno,
    version,
    roi_x,
    roi_y,
    roi_w,
    roi_h,
    fps,
    pixelformat,
    factoryparam,
    paramsave,
    /////////AEGE/////////
    expmode,
    aatarget,
    metime,
    aemaxtime,
    exptime,
    gainmode,
    mgain,
    agmaxgain,
    curgain,
    exptime_max,
    exptime_min,
    ///////Trigge/////////
    trgmodecap,
    imgacq,
    trgmode,
    trgsrc,
    trgexp_delay,
    trgnum,
    trginterval,
    trgfilter_time,
    trgdelay,
    trgone
    //trgedge
} EuCMD;

typedef struct _tagI2CCmd
{
    bool bWrite;
    int cmd;
    QString i2c;
    uint32_t addr;
    uint32_t reg;
    uint32_t data;
    QObject *obj;
    bool showlog;
} StI2CCmd;
Q_DECLARE_METATYPE(StI2CCmd);

typedef struct _tagCmdRet
{
    int cmd;
    uint32_t data;
    QObject *obj;
    bool show;
} StCmdRet;
Q_DECLARE_METATYPE(StCmdRet);

typedef struct _tagRegInfo
{
    QString name;
    QString address;
    QString rd_wt;
    QString data_type;
    QString update_timing;
    QString raw_support;
    QString data;
    QString description;
} StRegInfo;
Q_DECLARE_METATYPE(StRegInfo);

typedef struct _tagCamParam
{
    QString videoNode;
    QString subNode;
    int x;
    int y;
    int w;
    int h;
    int fps;
    //int captureType;
} StCamParam;
Q_DECLARE_METATYPE(StCamParam);

typedef struct _tagCamInfo
{
    int left;
    int top;
    int width;
    int height;
    double fps;
} StCamInfo;
Q_DECLARE_METATYPE(StCamInfo);

typedef struct _tagImgAttrInfo
{
    QString strX;
    QString strY;
    QString strW;
    QString strH;
    QString strFps;
} StImgAttrInfo;
Q_DECLARE_METATYPE(StImgAttrInfo);

// typedef struct _tagExpAndGainInfo
// {
//     QString model;
//     int expmode;
//     int aatarget;
//     int metime;
//     int aemaxtime;
//     int exptime;
//     int gainmode;
//     int mgain;
//     int agmaxgain;
//     int curgain;
//     int exptime_min;
//     int exptime_max;
// } StExpAndGainInfo;
// Q_DECLARE_METATYPE(StExpAndGainInfo);

// typedef struct _tagTriggerInfo
// {
//     int trgmodecap;
//     int imgacq;
//     int trgmode;
//     int trgsrc;

//     int trgexp_delay;
//     int trgnum;
//     int trginterval;
//     int trgfilter_time;
//     int trgdelay;

//     int trgone;
//     //int trgedge;
// } StTriggerInfo;
// Q_DECLARE_METATYPE(StTriggerInfo);

// typedef struct _tagCurAttrInfo
// {
//     QString strResolution;
//     QString strFps;
//     QString strFormat;
//     QString strTrgMode;
//     QString strTrgSrc;

// } StCurAttrInfo;
// Q_DECLARE_METATYPE(StCurAttrInfo);

#endif // COMMSTUCT_H
