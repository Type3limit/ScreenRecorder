//
// Created by 58226 on 2023/11/21.
//

#ifndef RECORDINGWINDOW_H
#define RECORDINGWINDOW_H

#include <QMainWindow>


QT_BEGIN_NAMESPACE
namespace Ui { class RecordingWindow; }
QT_END_NAMESPACE

class RecordingWindow : public QMainWindow {
Q_OBJECT

public:
    explicit RecordingWindow(QWidget *parent = nullptr);
    ~RecordingWindow() override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
private:
    QPoint m_pos;
    bool m_leftButtonPressed;
private:
    Ui::RecordingWindow *ui;
};


#endif //RECORDINGWINDOW_H
