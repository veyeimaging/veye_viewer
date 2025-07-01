#ifndef QXTOAST_H
#define QXTOAST_H

#include <QWidget>

namespace Ui {
class QxToast;
}

class QxToast : public QWidget
{
    Q_OBJECT

public:
    explicit QxToast(QWidget *parent = nullptr);
    ~QxToast();
    void initView();
    void setText(const QString &text);
    void showAnimation(int timeout = 3000); //动画方式show出，默认3秒后消失
    //静态调用
    static QWidget *showTip(const QString &text, QWidget *parent = nullptr, int timeout = 3000);

protected:
    virtual void paintEvent(QPaintEvent *event) override;

private:
    Ui::QxToast *ui;
};

#endif // QXTOAST_H
