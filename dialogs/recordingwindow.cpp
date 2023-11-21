//
// Created by 58226 on 2023/11/21.
//

// You may need to build the project (run Qt uic code generator) to get "ui_RecordingWindow.h" resolved

#include "recordingwindow.h"
#include "dialog/ui_RecordingWindow.h"
#include <QMouseEvent>

RecordingWindow::RecordingWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::RecordingWindow) {
    ui->setupUi(this);
    // hide title bar
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
}

RecordingWindow::~RecordingWindow() {
    delete ui;
}

void RecordingWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        return;
    }
    m_pos = e->globalPos() - mapToGlobal({0,0}) ;
    m_leftButtonPressed = true;
}

void RecordingWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_leftButtonPressed) {
        return;
    }
    QPoint posOffset = e->globalPos() - m_pos;
    move( posOffset);
}

void RecordingWindow::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        return;
    }
    m_leftButtonPressed = false;
}