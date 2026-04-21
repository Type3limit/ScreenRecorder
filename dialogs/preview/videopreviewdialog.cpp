//
// Created by 58226 on 2024/11/26.
//

// You may need to build the project (run Qt uic code generator) to get "ui_VideoPreviewDialog.h" resolved

#include "videopreviewdialog.h"
#include "QtAVPlayer/qavplayer.h"
#include "QtAVPlayer/qavvideoframe.h"
#include "QtAVPlayer/qavaudiooutput.h"
#include "ui_VideoPreviewDialog.h"
#include "usermessagebox.h"
#include "Fluent/FluentButton.h"
#include "Fluent/FluentIconButton.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include <QDesktopServices>
#include <QDebug>
#include <QFileInfo>
#include <QUrl>

namespace {

QString previewDialogStyleSheet()
{
    const auto &colors = Fluent::ThemeManager::instance().colors();
    const bool dark = Fluent::ThemeManager::instance().themeMode() == Fluent::ThemeManager::ThemeMode::Dark;
    const QString sliderAccent = colors.accent.name();
    const QString sliderGroove = colors.border.name();
    const QColor panelBackground = dark
                                       ? Fluent::Style::mix(colors.surface, colors.hover, 0.32)
                                       : Fluent::Style::mix(colors.surface, colors.hover, 0.88);
    const QColor panelBorder = dark
                                   ? Fluent::Style::mix(colors.border, colors.hover, 0.25)
                                   : Fluent::Style::mix(colors.border, colors.hover, 0.55);

    return QString(
        "%1"
        "QFrame#contentFrame {"
        "  border: none;"
        "}"
        "QWidget#m_backgroundWidget {"
        "  background: #000000;"
        "  border: 1px solid %2;"
        "  border-radius: 10px;"
        "}"
        "QWidget#processWidget,"
        "QWidget#bottomWidget {"
        "  background: %4;"
        "  border: 1px solid %5;"
        "  border-radius: 10px;"
        "}"
        "QLabel#positionLabel,"
        "QLabel#label_2,"
        "QLabel#durationLabel {"
        "  color: %3;"
        "  font-size: 12px;"
        "}"
        "MousePressableSlider:horizontal {"
        "  background: transparent;"
        "}"
        "MousePressableSlider::groove:horizontal {"
        "  background: %6;"
        "  border-radius: 1px;"
        "  height: 2px;"
        "}"
        "MousePressableSlider::sub-page:horizontal {"
        "  background: %7;"
        "}"
        "MousePressableSlider::add-page:horizontal {"
        "  background: %6;"
        "}"
        "MousePressableSlider::handle:horizontal {"
        "  border-image: url(:/icons/images/slider.svg);"
        "  background: transparent;"
        "  width: 12px;"
        "  height: 12px;"
        "  margin: -5px 0px;"
        "}")
        .arg(Fluent::Theme::dialogStyle(colors))
        .arg(colors.border.name())
        .arg(colors.subText.name())
        .arg(panelBackground.name())
        .arg(panelBorder.name())
        .arg(sliderGroove)
        .arg(sliderAccent);
}

void configurePreviewIconButton(QPushButton *button, const QString &iconName, const QString &toolTip, const QSize &iconSize, const bool checkable = false)
{
    if (button == nullptr) {
        return;
    }

    button->setStyleSheet(QString());
    button->setText(QString());
    button->setToolTip(toolTip);
    button->setIcon(QIcon(QString(":/icons/images/%1.svg").arg(iconName)));
    button->setIconSize(iconSize);
    button->setCheckable(checkable);
    button->setCursor(Qt::PointingHandCursor);

    if (auto *iconButton = qobject_cast<Fluent::FluentIconButton *>(button)) {
        iconButton->setButtonExtent(30);
    }
    if (auto *fluentButton = qobject_cast<Fluent::FluentButton *>(button)) {
        fluentButton->setPrimary(true);
    }
}

void configurePreviewActionButton(QPushButton *button, const QString &text, const QString &iconName, const bool primary)
{
    if (button == nullptr) {
        return;
    }

    button->setStyleSheet(QString());
    button->setText(text);
    button->setIcon(iconName.isEmpty() ? QIcon() : QIcon(QString(":/icons/images/%1.svg").arg(iconName)));
    button->setIconSize(QSize(16, 16));
    button->setCursor(Qt::PointingHandCursor);

    if (auto *fluentButton = qobject_cast<Fluent::FluentButton *>(button)) {
        fluentButton->setPrimary(primary);
    }
}

}


