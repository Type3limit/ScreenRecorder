//
// Created by 58226 on 2023/11/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_minimizedrecordingwindow.h" resolved

#include "minimizedrecordingwindow.h"
#include "ui_minimizedrecordingwindow.h"
#include "Fluent/FluentTheme.h"
#include "obswrapper.h"
#include <QMouseEvent>

namespace {

QString minimizedWindowStyleSheet()
{
    const auto &colors = Fluent::ThemeManager::instance().colors();

    return QString(
        "QFrame#contentFrame {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 12px;"
        "}"
        "QFrame#line,"
        "#line_1,"
        "#line_2,"
        "#line_3,"
        "#line_4 {"
        "  background-color: %3;"
        "}")
        .arg(colors.surface.name())
        .arg(colors.border.name())
        .arg(colors.hover.name());
}

QString compactButtonStyle(const QString &iconName = QString())
{
    const auto &colors = Fluent::ThemeManager::instance().colors();
    QString iconRule;
    if (!iconName.isEmpty())
    {
        iconRule = QString("qproperty-icon:url(:/icons/images/%1.svg); qproperty-iconSize:20px;").arg(iconName);
    }

    return QString(
        "QPushButton {"
        "  %1"
        "  color: %2;"
        "  background-color: transparent;"
        "  border: 1px solid transparent;"
        "  border-radius: 6px;"
        "  padding: 1px 0px 1px 2px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %3;"
        "  border-color: %4;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %5;"
        "  border-color: %4;"
        "}"
        "QPushButton:disabled {"
        "  color: %6;"
        "  border-color: transparent;"
        "}")
        .arg(iconRule)
        .arg(colors.text.name())
        .arg(colors.hover.name())
        .arg(colors.accent.name())
        .arg(colors.pressed.name())
        .arg(colors.disabledText.name());
}

void applyCompactWindowState(Ui::MinimizedRecordingWindow* ui, const int status)
{
    if (ui == nullptr)
    {
        return;
    }

    if (status == RecordingStatus::Recording)
    {
        ui->recordingButton->setStyleSheet(compactButtonStyle("stop_recoding_minimize"));
        ui->pauseButton->setStyleSheet(compactButtonStyle("pause"));
        ui->pauseButton->setEnabled(true);
        return;
    }

    if (status == RecordingStatus::Paused)
    {
        ui->recordingButton->setStyleSheet(compactButtonStyle("stop_recoding_minimize"));
        ui->pauseButton->setStyleSheet(compactButtonStyle("play"));
        ui->pauseButton->setEnabled(true);
        return;
    }

    ui->recordingButton->setStyleSheet(compactButtonStyle("start"));
    ui->pauseButton->setStyleSheet(compactButtonStyle("play"));
    ui->pauseButton->setEnabled(false);
}

}

MinimizedRecordingWindow::MinimizedRecordingWindow(const QSharedPointer<ObsWrapper>& obs_wrapper, QWidget* parent)
    : DragMoveDialog(parent)
    , ui(new Ui::MinimizedRecordingWindow)
    , m_obs(obs_wrapper)
{
    ui->setupUi(this);
    setStyleSheet(minimizedWindowStyleSheet());
    // hide title bar
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowIcon(QIcon(QString(":/icons/images/recording.svg")));
    setWindowTitle(u8"录屏");
    applyCompactWindowState(ui, m_recordStatus);
    ui->recoverButton->setStyleSheet(compactButtonStyle());
    connect(m_obs.get(), &ObsWrapper::recordStatusChanged, this, [&](const int status)
    {
        m_recordStatus = status;
        applyCompactWindowState(ui, m_recordStatus);
    });
    connect(&Fluent::ThemeManager::instance(), &Fluent::ThemeManager::themeChanged, this, [this]() {
        setStyleSheet(minimizedWindowStyleSheet());
        applyCompactWindowState(ui, m_recordStatus);
        ui->recoverButton->setStyleSheet(compactButtonStyle());
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
