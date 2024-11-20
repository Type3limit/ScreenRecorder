//
// Created by 58226 on 2024/11/19.
//

#ifndef DISABLEVIEW_H
#define DISABLEVIEW_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class DisableView; }
QT_END_NAMESPACE

class DisableView : public QWidget {
Q_OBJECT

public:
    explicit DisableView(QWidget *parent = nullptr);
    ~DisableView() override;
    void resizeEvent(QResizeEvent* event) override;

    void setNoticeMessage(const QString& message);

private:
    Ui::DisableView *ui;
};


#endif //DISABLEVIEW_H
