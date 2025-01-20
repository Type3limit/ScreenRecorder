//
// Created by 58226 on 2024/11/20.
//

#ifndef DRAGMOVEDIALOG_H
#define DRAGMOVEDIALOG_H
#include <QDialog>
#include <QtWidgets/QWidget>
#include <QtWidgets/QRubberBand>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtGui/QMouseEvent>


class ResizeableHelper : public QObject {
    Q_OBJECT

public:
    enum Edge {
        None = 0x0,
        Left = 0x1,
        Top = 0x2,
        Right = 0x4,
        Bottom = 0x8,
        TopLeft = 0x10,
        TopRight = 0x20,
        BottomLeft = 0x40,
        BottomRight = 0x80,
    };
    Q_ENUM(Edge);
    Q_DECLARE_FLAGS(Edges, Edge);

    ResizeableHelper(QWidget *target);

    void setBorderWidth(int w) {
        _borderWidth = w;
    }
    int borderWidth() const {
        return _borderWidth;
    }

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
    void mouseHover(QHoverEvent*);
    void mouseLeave(QEvent*);
    void mousePress(QMouseEvent*);
    void mouseRealese(QMouseEvent*);
    void mouseMove(QMouseEvent*);
    void updateCursorShape(const QPoint &);
    void calculateCursorPosition(const QPoint &, const QRect &, Edges &);

private:
    QWidget *_target = nullptr;
    QRubberBand *_rubberband = nullptr;
    bool _cursorchanged;
    bool _leftButtonPressed;
    Edges _mousePress = Edge::None;
    Edges _mouseMove = Edge::None;
    int _borderWidth;

    QPoint _dragPos;
    bool _dragStart = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ResizeableHelper::Edges);

class DragMoveDialog:public QDialog
{
public:
    DragMoveDialog(QWidget* parent=nullptr);
    DragMoveDialog(bool resizeable,QWidget* parent=nullptr);
protected:
    virtual ~DragMoveDialog() = default;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

protected:
    QPoint m_pos = {0,0};
    bool m_leftButtonPressed = false;
    ResizeableHelper* m_frameLess = nullptr;
};



#endif //DRAGMOVEDIALOG_H
