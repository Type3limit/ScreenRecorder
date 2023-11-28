//
// Created by Type3Limit on 2023/11/28.
//

#ifndef TESTWINDOW_H
#define TESTWINDOW_H

#include <QDialog>
#include "obswrapper.h"
QT_BEGIN_NAMESPACE
namespace Ui { class testwindow; }
QT_END_NAMESPACE

class testwindow : public QDialog {
Q_OBJECT

public:
    explicit testwindow(const QSharedPointer<ObsWrapper>& obs,QWidget *parent = nullptr);
    ~testwindow() override;
    void createDisplayer();
    void resizeEvent(QResizeEvent* event) override;
    static void DrawPreview(void* data, uint32_t cx, uint32_t cy);

private:
    Ui::testwindow *ui;
    QSharedPointer<ObsWrapper> m_obs;
    OBSDisplay m_displayer;
};


#endif //TESTWINDOW_H
