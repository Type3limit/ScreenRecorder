//
// Created by 58226 on 2023/11/22.
//

#ifndef BACKGROUNDWINDOW_H
#define BACKGROUNDWINDOW_H

#include <QWidget>
#include <QSvgRenderer>
#include <QMutex>
#include <QScreen>

class BackgroundWindow : public QWidget
{
    Q_OBJECT

    enum DragModel
    {
        none = -1,
        areaDivide = 0,
        leftTop = 1,
        leftBottom = 2,
        rightTop = 3,
        rightBottom =4,
        center =5
    };

public:
signals:
    void areaChanged(qreal x1,qreal y1,qreal x2,qreal y2);
    void requestHideWindow();
    signals:
    void onCutomizedWindowFinished();
public:
    BackgroundWindow(bool isFullScreenModel, QWidget* parent ,
                              const QScreen*  curScreen );
    ~BackgroundWindow() override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

    QRectF centerAnchorRect() const;
    QRectF leftTopRect() const;
    QRectF rightTopRect() const;
    QRectF leftBottomRect() const;
    QRectF rightBottomRect() const;

    void resetStatus(bool isFullScreenModel, const QScreen*  curScreen);
    void resetSelectionPos(int curWidth,int curHeight);
    void setSize(int Width,int Height);
public:
    volatile bool IsShown = false;
    volatile bool IsClosed = false;
private:
    QPointF m_startPos = {0, 0};
    QPointF m_endPos = {0, 0};
    QPointF m_centerStartPos = {0,0};
    QSvgRenderer m_renderer;
    QColor m_drawingColor;
    volatile bool m_IsDragMoveModel = false;
    volatile bool m_IsFirstAreaCompelete = false;
    volatile bool m_isFullScreen = false;

    QMutex dataMutex;
    QRectF m_scrrenRegion ={0,0,0,0};
    DragModel m_dragModel = none;
};


#endif //BACKGROUNDWINDOW_H
