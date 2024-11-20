#ifndef UPLOADPATHWIDGET_H
#define UPLOADPATHWIDGET_H

#include <QIcon>
#include <QWidget>

class UploadPathWidgetPrivate;

class UploadPathWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon)
    Q_PROPERTY(QStringList dirs READ dirs WRITE setDirs NOTIFY dirsChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont)
    Q_PROPERTY(QColor color READ color WRITE setColor)
    Q_PROPERTY(QString split READ split WRITE setSplit)
public:
    explicit UploadPathWidget(QWidget *parent = nullptr);
    ~UploadPathWidget();
    void setIcon(const QIcon &icon);
    QIcon icon() const;

    void setDirs(const QStringList &dirs);
    QStringList dirs() const;

    void setDatas(const QList<QMap<int, QVariant> > &datas);
    QList<QMap<int, QVariant>> datas();

    void setData(int index, QVariant data, int userRole = Qt::UserRole);
    QVariant data(int index, int userRole = Qt::UserRole);

    void setFont(const QFont &font);
    QFont font() const;

    void setColor(const QColor &color);
    QColor color() const;

    void setSplit(const QString &split);
    QString split() const;

signals:
    void dirsChanged(const QStringList &dirs);

    // QWidget interface
protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

protected:
    const QScopedPointer<UploadPathWidgetPrivate> d_ptr;

private:
    Q_DISABLE_COPY(UploadPathWidget)
    Q_DECLARE_PRIVATE(UploadPathWidget)
};

#endif // UPLOADPATHWIDGET_H
