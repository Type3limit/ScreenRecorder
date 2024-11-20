#include "uploadpathwidget.h"
#include "qdebug.h"

#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>

class UploadPathWidgetPrivate {
public:
    explicit UploadPathWidgetPrivate(UploadPathWidget *q);
    void init();
    void setupProperties();
    void getHover(const QPoint &pos);
    void pressDir();
    void leave();

    UploadPathWidget *q_ptr;
    QIcon m_icon;
    QStringList m_dirs;
    QList<QRect> m_dirRects;
    QFont m_font;
    QColor m_color;
    QString m_split;
    QList<QRect> m_splitRects;
    QSize m_sizeHint;
    int m_hover = -1;
    QList<QMap<int, QVariant>> m_data;

    const int DEFAULTWIDET = 452;
    const int INTERVAL = 2;
    const QSize ICONSIZE{16, 16};
private:
    Q_DISABLE_COPY(UploadPathWidgetPrivate)
    Q_DECLARE_PUBLIC(UploadPathWidget)
};

UploadPathWidget::UploadPathWidget(QWidget *parent)
    : QWidget{parent},
      d_ptr(new UploadPathWidgetPrivate(this))
{
    Q_D(UploadPathWidget);
    d->init();
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

UploadPathWidget::~UploadPathWidget()
{
}

void UploadPathWidget::setIcon(const QIcon &icon)
{
    Q_D(UploadPathWidget);
    d->m_icon = icon;
    d->setupProperties();
}

QIcon UploadPathWidget::icon() const
{
    Q_D(const UploadPathWidget);
    return d->m_icon;
}

void UploadPathWidget::setDirs(const QStringList &dirs)
{
    Q_D(UploadPathWidget);
    d->m_dirs = dirs;
    d->m_data.clear();
//    for(int i = 0; i < d->m_dirs.size(); i++) {
//        d->m_data.append(QMap<Qt::ItemDataRole, QVariant>());
//    }
    d->setupProperties();
}

QStringList UploadPathWidget::dirs() const
{
    Q_D(const UploadPathWidget);
    return d->m_dirs;
}

void UploadPathWidget::setDatas(const QList<QMap<int, QVariant>> &datas)
{
    Q_D(UploadPathWidget);
    d->m_data = datas;
}

QList<QMap<int, QVariant> > UploadPathWidget::datas()
{
    Q_D(const UploadPathWidget);
    return d->m_data;
}

void UploadPathWidget::setData(int index, QVariant data, int userRole)
{
    Q_D(UploadPathWidget);
    if(index < d->m_dirs.size() && index >= 0) {
        d->m_data[index][userRole] = data;
    }
}

QVariant UploadPathWidget::data(int index, int userRole)
{
    Q_D(UploadPathWidget);
    if(index < d->m_dirs.size() && index >= 0) {
        return d->m_data[index][userRole];
    }
    return QVariant();
}

void UploadPathWidget::setFont(const QFont &font)
{
    Q_D(UploadPathWidget);
    d->m_font = font;
    d->setupProperties();
}

QFont UploadPathWidget::font() const
{
    Q_D(const UploadPathWidget);
    return d->m_font;
}

void UploadPathWidget::setColor(const QColor &color)
{
    Q_D(UploadPathWidget);
    d->m_color = color;
    d->setupProperties();
}

QColor UploadPathWidget::color() const
{
    Q_D(const UploadPathWidget);
    return d->m_color;
}

void UploadPathWidget::setSplit(const QString &split)
{
    Q_D(UploadPathWidget);
    d->m_split = split;
    d->setupProperties();
}

QString UploadPathWidget::split() const
{
    Q_D(const UploadPathWidget);
    return d->m_split;
}

void UploadPathWidget::paintEvent(QPaintEvent *event)
{
    Q_D(UploadPathWidget);
    QStyleOption option;
    option.init(this);
    //绘制Icon
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
    painter.save();
    d->m_icon.paint(&painter, QRect(QPoint(0, d->INTERVAL), d->ICONSIZE));
    painter.restore();
    //绘制字符
    painter.save();
    painter.setFont(d->m_font);
    for(int i = 0; i < d->m_dirs.count(); i++) {
        if(d->m_hover == i) {
            painter.setPen(Qt::white);
        }else {
            painter.setPen(d->m_color);
        }
//        painter.drawRect(d->m_dirRects[i]);
        painter.drawText(d->m_dirRects[i], 1, d->m_dirs[i]);
    }
    painter.restore();
    //绘制分隔符
    painter.save();
    painter.setFont(d->m_font);
    painter.setPen(d->m_color);
    for(int i = 0; i < d->m_splitRects.count(); i++) {
//        painter.drawRect(d->m_splitRects[i]);
        painter.drawText(d->m_splitRects[i], 1, d->m_split);
    }
    painter.restore();
}

QSize UploadPathWidget::sizeHint() const
{
    Q_D(const UploadPathWidget);
    qDebug() << "UploadPathWidget::sizeHint()" << d->m_sizeHint;
    return d->m_sizeHint;
}

void UploadPathWidget::mousePressEvent(QMouseEvent *event)
{
    Q_D(UploadPathWidget);
    d->getHover(event->pos());
    d->pressDir();
    d->getHover(event->pos());
}

void UploadPathWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
}

