#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    initUIStyle();
    initSigSlot();
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::initUIStyle()
{
    setAttribute(Qt::WA_TranslucentBackground, false);
    setWindowFlags(Qt::FramelessWindowHint | Qt::SubWindow | Qt::WindowStaysOnTopHint
                   | Qt::X11BypassWindowManagerHint);

    ui->pushButtonClose->setFixedWidth(42);
    ui->widgetDlgHead->setFixedHeight(40);
    ui->pushButtonClose->setIcon(QIcon(":/image/close.png"));
    ui->labelLogo->setStyleSheet("image: url(:/image/log0.ico);");
    ui->labelName->setStyleSheet("color:#326cf3; font-size: 30px;");
    ui->label_Version->setStyleSheet("color:#326cf3; ");
    ui->labelRight->setStyleSheet("color:#bdbdbd; ");
    ui->labelMIT->setStyleSheet("color:#bdbdbd; ");
    ui->labelWWW->setStyleSheet("color:#bdbdbd; ");
    ui->pushButtonPlaceholder->setEnabled(false);
    ui->pushButtonPlaceholder->setFixedHeight(20);
    setFixedSize(550, 300);
}

void AboutDialog::initSigSlot()
{
    connect(ui->pushButtonClose, &QPushButton::clicked, this, [this]() { close(); });
}

void AboutDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(event);
}
