#include "framelessdialog.h"

#include <QMouseEvent>
#include <QDebug>
#include <QPainter>

FramelessDialog::FramelessDialog(QWidget *parent)
    : QDialog(parent),
      m_pos(),
      m_leftButtonPressed(false)
{
    // hide title bar
    setWindowFlags(Qt::FramelessWindowHint);
}

void FramelessDialog::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        return;
    }
    m_pos = e->globalPos() -  mapToGlobal({0,0}) ;
    m_leftButtonPressed = true;
}

void FramelessDialog::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_leftButtonPressed) {
        return;
    }
    QPoint posOffset = e->globalPos() - m_pos;
    move( posOffset);
}

void FramelessDialog::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        return;
    }
    m_leftButtonPressed = false;
}

void FramelessDialog::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(QBrush(QColor(13, 13, 13,0)), 1);
    painter.setPen(pen);
    painter.drawRect(rect());
}
