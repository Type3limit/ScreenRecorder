//
// Created by Type3Limit on 2023/11/24.
//

#ifndef COUNTDOWNDIALOG_H
#define COUNTDOWNDIALOG_H

#include <QDialog>
#include <QScreen>
#include <QTimer>
class ObsWrapper;
QT_BEGIN_NAMESPACE

namespace Ui
{
    class CountDownDialog;
}

QT_END_NAMESPACE

class CountDownDialog : public QDialog
{
    Q_OBJECT

public:
    CountDownDialog(int countDownSeconds,const QSharedPointer<ObsWrapper>& obs,QScreen* curSc,QWidget* parent = nullptr);
    ~CountDownDialog() override;
    void setDialogCenter(QScreen* curSc);
    void keyPressEvent(QKeyEvent* event) override;

private:
    Ui::CountDownDialog* ui;
    QSharedPointer<ObsWrapper> m_obs;
    int m_countDownSeconds;
    QScreen* m_curScreen;
    QTimer m_timer;
};


#endif //COUNTDOWNDIALOG_H
