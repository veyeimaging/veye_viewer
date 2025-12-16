#ifndef COMMSTUCT_H
#define COMMSTUCT_H
#include <QObject>
#include <QString>
#include "CommHeader.h"

enum class CurPage {
    ImgAttrDlg,
    ExpGainDlg,
    WhiteBlanceDlg,
    ImgProcDlg,
    TriggerDlg,
    RegisterDlg,
    CurAttrDlg,
    TOTAL_Dlg
};

enum class EuCamType {
    GxCamera, // 代表 GX 系列相机
    MvCamera  // 代表 MV 系列相机
};

enum class EuPlatform {
    Rockchip,     // 瑞芯微平台
    RaspberryPi,  // 树莓派 (泛指或旧型号)
    RaspberryPi5, // 树莓派 5
    Jetson,       // NVIDIA Jetson 平台
    None          // 无/未知平台
};
struct CommDevCfg
{
    EuCamType cameType;  // 相机类型
    EuPlatform platform; // 运行平台
};
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
    int nBytesperline;
    int width;
    int height;
    int fps;
    QString strTS;
} StImgInfo;
Q_DECLARE_METATYPE(StImgInfo)

#ifdef Q_MOC_RUN
qRegisterMetaType<StImgInfo>("StImgInfo");
#endif

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
Q_DECLARE_METATYPE(StSaveImg)

typedef struct _tagCamIndex
{
    QString i2c_bus;
    int i2c_bus_num;
    QString media_node;
    QString video_node;
    QString video_subnode;
    QString media_entity_name;
} StCamIndex;
Q_DECLARE_METATYPE(StCamIndex)

typedef enum _tagTrgMode { Stream, Normal, Rolling, Pulse, CamSync } EuTrgMode;

enum EuSlaveMode { Master, Slave };

enum EuWBMode { MWB, AWB = 2 };

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
    fps_gx,
    maxfps,
    pixelformat,
    factoryparam,
    paramsave,
    //gx cam resolution
    videomode,
    readmode,
    videomodenum,
    videomodewh1,
    videomodeparam1,
    videomodewh2,
    videomodeparam2,
    videomodewh3,
    videomodeparam3,
    videomodewh4,
    videomodeparam4,
    videomodewh5,
    videomodeparam5,
    videomodewh6,
    videomodeparam6,
    videomodewh7,
    videomodeparam7,
    videomodewh8,
    videomodeparam8,
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

    workmodecap, //gx
    workmode,    //gx
    slavemode,   //gx
    trgmodecap,  //mv
    imgacq,
    trgmode, //mv
    trgsrc,
    trgexp_delay,
    trgnum,
    trginterval,
    trgfilter_time,
    trgdelay,
    trgone,
    //writebalnce
    wbmode,
    awbcolortempmin,
    awbcolortempmax,
    mwbrgain,
    mwbbgain,
    colortemp,
    currgain,
    curbgain,
    //gx
    saturation,
    contrast,
    hue

} EuCMD;
Q_DECLARE_METATYPE(EuCMD)

typedef struct _tagI2CCmd
{
    EuCamType camType;
    bool bWrite;
    int cmd;
    QString i2c;
    int32_t addr;
    int32_t reg;
    int32_t data;
    QObject *obj;
    bool showlog;
    QString sysCmd;
} StI2CCmd;
Q_DECLARE_METATYPE(StI2CCmd)

typedef struct _tagCmdRet
{
    int cmd;
    int32_t data;
    QObject *obj;
    bool show;
    QString strReg;
} StCmdRet;
Q_DECLARE_METATYPE(StCmdRet)

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
Q_DECLARE_METATYPE(StRegInfo)

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
Q_DECLARE_METATYPE(StCamParam)

typedef struct _tagCamInfo
{
    int left;
    int top;
    int width;
    int height;
    double fps;
} StCamInfo;
Q_DECLARE_METATYPE(StCamInfo)

typedef struct _tagImgAttrInfo
{
    bool read = true;
    QString strX;
    QString strY;
    QString strW;
    QString strH;
    QString strFps;
} StImgAttrInfo;
Q_DECLARE_METATYPE(StImgAttrInfo)

#endif // COMMSTUCT_H