VideoPreviewDialog::VideoPreviewDialog(const QString& file, QWidget* parent) :
    m_previewFile(file)
    , Fluent::FluentDialog(parent)
    , ui(new Ui::VideoPreviewDialog)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(QString(":/icons/images/recording.svg")));
    setWindowTitle(u8"录屏预览");
    setFluentWindowButtons(Fluent::FluentDialog::CloseButton);
    setFluentResizeEnabled(true);
    ui->titleFrame->hide();
    init();
}

VideoPreviewDialog::~VideoPreviewDialog()
{
    QMutexLocker locker(&m_mutex);
    m_closed = true;

    if (m_renderer && m_renderer->m_surface && m_renderer->m_surface->isActive()) {
        m_renderer->m_surface->stop();
    }
    delete m_audioOutPut;
    delete m_player;
    delete ui;
}

void VideoPreviewDialog::closeEvent(QCloseEvent* event)
{
    m_closed = true;
    m_timer.stop();
    if (m_player != nullptr) {
        m_player->stop();
    }
    if (m_audioOutPut != nullptr) {
        m_audioOutPut->stop();
    }
    QDialog::closeEvent(event);
}

void VideoPreviewDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key()==Qt::Key_Left)
    {
        m_player->paused(m_player->position());
        m_player->stepBackward();
        event->accept();
    }
    if (event->key() == Qt::Key_Right)
    {
        m_player->paused(m_player->position());
        m_player->stepForward();
        event->accept();
    }
    QDialog::keyPressEvent(event);
}

