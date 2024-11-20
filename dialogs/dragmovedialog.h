//
// Created by 58226 on 2024/11/20.
//

#ifndef DRAGMOVEDIALOG_H
#define DRAGMOVEDIALOG_H
#include <QDialog>


class DragMoveDialog:public QDialog
{
public:
    DragMoveDialog(QWidget* parent=nullptr)
        :QDialog(parent)
    {

    }
protected:
    virtual ~DragMoveDialog() = default;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

protected:
    QPoint m_pos = {0,0};
    bool m_leftButtonPressed = false;
};



#endif //DRAGMOVEDIALOG_H
