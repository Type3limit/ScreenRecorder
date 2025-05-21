//
// Created by 58226 on 2023/11/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_minimizedrecordingwindow.h" resolved

#include "minimizedrecordingwindow.h"
#include "ui_minimizedrecordingwindow.h"
#include "obswrapper.h"
#include <QMouseEvent>

MinimizedRecordingWindow::MinimizedRecordingWindow(const QSharedPointer<ObsWrapper>& obs_wrapper, QWidget* parent)
    : DragMoveDialog(parent)
    , ui(new Ui::MinimizedRecordingWindow)
    , m_obs(obs_wrapper)
{
    ui->setupUi(this);
    // hide title bar
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowIcon(QIcon(QString(":/icons/images/recording.svg")));
    setWindowTitle(u8"录屏");
    connect(m_obs.get(), &ObsWrapper::recordStatusChanged, this, [&](const int status)
    {
        auto startButtonStyle = QString("QPushButton {"
            "    qproperty-icon:url(:/icons/images/{iconName}.svg);"
            "    qproperty-iconSize:20px;"
            "    selection-background-color: #40404A;"
            "    selection-color: #3B3B41;"
            "    color: #99A1B0;"
            "    height:24px;"
            "    background-color: transparent;"
            "    border-style: solid;"
            "    border: 0px solid #0F0F0F;"
            "    border-radius: 4;"
            "    padding: 1px 0px 1px 2px;"
            "    font-size: 12px;"
            "    font-family:Microsoft YaHei UI;"
            "    padding-left:0px;"
            "}"
            ""
            "QPushButton:hover {"
            "    background-color: #2F2F34;"
            "    border-radius: 4px;"
            "    border: 1px solid #5967f2;"
            "}"
        );
        if (status == RecordingStatus::Recording)
        {
            auto recStyle = QString(startButtonStyle).replace("{iconName}", "stop_recoding_minimize");
            ui->recordingButton->setStyleSheet(recStyle);
            auto pauseStyle = QString(startButtonStyle).replace("{iconName}", "pause");
            ui->pauseButton->setStyleSheet(pauseStyle);
            ui->pauseButton->setEnabled(true);
        }
        else if (status == RecordingStatus::Paused)
        {
            auto recStyle = QString(startButtonStyle).replace("{iconName}", "stop_recoding_minimize");
            ui->recordingButton->setStyleSheet(recStyle);

            auto pauseStyle = QString(startButtonStyle).replace("{iconName}", "play");
            ui->pauseButton->setStyleSheet(pauseStyle);
            ui->pauseButton->setEnabled(true);
        }
        else
        {
            auto recStyle = QString(startButtonStyle).replace("{iconName}", "start");
            ui->recordingButton->setStyleSheet(recStyle);
            auto pauseStyle = QString(startButtonStyle).replace("{iconName}", "play");
            ui->pauseButton->setStyleSheet(pauseStyle);
            ui->pauseButton->setEnabled(false);
        }
    });
    connect(m_obs.get(), &ObsWrapper::micVolumeDataChange, this, [&]
        (const float* magnitude, const float* peak, const float* inputPeak)
            {
                onAudioFrame(magnitude, peak, inputPeak, true);
            }, Qt::QueuedConnection);
    connect(m_obs.get(), &ObsWrapper::playerVolumeDataChange, this, [&]
        (const float* magnitude, const float* peak, const float* inputPeak)
            {
                onAudioFrame(magnitude, peak, inputPeak, false);
            }, Qt::QueuedConnection);
    int audioChannel = m_obs->audioChannel();
    ui->CurrentVolume->setChannelCount(m_obs->micChannelCount());
    ui->CurrentVolume->setAudioChannel(audioChannel);
    ui->CurrentVolume->setInverting(true);

    connect(ui->recoverButton, &QPushButton::clicked, this, &MinimizedRecordingWindow::onRecover);
    connect(ui->recordingButton, &QPushButton::clicked, this, &MinimizedRecordingWindow::onRecordAct);
    connect(ui->pauseButton, &QPushButton::clicked, this, &MinimizedRecordingWindow::onPauseAct);
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
