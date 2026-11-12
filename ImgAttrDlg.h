#ifndef IMGATTRDLG_H
#define IMGATTRDLG_H

#include <QDialog>
#include "CommStuct.h"

struct readMode
{
    QString width;
    QString height;
    QString fps;
    QString mode;
};
namespace Ui {
class ImgAttrDlg;
}

class ImgAttrDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ImgAttrDlg(CommDevCfg devCfg, QWidget *parent = nullptr);
    ~ImgAttrDlg();
signals:
    void setRoiAndFpsData(StImgAttrInfo &info);
    void sndCmd(EuCMD cmd, uint32_t data, bool bShow = true);
    void readCmd(EuCMD cmd, QObject *obj, bool bShow = true);
public slots:
    void rcvUpdateUi();
    void rcvSetROI(bool read);
    void rcvCmdRet(const StCmdRet &ret);
    void onSetRoiAndFpsData(bool read);

public:
    void setGxMode(bool bRoiMode);
    // 设备类型会在“选设备/切通道”时才探测出来，不能只在构造时快照一次
    void setDevCfg(CommDevCfg devCfg);

protected:
    void setupUi();
    // 按当前设备类型切换 MV/GX 面板可见性
    void applyCamType();

    std::pair<uint16_t, uint16_t> parseWidthHeight(uint32_t data);
    std::pair<uint8_t, uint16_t> getParam(uint32_t data);

private:
    Ui::ImgAttrDlg *ui;
    CommDevCfg m_devCfg;
    QMap<int, readMode> m_readModes;
    bool m_bGXRoiMode = false; // false=VideoMode, true=ROI mode
};

#endif // IMGATTRDLG_H
