//
// Created by 58226 on 2023/11/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_backgroundwindow.h" resolved

#include "backgroundwindow.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QDesktopWidget>
#include <QDebug>
#include <QIcon>


#define ANCHOR_SIZE 50.0
#define CORNOR_SIZE 18.0
#define TEXT_SIZE 300.0
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
BackgroundWindow::BackgroundWindow(bool isFullScreenModel, QWidget* parent, const QScreen*  curScreen) :
    QWidget(parent)
{
    m_renderer.load(QString(":/icons/images/centerAnchor.svg"));
    m_drawingColor = QColor(255, 154, 0);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::SubWindow);
    resetStatus(isFullScreenModel,curScreen);
    setMouseTracking(true);
    setWindowIcon(QIcon(QString(":/icons/images/recording.svg")));
    setWindowTitle(u8"录制区");
}

BackgroundWindow::~BackgroundWindow()
{
}

void BackgroundWindow::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    //p.fillRect(rect(), QBrush(Qt::red));
    if (m_isFullScreen)
    {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(m_drawingColor, 1, Qt::DashLine));
        p.drawRect(rect());
    }
    else
    {
        if (!m_IsFirstAreaCompelete)
        {
            p.setBrush(QBrush({0, 0, 0, 128}));
            p.drawRect(rect());
            p.setPen(QPen(m_drawingColor, 1, Qt::DashLine));
            p.setBrush(Qt::NoBrush);
            p.drawRect(QRectF(m_startPos, m_endPos));
            QRectF textRect =
            {
                rect().left() + ((rect().right() - rect().left())/2 - (TEXT_SIZE / 2)),
                rect().top() + ((rect().bottom() - rect().top())/2 - (TEXT_SIZE / 2))+100,
                TEXT_SIZE,
                TEXT_SIZE
            };
            p.setFont(QFont("Microsoft YaHei",20));
            p.drawText(textRect,QString(u8"在此处框选自定义区域"));
        }
        else
        {
            p.setPen(QPen(m_drawingColor, 1, Qt::DashLine));
            p.setBrush(QBrush({0,0,0,1}));
            p.drawRect(leftTopRect());
            p.drawRect(rightTopRect());
            p.drawRect(leftBottomRect());
            p.drawRect(rightBottomRect());
            if(!centerAnchorRect().isEmpty())
            {
                m_renderer.render(&p, centerAnchorRect());
            }

        }
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(m_startPos, m_endPos));
    }

}

void BackgroundWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        auto curCursorPos = event->pos();
        m_IsDragMoveModel = true;
        auto checkRegion = [&]()
        {
            if (centerAnchorRect().contains(curCursorPos))
            {
                m_dragModel = DragModel::center;
            }
            else if (leftTopRect().contains(curCursorPos))
            {
                m_dragModel = DragModel::leftTop;
            }
            else if (leftBottomRect().contains(curCursorPos))
            {
                m_dragModel = DragModel::leftBottom;
            }
            else if (rightBottomRect().contains(curCursorPos))
            {
                m_dragModel = DragModel::rightBottom;
            }
            else if (rightTopRect().contains(curCursorPos))
            {
                m_dragModel = DragModel::rightTop;
            }
        };
        if (m_isFullScreen)
        {
            checkRegion();
        }
        else
        {
            if (!m_IsFirstAreaCompelete)
            {
                m_dragModel = DragModel::areaDivide;
                m_startPos = curCursorPos;
            }
            else
            {
                checkRegion();
                if(m_dragModel==DragModel::center)
                {
                    m_centerStartPos = curCursorPos;
                }
            }
        }
    }
}

void BackgroundWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_IsDragMoveModel)
    {
        m_IsDragMoveModel =false;
        auto curCursorPos = event->pos();
        if (m_dragModel == DragModel::areaDivide)
        {
            if((abs(curCursorPos.x()-m_startPos.x())
                <50)&&(abs(curCursorPos.y()-m_startPos.y())<50))
            {
                m_startPos = {0,0};
                m_endPos = {static_cast<qreal>(rect().width()),static_cast<qreal>(rect().height())};
                emit areaChanged(m_startPos.x(),m_startPos.y(),m_endPos.x(),m_endPos.y());
                return;
            }
            m_endPos = curCursorPos;
            m_IsFirstAreaCompelete = true;

        }
        m_dragModel = DragModel::none;
        auto startX = min(m_endPos.x(), m_startPos.x());
        auto startY = min(m_endPos.y(), m_startPos.y());
        auto endX = max(m_endPos.x(), m_startPos.x());
        auto endY = max(m_endPos.y(), m_startPos.y());
        m_startPos = {startX,startY};
        m_endPos = {endX,endY};
        emit areaChanged(m_startPos.x(),m_startPos.y(),m_endPos.x(),m_endPos.y());
        update();
    }
}

void BackgroundWindow::mouseMoveEvent(QMouseEvent* event)
{
    auto curCursorPos = event->pos();

    if(m_IsDragMoveModel&&!m_isFullScreen)
    {

        switch (m_dragModel)
        {
        case none:
            {
                break;
            }
        case areaDivide:
            {
                m_endPos = curCursorPos;
                break;
            }

        case leftTop:
            {
                m_startPos = curCursorPos;
                break;
            }

        case leftBottom:
            {
                m_startPos.setX(curCursorPos.x());
                m_endPos.setY(curCursorPos.y());
                break;
            }

        case rightTop:
            {
                m_endPos.setX(curCursorPos.x());
                m_startPos.setY(curCursorPos.y());
                break;
            }

        case rightBottom:
            {
                m_endPos = curCursorPos;
                break;
            }

        case center:
            {
                auto diffX = curCursorPos.x()  - m_centerStartPos.x();
                auto diffY = curCursorPos.y() - m_centerStartPos.y();
                m_startPos.setX(m_startPos.x()+diffX);
                m_startPos.setY(m_startPos.y()+diffY);
                m_endPos.setX(m_endPos.x()+diffX);
                m_endPos.setY(m_endPos.y()+diffY);
                m_centerStartPos = curCursorPos;
                break;
            }
        }
    }
    if(centerAnchorRect().contains(curCursorPos))
    {
        this->setCursor(Qt::CursorShape::SizeAllCursor);
    }
    else if(leftTopRect().contains(curCursorPos))
    {
        this->setCursor(Qt::CursorShape::SizeFDiagCursor);
    }
    else if(leftBottomRect().contains(curCursorPos))
    {
        this->setCursor(Qt::CursorShape::SizeBDiagCursor);
    }
    else if(rightTopRect().contains(curCursorPos))
    {
        this->setCursor(Qt::CursorShape::SizeBDiagCursor);
    }
    else if(rightBottomRect().contains(curCursorPos))
    {
        this->setCursor(Qt::CursorShape::SizeFDiagCursor);
    }
    else
    {
       this->setCursor(Qt::CursorShape::ArrowCursor);
    }
   this->update();
}


void BackgroundWindow::resetStatus(bool isFullScreenModel, const QScreen*  curScreen)
{
    QMutexLocker locker(&dataMutex);
    m_isFullScreen = isFullScreenModel;
    auto sRegion = curScreen->geometry();
    qreal devicePixelRatio = curScreen->devicePixelRatio();
    QRect FullScreenRegion = {
        (int)(sRegion.x() / devicePixelRatio),
        (int)(sRegion.y() / devicePixelRatio),
        sRegion.width(),
        sRegion.height()
    };
    m_scrrenRegion = FullScreenRegion;
    setFixedWidth(FullScreenRegion.width());
    setFixedHeight(FullScreenRegion.height());
    //设置两次相同的geometry会导致问题？
    setGeometry((FullScreenRegion));
    resetSelectionPos(FullScreenRegion.width(),FullScreenRegion.height());
    locker.unlock();
}