void UploadPathWidget::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(UploadPathWidget);
    d->getHover(event->pos());
}

void UploadPathWidget::leaveEvent(QEvent *event)
{
    Q_D(UploadPathWidget);
    d->leave();
}

void UploadPathWidget::resizeEvent(QResizeEvent *event)
{
    Q_D(UploadPathWidget);
    d->setupProperties();
}

UploadPathWidgetPrivate::UploadPathWidgetPrivate(UploadPathWidget *q)
    : q_ptr(q),
      m_icon(),
      m_dirs(),
      m_dirRects(),
      m_font(),
      m_color(),
      m_split(),
      m_splitRects(),
      m_sizeHint()
{

}

void UploadPathWidgetPrivate::init()
{
    m_icon = QIcon(":/icons/images/folder.svg");
    m_font = QFont("Microsoft YaHei UI");
    m_font.setWeight(QFont::Normal);
    m_font.setPixelSize(12);
    m_color = QColor("#99A1B0");
    m_split = ">";
}

void UploadPathWidgetPrivate::setupProperties()
{
    Q_Q(UploadPathWidget);
    int widthSum = ICONSIZE.width() + INTERVAL;
    int height = ICONSIZE.height();
    int currentTop = INTERVAL;
    m_dirRects.clear();
    m_splitRects.clear();
    QFontMetrics fm(m_font);
    for (int i = 0; i < m_dirs.count(); i++) {
        QSize dirSize = fm.boundingRect(m_dirs[i]).size();
        if(dirSize.width() > q->width()) {
            height = qMax(height, dirSize.height());
            QRect rect(widthSum, currentTop, q->width() - widthSum, dirSize.height());
            widthSum = q->width();
            m_dirRects.append(rect);
        }else {
            if(widthSum + dirSize.width() <= q->width()) {
                height = qMax(height, dirSize.height());
                QRect rect(widthSum, currentTop, dirSize.width(), dirSize.height());
                widthSum += dirSize.width();
                m_dirRects.append(rect);
            }else {
                widthSum = ICONSIZE.width() + INTERVAL;
                currentTop += INTERVAL + height;
                height = dirSize.height();
                QRect rect(widthSum, currentTop, dirSize.width(), dirSize.height());
                widthSum += dirSize.width();
                m_dirRects.append(rect);
            }
        }
        if(i != m_dirs.count() - 1) {
            QSize splitSize = fm.boundingRect(m_split).size();
            if(widthSum + INTERVAL + splitSize.width() + INTERVAL <= /*DEFAULTWIDET*/ q->width()) {
                height = qMax(height, splitSize.height());
                widthSum += INTERVAL;
                QRect rect(widthSum, currentTop, splitSize.width(), splitSize.height());
                widthSum += splitSize.width() + INTERVAL;
                m_splitRects.append(rect);
            }else {
                widthSum = ICONSIZE.width() + INTERVAL;
                currentTop += INTERVAL + height;
//                height = qMax(height, splitSize.height());
                height = splitSize.height();
                QRect rect(widthSum, currentTop, splitSize.width(), splitSize.height());
                widthSum += splitSize.width() + INTERVAL;
                m_splitRects.append(rect);
            }
        }
    }
    m_sizeHint = QSize(q->width(), currentTop + height + 2 * INTERVAL);
    q->setFixedHeight(currentTop + height + 2 * INTERVAL);
    q->update();
}

void UploadPathWidgetPrivate::getHover(const QPoint &pos)
{
    Q_Q(UploadPathWidget);
    for(int i = 0; i < m_dirRects.count() - 1; i++) {
        if(m_dirRects[i].contains(pos)) {
            if(m_hover != i) {
                m_hover = i;
                q->update();
            }
            return;
        }
    }
    if(m_hover != -1) {
        m_hover = -1;
        q->update();
    }
}

void UploadPathWidgetPrivate::pressDir()
{
    Q_Q(UploadPathWidget);
    if(m_hover != -1) {
        m_dirs = m_dirs.mid(0, m_hover + 1);
        m_data = m_data.mid(0, m_hover + 1);
        setupProperties();
        QMetaObject::invokeMethod(q, "dirsChanged", Qt::QueuedConnection,
                                  Q_ARG(const QStringList &, m_dirs));
    }
}

void UploadPathWidgetPrivate::leave()
{
    Q_Q(UploadPathWidget);
    m_hover = -1;
    q->update();
}