void VideoPreviewDialog::init()
{
    setStyleSheet(previewDialogStyleSheet());
    connect(&Fluent::ThemeManager::instance(), &Fluent::ThemeManager::themeChanged, this, [this]() {
        setStyleSheet(previewDialogStyleSheet());
    });

    m_player = new QAVPlayer;
    m_audioOutPut = new QAVAudioOutput;
    m_audioOutPut->setVolume(1.0);
    ui->m_previewWidget->setAspectRatioMode(Qt::KeepAspectRatio);
    ui->m_previewWidget->setFullScreen(false);

    configurePreviewIconButton(ui->preFrameButton, "preFrame", u8"上一帧", QSize(12, 16));
    configurePreviewIconButton(ui->playFrameButton, "play", u8"播放/暂停", QSize(16, 16), true);
    configurePreviewIconButton(ui->nextFrameButton, "nextFrame", u8"下一帧", QSize(12, 16));
    configurePreviewActionButton(ui->cancelButton, u8"关闭", QString(), false);
    configurePreviewActionButton(ui->sureButton, u8"打开文件夹", "folder", true);

    const auto updatePlayButtonIcon = [this](const bool playing) {
        ui->playFrameButton->setIcon(QIcon(QString(":/icons/images/%1.svg").arg(playing ? "pause" : "play")));
    };
    updatePlayButtonIcon(false);


    m_renderer = new VideoRenderer;
    m_mediaObject = new MediaObject(m_renderer);

    ui->m_previewWidget->setMediaObject(m_mediaObject);
    QObject::connect(m_player, &QAVPlayer::audioFrame, this, [&](const QAVAudioFrame &frame)
    {
        if (m_closed)
            return;
        m_audioOutPut->play(frame);
    }, Qt::DirectConnection);
    QObject::connect(m_player, &QAVPlayer::videoFrame, this, [&](const QAVVideoFrame &frame) {
        QMutexLocker locker(&m_mutex);

   // 先检查基本状态
   if (!m_renderer || !m_renderer->m_surface || m_closed || m_onResize) {
       return;
   }

   // 转换视频帧
   QVideoFrame videoFrame = frame.convertTo(AVPixelFormat::AV_PIX_FMT_RGB32);
   if (!videoFrame.isValid()) {
       qWarning() << "Invalid video frame!";
       return;
   }

   // 安全地重新配置surface
   if (!m_renderer->m_surface->isActive()) {
       QVideoSurfaceFormat format(videoFrame.size(), videoFrame.pixelFormat(), videoFrame.handleType());
       if (!m_renderer->m_surface->start(format)) {
           qWarning() << "Failed to start video surface!";
           return;
       }
   } else if (m_renderer->m_surface->surfaceFormat().frameSize() != videoFrame.size()) {
       // 如果尺寸改变，先停止再重新开始
       m_renderer->m_surface->stop();
       QVideoSurfaceFormat format(videoFrame.size(), videoFrame.pixelFormat(), videoFrame.handleType());
       if (!m_renderer->m_surface->start(format)) {
           qWarning() << "Failed to restart video surface with new size!";
           return;
       }
   }

   // 确保surface仍然是活动的再呈现
   if (m_renderer->m_surface->isActive()) {
       if (!m_renderer->m_surface->present(videoFrame)) {
           qWarning() << "Failed to present video frame!";
       }
   }
     }, Qt::DirectConnection);

    QFile file(m_previewFile);
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Could not open file" << m_previewFile;
        UserMessageBox::warning(this, u8"错误", u8"无法打开文件" + m_previewFile);
        close();
        return;
    }
    file.close();

    m_player->setSource(m_previewFile);
    m_player->setSynced(true);
    m_player->setInputOptions(
        QMap<QString, QString>{
        {"fastseek","1"},
        {"accurate_seek","1"},
        {"threads","4"},
        {"skip_loop_filter","ALL"},
        {"skip_frame","NONKEY"}
    });
    connect(m_player, &QAVPlayer::durationChanged, this, &VideoPreviewDialog::onDurationChanged);
    connect(m_player, &QAVPlayer::played, this, &VideoPreviewDialog::onPositionChanged);
    connect(m_player, &QAVPlayer::paused, this, &VideoPreviewDialog::onPositionChanged);
    connect(m_player, &QAVPlayer::stepped, this, &VideoPreviewDialog::onPositionChanged);
    connect(m_player, &QAVPlayer::seeked, this, &VideoPreviewDialog::onPositionChanged);
    connect(m_player, &QAVPlayer::stepped, this, &VideoPreviewDialog::onPositionChanged);
    connect(ui->horizontalSlider, &QSlider::valueChanged, [this](int value)
    {
        if (m_isSelfInvokeSliderChange)
            return;
        m_isSelfInvokeSliderChange = true;
        seek(value);
        m_isSelfInvokeSliderChange = false;
    });
    connect(ui->playFrameButton, &QPushButton::clicked, [&]()
    {
        if (m_player->state() == QAVPlayer::PlayingState)
        {
            m_player->pause();
            ui->playFrameButton->setChecked(false);
            updatePlayButtonIcon(false);
        }
        else //if (m_player->state()==QAVPlayer::PausedState)
        {
            m_player->play();
            ui->playFrameButton->setChecked(true);
            updatePlayButtonIcon(true);
        }
    });
    connect(ui->preFrameButton, &QPushButton::clicked, [&]()
    {
        m_player->paused(m_player->position());
        m_player->stepBackward();
    });
    connect(ui->nextFrameButton, &QPushButton::clicked, [&]()
    {
        m_player->paused(m_player->position());
        m_player->stepForward();
    });

    connect(ui->closeButton, &QPushButton::clicked, this, &VideoPreviewDialog::close);
    connect(ui->cancelButton, &QPushButton::clicked, this, &VideoPreviewDialog::close);
    connect(ui->sureButton, &QPushButton::clicked, this, &VideoPreviewDialog::openPreviewFolder);

    connect(m_player, &QAVPlayer::mediaStatusChanged, [&](auto status) {
        qDebug() << "mediaStatusChanged"<< status << m_player->state();
        if (status == QAVPlayer::LoadedMedia) {
            qDebug() << "Video streams:" << m_player->currentVideoStreams().size();
            for (const auto &s: m_player->currentVideoStreams())
                qDebug() << "[" << s.index() << "]" << s.metadata() << s.framesCount() << "frames,"
            << s.frameRate() << "frame rate";
            qDebug() << "Audio streams:" << m_player->currentAudioStreams().size();
            for (const auto &s: m_player->currentAudioStreams())
                qDebug() << "[" << s.index() << "]" << s.metadata() << s.framesCount() << "frames,"
            << s.frameRate() << "frame rate";
        }
    });

    connect(ui->horizontalSlider,&MousePressableSlider::onMousePress,[&](bool pressed)
    {
        m_ignoreTimerUpdate = pressed;
    });
    connect(m_player,&QAVPlayer::errorOccurred,this,[&](QAVPlayer::Error error,const QString& str)
    {
        qDebug()<<"media play error occurred:"<<error<<str;
    });
    //play media by default
    ui->m_previewWidget->show();

    m_timer.setInterval(100);
    connect(&m_timer,&QTimer::timeout,[&]()
    {
        if (m_ignoreTimerUpdate)
            return;
        auto curPos = m_player->position();
        onPositionChanged(curPos);
    });

    m_player->play();
    ui->playFrameButton->setChecked(true);
    updatePlayButtonIcon(true);
    m_timer.start();
}

