#include "qxtoast.h"
#include <QDebug>
#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QScreen>
#include <QTimer>
#include "ui_qxtoast.h"

QxToast::QxToast(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::QxToast)
{
    ui->setupUi(this);
    initView();
}

QxToast::~QxToast()
{
    delete ui;
}

void QxToast::initView()
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMaximumWidth(600);
    ui->label->setMinimumWidth(300);
    ui->label->setStyleSheet("background: rgba(0,0,0,0.7);border-radius: 10px;font-size: "
                             "18px;font-family: Microsoft YaHei;font-weight: normal;color: "
                             "#FFFFFF;padding:20px;");
    ui->label->setAlignment(Qt::AlignCenter);
    ui->label->setWordWrap(true);
}

void QxToast::setText(const QString &text)
{
    ui->label->setText(text);
}

void QxToast::showAnimation(int timeout)
{
    //开始动画
    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(500);
    animation->setStartValue(0);
    animation->setEndValue(1);
    animation->start();
    show();
    QTimer::singleShot(timeout, [&] {
        //结束动画
        QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
        animation->setDuration(500);
        animation->setStartValue(1);
        animation->setEndValue(0);
        animation->start();
        connect(animation, &QPropertyAnimation::finished, [&] {
            close();
            deleteLater(); //关闭后析构
        });
    });
}

QWidget *QxToast::showTip(const QString &text, QWidget *parent, int timeout)
{
    QxToast *toast = new QxToast(parent);
    toast->setWindowFlags(toast->windowFlags() | Qt::WindowStaysOnTopHint); //置顶
    toast->showAnimation(timeout);
    toast->setText(text);
    toast->adjustSize(); //设置完文本后调整大小
    int px = 0;
    int py = 0;
    if (parent == nullptr) {
        //主屏的60%高度位置
        QScreen *pScreen = QGuiApplication::primaryScreen();
        px = (pScreen->size().width() - toast->width()) / 2;
        py = pScreen->size().height() * 6 / 10;
    } else {
        //弹框居中
        QPoint parentPos = parent->mapToGlobal(QPoint(0, 0));
        px = (parent->width() - toast->width()) / 2 + parentPos.x();
        py = (parent->height() - toast->height()) / 2 + parentPos.y();       
    }

    toast->move(px, py);
    toast->showAnimation();
    return toast;
}
//绘制背景为黑色
void QxToast::paintEvent(QPaintEvent *event)
{
    // QPainter paint(this);
    // paint.setPen(Qt::NoPen);
    // paint.setBrush(Qt::black);
    // paint.setRenderHint(QPainter::Antialiasing);
    // QPainterPath path;
    // path.addRoundedRect(rect(), 10, 10);
    // paint.setClipPath(path);                      //圆角边框去掉毛刺
    // QBrush brush = QBrush(qRgba(255, 0, 0, 0.7)); //黑色背景
    // paint.setBrush(brush);
    // paint.drawRect(rect());
}
