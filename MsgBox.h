#ifndef MSGBOX_H
#define MSGBOX_H

#include <QDialog>
#include <QPoint>

namespace Ui {
class MsgBox;
}

class MsgBox : public QDialog
{
    Q_OBJECT

public:
    explicit MsgBox(QWidget *parent = nullptr);
    ~MsgBox();
    void SetMessage(const QString &title, const QString &msg, int type);

private:
    void setupUi();

protected:
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *);

private:
    QPoint mousePoint;
    bool mousePressed;

private:
    Ui::MsgBox *ui;
};

#endif // MSGBOX_H
