#include "MsgBox.h"
#include <QMouseEvent>
#include <QStringLiteral>
#include "ui_MsgBox.h"

MsgBox::MsgBox(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MsgBox)
{
    ui->setupUi(this);
    setupUi();
}

MsgBox::~MsgBox()
{
    delete ui;
}

void MsgBox::SetMessage(const QString &title, const QString &msg, int type)
{
    ui->labelTitle->setText(title);
    if (type == 0) {
        ui->pushButtonCancel->setVisible(false);
    }
    ui->labelText->setWordWrap(true);
    QString strText = msg;
    ui->labelText->setText(strText);
    ui->labelText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // ui->pushButtonCancel->setText(strNO);
    // ui->pushButtonSave->setText(strYes);
}

void MsgBox::setupUi()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog /*| Qt::WindowStaysOnTopHint*/);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);
    // ui->line_1->setFrameShadow(QFrame::Sunken);
    // ui->line_2->setFrameShadow(QFrame::Sunken);
    ui->pushButtonClose->setIcon(QIcon(":/image/close.png"));
    ui->pushButtonPlaceholder->setEnabled(false);
    ui->pushButtonPlaceholder->setFixedSize(32, 32);
    ui->pushButtonPlaceholder->hide();

    ui->widgetDlgHead->setFixedHeight(40);
    ui->widgetBottom->setFixedHeight(50);
    ui->pushButtonClose->setFixedSize(32, 32);
    ui->pushButtonClose->hide();

    connect(ui->pushButtonCancel, &QPushButton::clicked, this, &MsgBox::close);
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &MsgBox::close);
    connect(ui->pushButtonSave, &QPushButton::clicked, this, [this]() {
        done(1);
        close();
    });
}

void MsgBox::mouseMoveEvent(QMouseEvent *e)
{
    if (mousePressed && (e->buttons() && Qt::LeftButton)) {
        this->move(e->globalPos() - mousePoint);
        e->accept();
    }
}

void MsgBox::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        mousePressed = true;
        mousePoint = e->globalPos() - this->pos();
        e->accept();
    }
}

void MsgBox::mouseReleaseEvent(QMouseEvent *)
{
    mousePressed = false;
}
