#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QWidget>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QWidget
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog();

protected:
    void initUIStyle();
    void initSigSlot();

protected:
    void changeEvent(QEvent *event) override;

private:
    Ui::AboutDialog *ui;
};

#endif // ABOUTDIALOG_H
