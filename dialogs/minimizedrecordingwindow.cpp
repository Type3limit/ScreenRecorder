//
// Created by 58226 on 2023/11/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_minimizedrecordingwindow.h" resolved

#include "minimizedrecordingwindow.h"
#include "ui_minimizedrecordingwindow.h"
#include "obswrapper.h"
#include <QMouseEvent>

MinimizedRecordingWindow::MinimizedRecordingWindow(const QSharedPointer<ObsWrapper>& obs_wrapper, QWidget* parent)
    : QDialog(parent), ui(new Ui::MinimizedRecordingWindow), m_obs(obs_wrapper)
{
    ui->setupUi(this);
    // hide title bar
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    connect(m_obs.get(), &ObsWrapper::recordStatusChanged, this, [&](const int status)
    {
        auto style = QString("QPushButton{"
            "qproperty-icon:url(:/icons/images/{iconName}.svg);"
            "qproperty-iconSize:20px;"
            "background-color:#2F2F34;"
            "border-radius:4px;"
            "}");
        if (status == RecordingStatus::Recording)
        {
            ui->recordingButton->setStyleSheet(style.replace("{iconName}", "stop_recoding_minimize"));
        }
        else if (status == RecordingStatus::Paused)
        {
            ui->recordingButton->setStyleSheet(style.replace("{iconName}", "stop_recoding_minimize"));;
        }
        else
        {
            ui->recordingButton->setStyleSheet(style.replace("{iconName}", "start"));
        }
    });
    connect(m_obs.get(), &ObsWrapper::MicVolumeDataChange, this, [&]
        (const float* magnitude, const float* peak, const float* inputPeak)
            {
                onAudioFrame(magnitude, peak, inputPeak, true);
            });
    connect(m_obs.get(), &ObsWrapper::PlayerVolumeDataChange, this, [&]
        (const float* magnitude, const float* peak, const float* inputPeak)
            {
                onAudioFrame(magnitude, peak, inputPeak, false);
            });
    int audioChannel = m_obs->audioChannel();
    ui->CurrentVolume->setChannelCount(m_obs->micChannelCount());
    ui->CurrentVolume->setAudioChannel(audioChannel);
    connect(ui->recoverButton,&QPushButton::clicked,this,&MinimizedRecordingWindow::onRecover);
    connect(ui->recordingButton,&QPushButton::click,this,[&]()
    {
       emit onRecordAct();
    });
}

MinimizedRecordingWindow::~MinimizedRecordingWindow()
{
    delete ui;
}

void MinimizedRecordingWindow::closeEvent(QCloseEvent* event)
{
    emit closed();
    QDialog::closeEvent(event);
}

void MinimizedRecordingWindow::onAudioFrame(const float currentMagnitude[8], const float currentPeak[8],
                                            const float currentInputPeak[8]
                                            , const bool isMicphone)
{
    if (isMicphone)
    {
        for (int i = 0; i < 8; i++)
        {
            m_currentMagnitude[i] = currentMagnitude[i];
            m_currentPeak[i] = currentPeak[i];
            m_currentInputPeak[i] = currentInputPeak[i];
        }
    }
    else
    {
        for (int i = 0; i < 8; i++)
        {
            m_currentMagnitude[i] = std::max(m_currentMagnitude[i], currentMagnitude[i]);
            m_currentPeak[i] = std::max(m_currentPeak[i], currentPeak[i]);
            m_currentInputPeak[i] = std::max(m_currentPeak[i], currentInputPeak[i]);
        }
        ui->CurrentVolume->setLevels(m_currentMagnitude, m_currentPeak, m_currentInputPeak);
    }
}
void MinimizedRecordingWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        return;
    }
    m_pos = e->globalPos() - mapToGlobal({0,0}) ;
    m_leftButtonPressed = true;
}

void MinimizedRecordingWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_leftButtonPressed) {
        return;
    }
    QPoint posOffset = e->globalPos() - m_pos;
    move( posOffset);
}

void MinimizedRecordingWindow::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        return;
    }
    m_leftButtonPressed = false;
}
