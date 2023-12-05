#ifndef FRAMELESSDIALOG_H
#define FRAMELESSDIALOG_H

#include <QDialog>

class FramelessDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FramelessDialog(QWidget *parent = nullptr);

protected:
    // Dialog移除边框之后的拖动实现
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

    void paintEvent(QPaintEvent *) override;

private:
    QPoint m_pos;
    bool m_leftButtonPressed;
};

#endif // FRAMELESSDIALOG_H