void BackgroundWindow::resetSelectionPos(int curWidth, int curHeight)
{
    m_startPos = {0,0};
    m_endPos = {(qreal)(curWidth),(qreal)(curHeight)};
    m_centerStartPos = {0,0};
    m_dragModel = DragModel::none;
    m_IsFirstAreaCompelete =false;
    m_IsDragMoveModel =false;
    update();
}

void BackgroundWindow::setSize(int Width, int Height)
{
    m_endPos.setX(m_startPos.x()+Width);
    m_endPos.setY(m_startPos.y()+Height);
    update();
}

void BackgroundWindow::showEvent(QShowEvent* event)
{
    IsShown = true;
    IsClosed =false;
    QWidget::showEvent(event);
}

void BackgroundWindow::closeEvent(QCloseEvent* event)
{
    IsShown = false;
    IsClosed = true;
    QWidget::closeEvent(event);
}

void BackgroundWindow::hideEvent(QHideEvent* event)
{
    IsShown = false;
    QWidget::hideEvent(event);
}

#pragma region rects

QRectF BackgroundWindow::centerAnchorRect() const
{
    if ((abs(m_endPos.x() - m_startPos.x()) <= ANCHOR_SIZE) ||
        (abs(m_endPos.y() - m_startPos.y()) <= ANCHOR_SIZE))
    {
        return {0, 0, 0, 0};
    }
    auto startX = min(m_endPos.x(), m_startPos.x());
    auto startY = min(m_endPos.y(), m_startPos.y());
    auto endX = max(m_endPos.x(), m_startPos.x());
    auto endY = max(m_endPos.y(), m_startPos.y());

    return {
        startX + ((endX - startX)/2 - (ANCHOR_SIZE / 2)),
        startY + ((endY - startY)/2 - (ANCHOR_SIZE / 2)),
        ANCHOR_SIZE,
        ANCHOR_SIZE
    };
}

QRectF BackgroundWindow::leftTopRect() const
{
    if ((abs(m_endPos.x() - m_startPos.x()) <= CORNOR_SIZE) ||
        (abs(m_endPos.y() - m_startPos.y()) <= CORNOR_SIZE))
    {
        return {0, 0, 0, 0};
    }
    auto startX = min(m_endPos.x(), m_startPos.x());
    auto startY = min(m_endPos.y(), m_startPos.y());
    return {startX, startY,CORNOR_SIZE,CORNOR_SIZE};
}

QRectF BackgroundWindow::rightTopRect() const
{
    if ((abs(m_endPos.x() - m_startPos.x()) <= CORNOR_SIZE) ||
        (abs(m_endPos.y() - m_startPos.y()) <= CORNOR_SIZE))
    {
        return {0, 0, 0, 0};
    }
    auto startY = min(m_endPos.y(), m_startPos.y());
    auto endX = max(m_endPos.x(), m_startPos.x());
    return {endX - CORNOR_SIZE, startY,CORNOR_SIZE,CORNOR_SIZE};
}

QRectF BackgroundWindow::leftBottomRect() const
{
    if ((abs(m_endPos.x() - m_startPos.x()) <= CORNOR_SIZE) ||
        (abs(m_endPos.y() - m_startPos.y()) <= CORNOR_SIZE))
    {
        return {0, 0, 0, 0};
    }
    auto startX = min(m_endPos.x(), m_startPos.x());
    auto endY = max(m_endPos.y(), m_startPos.y());

    return {startX, endY - CORNOR_SIZE,CORNOR_SIZE,CORNOR_SIZE};
}

QRectF BackgroundWindow::rightBottomRect() const
{
    if ((abs(m_endPos.x() - m_startPos.x()) <= CORNOR_SIZE) ||
        (abs(m_endPos.y() - m_startPos.y()) <= CORNOR_SIZE))
    {
        return {0, 0, 0, 0};
    }
    auto endX = max(m_endPos.x(), m_startPos.x());
    auto endY = max(m_endPos.y(), m_startPos.y());

    return {endX - CORNOR_SIZE, endY - CORNOR_SIZE,CORNOR_SIZE,CORNOR_SIZE};
}


#pragma endregion