void VideoPreviewDialog::onDurationChanged(qint64 duration)
{
    ui->horizontalSlider->setMaximum(duration);
    int s = duration / 1000;
    int m = s/60;
    int h = m/60;
    m = m %60;
    s = s % 60;
    m_durationTime = QString::asprintf("%d:%d:%d",h, m, s);
    ui->durationLabel->setText(m_durationTime);
}

void VideoPreviewDialog::onPositionChanged(qint64 position)
{
    if (ui->horizontalSlider->isSliderDown() || m_isSelfInvokeSliderChange)
        return;
    m_isSelfInvokeSliderChange = true;
    ui->horizontalSlider->setSliderPosition(position);
    int s = position / 1000;
    int m = s/60;
    int h = m/60;
    m = m %60;
    s = s % 60;
    m_positionTime = QString::asprintf("%d:%d:%d",h, m, s);
    ui->positionLabel->setText(m_positionTime);
    m_isSelfInvokeSliderChange = false;
}

void VideoPreviewDialog::openPreviewFolder()
{
    const QFileInfo fileInfo(m_previewFile);
    const QString folderPath = fileInfo.absolutePath();
    if (folderPath.isEmpty() || !QFileInfo::exists(folderPath)) {
        UserMessageBox::warning(this, u8"错误", u8"无法定位录屏文件所在文件夹");
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath))) {
        UserMessageBox::warning(this, u8"错误", u8"无法打开录屏文件所在文件夹");
        return;
    }

    close();

}

void VideoPreviewDialog::seek(qint64 position)
{
    ui->playFrameButton->setChecked(false);
    ui->playFrameButton->setIcon(QIcon(QString(":/icons/images/play.svg")));
    m_player->pause();
    m_player->seek(position);
    onPositionChanged(position);
}



void VideoPreviewDialog::resizeEvent(QResizeEvent* event)
{
    QMutexLocker locker(&m_mutex);
    m_onResize = true;

    // 调用基类的resize事件
    Fluent::FluentDialog::resizeEvent(event);

    // 确保surface安全停止
    if (m_renderer && m_renderer->m_surface && m_renderer->m_surface->isActive()) {
        m_renderer->m_surface->stop();
    }

    m_onResize = false;
}
