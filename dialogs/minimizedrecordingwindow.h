//
// Created by 58226 on 2023/11/22.
//

#ifndef MINIMIZEDRECORDINGWINDOW_H
#define MINIMIZEDRECORDINGWINDOW_H

#include <QDialog>

class ObsWrapper;
QT_BEGIN_NAMESPACE
namespace Ui { class MinimizedRecordingWindow; }
QT_END_NAMESPACE

class MinimizedRecordingWindow : public QDialog {
Q_OBJECT

public:
    signals:
    void onRecover();
    void closed();
    void onRecordAct();
public:
    explicit MinimizedRecordingWindow(const QSharedPointer<ObsWrapper>& obs_wrapper,QWidget *parent = nullptr);
    ~MinimizedRecordingWindow() override;
    void closeEvent(QCloseEvent* event) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void onAudioFrame(const float currentMagnitude[8],const float currentPeak[8],const float currentInputPeak[8],bool isMicphone);
private:
    Ui::MinimizedRecordingWindow *ui;
    QSharedPointer<ObsWrapper> m_obs;
    QPoint m_pos = {0,0};
    bool m_leftButtonPressed = false;
    float m_currentMagnitude[8]{-90};
    float m_currentPeak[8]{-90};
    float m_currentInputPeak[8]{-90};
};


#endif //MINIMIZEDRECORDINGWINDOW_H
