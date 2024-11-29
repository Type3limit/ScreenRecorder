//
// Created by 58226 on 2024/11/26.
//

#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <QMediaService>
#include <QMediaObject>
#include <QMouseEvent>
#include <QSlider>
#include <QVideoRendererControl>
#include <QVideoWidget>

class VideoRenderer : public QVideoRendererControl
{
public:
    QAbstractVideoSurface *surface() const override
    {
        return m_surface;
    }

    void setSurface(QAbstractVideoSurface *surface) override
    {
        m_surface = surface;
    }

    QAbstractVideoSurface *m_surface = nullptr;
};

class MediaService : public QMediaService
{
public:
    MediaService(VideoRenderer *vr, QObject* parent = nullptr)
        : QMediaService(parent)
        , m_renderer(vr)
    {
    }

    QMediaControl* requestControl(const char *name) override
    {
        if (qstrcmp(name, QVideoRendererControl_iid) == 0)
            return m_renderer;

        return nullptr;
    }

    void releaseControl(QMediaControl *) override
    {
    }

    VideoRenderer *m_renderer = nullptr;
};

class MediaObject : public QMediaObject
{
public:
    explicit MediaObject(VideoRenderer *vr, QObject* parent = nullptr)
        : QMediaObject(parent, new MediaService(vr, parent))
    {
    }
};

class VideoWidget : public QVideoWidget
{
public:

    VideoWidget(QWidget*parent = nullptr)
        : QVideoWidget(parent)
    {
    }

    bool setMediaObject(QMediaObject *object) override
    {
        return QVideoWidget::setMediaObject(object);
    }
};

class MousePressableSlider : public QSlider {
    Q_OBJECT

public:
    signals:
      void onMousePress(bool pressed);
public:
    MousePressableSlider(QWidget *parent = nullptr) : QSlider(parent) {
        setOrientation(Qt::Horizontal);
    }

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        auto curValure = value();
        if (event->key() == Qt::Key_Left)
        {
            auto targetValue = curValure- 1<minimum()?minimum():curValure;
            setValue(targetValue);
            emit valueChanged(targetValue);
            event->accept();
        }
        if (event->key() == Qt::Key_Right)
        {
            auto targetValue = curValure + 1>maximum()?maximum():curValure;
            setValue(targetValue);
            emit valueChanged(targetValue);
            event->accept();
        }
        QSlider::keyPressEvent(event);
    }
    void mousePressEvent(QMouseEvent *event) override {
        // 获取鼠标点击的 x 坐标
        int clickPosition = event->x();

        // 获取滑块的宽度和当前最小值、最大值
        int sliderWidth = width();
        int minValue = minimum();
        int maxValue = maximum();

        // 根据鼠标点击的 x 坐标来计算滑块的新值
        int newValue = (clickPosition * (maxValue - minValue)) / sliderWidth + minValue;

        emit onMousePress(true);
        // 设置滑块的新值
        setValue(newValue);

        // 发射值改变信号
        emit valueChanged(newValue);

        // 调用基类事件处理
        QSlider::mousePressEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent* event) override
    {
        emit onMousePress(false);
        QSlider::mouseReleaseEvent(event);
    }
};

#endif //VIDEORENDERER_H
