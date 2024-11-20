//
// Created by 58226 on 2024/11/20.
//

#include "dragmovedialog.h"
#include <QMouseEvent>
void DragMoveDialog::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_leftButtonPressed)
    {
        return;
    }
    QPoint posOffset = e->globalPos() - m_pos;
    move(posOffset);

}

void DragMoveDialog::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton)
    {
        return;
    }
    m_pos = e->globalPos() - mapToGlobal({0, 0});
    m_leftButtonPressed = true;
}

void DragMoveDialog::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton)
    {
        return;
    }
    m_leftButtonPressed = false;
}
