#ifndef FRAMELESSWIDGET_H
#define FRAMELESSWIDGET_H

#include <QWidget>

class FramelessWidget : public QWidget

{
    Q_OBJECT
public:
    explicit FramelessWidget(QWidget *parent = 0);

protected:
    //窗体显示的时候触发
    void showEvent(QShowEvent *event);

    //事件过滤器识别拖动拉伸等
    void doWindowStateChange(QEvent *event);
    void doResizeEvent(QEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);

    //拦截系统事件用于修复系统休眠后唤醒程序的bug

    bool nativeEvent(const QByteArray &eventType, void *message, long *result);

private:
    //边距+可移动+可拉伸
    int padding;
    bool moveEnable;
    bool resizeEnable;

    //标题栏控件
    QWidget *titleBar;

    //鼠标是否按下+按下坐标+按下时窗体区域
    bool mousePressed;
    QPoint mousePoint;
    QRect mouseRect;

    //鼠标是否按下某个区域+按下区域的大小
    //依次为 左侧+右侧+上侧+下侧+左上侧+右上侧+左下侧+右下侧
    QList<bool> pressedArea;
    QList<QRect> pressedRect;

    //记录是否最小化
    bool isMin;
    //存储窗体默认的属性
    Qt::WindowFlags flags;

public slots:
    //设置边距+可拖动+可拉伸
    void setPadding(int padding);
    void setMoveEnable(bool moveEnable);
    void setResizeEnable(bool resizeEnable);

    //设置标题栏控件
    void setTitleBar(QWidget *titleBar);

signals:
    void titleDblClick();
};

#endif // FRAMELESSWIDGET_H
