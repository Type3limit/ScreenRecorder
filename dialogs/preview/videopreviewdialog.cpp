//
// Created by 58226 on 2024/11/26.
//

// You may need to build the project (run Qt uic code generator) to get "ui_VideoPreviewDialog.h" resolved

#include "videopreviewdialog.h"

#include "cloumnchoosenwidget.h"
#include "ui_VideoPreviewDialog.h"
#include "usermessagebox.h"



VideoPreviewDialog::VideoPreviewDialog(const QString& file, QSharedPointer<OnlineService> apiInstance, QWidget* parent) :
    m_previewFile(file)
    , m_api(apiInstance)
    , DragMoveDialog(parent)
    , ui(new Ui::VideoPreviewDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowIcon(QIcon(QString(":/icons/images/recording.svg")));
    init();
}

VideoPreviewDialog::~VideoPreviewDialog()
{
    delete m_audioOutPut;
    delete m_player;
    delete ui;
}

void VideoPreviewDialog::closeEvent(QCloseEvent* event)
{
    m_closed = true;
    m_timer.stop();
    m_player->stop();
    m_audioOutPut->stop();
    DragMoveDialog::closeEvent(event);
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
    DragMoveDialog::keyPressEvent(event);
}

void VideoPreviewDialog::init()
{
    m_player = new QAVPlayer;
    m_audioOutPut = new QAVAudioOutput;
    m_audioOutPut->setVolume(1.0);
    ui->m_previewWidget->setAspectRatioMode(Qt::KeepAspectRatio);
    ui->m_previewWidget->setFullScreen(false);


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
         if (m_renderer->m_surface == nullptr||m_closed)
             return;

         QVideoFrame videoFrame = frame.convertTo(AVPixelFormat::AV_PIX_FMT_RGB32);
         if (!m_renderer->m_surface->isActive() ||
             m_renderer->m_surface->surfaceFormat().frameSize() != videoFrame.size())
             {
             QVideoSurfaceFormat f(videoFrame.size(), videoFrame.pixelFormat(), videoFrame.handleType());
             m_renderer->m_surface->start(f);
         }
         if (m_renderer->m_surface->isActive())
             m_renderer->m_surface->present(videoFrame);
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
    m_player->setInputOptions(QMap<QString, QString>{
        {"fastseek","1"},
        {"accurate_seek","0"},
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
        }
        else //if (m_player->state()==QAVPlayer::PausedState)
        {
            m_player->play();
            ui->playFrameButton->setChecked(true);
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
    connect(ui->sureButton,&QPushButton::clicked,this,&VideoPreviewDialog::invokeUploadWindow);

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

void VideoPreviewDialog::invokeUploadWindow()
{
    if(m_api==nullptr)
    {
        qWarning()<<"api instance is null,ignore upload option";
        return;
    }
    if (m_previewFile.isEmpty()||!QFile::exists(m_previewFile))
    {
        qWarning()<<"recorded file is empty or not exist,ignore upload option";
        return;
    }
    CloumnChoosenDialog * dialog = new CloumnChoosenDialog(m_api,{m_previewFile},nullptr);
    connect(dialog,&CloumnChoosenDialog::hasUploadOptions,this,[&]()
    {
        emit hasUploadOption();
    });
    dialog->exec();
    dialog->deleteLater();
    close();
}

void VideoPreviewDialog::seek(qint64 position)
{
    ui->playFrameButton->setChecked(false);
    m_player->pause();
    m_player->seek(position);
    onPositionChanged(position);
}
