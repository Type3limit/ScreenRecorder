//
// Created by 58226 on 2023/11/21.
//

#ifndef RECORDINGWINDOW_H
#define RECORDINGWINDOW_H

#include "Fluent/FluentMainWindow.h"
#include "obswrapper.h"
#include "config.h"
#include "minimizedrecordingwindow.h"
#include "qhotkey.h"
#include "backgroundwindow.h"
#include <QTcpServer>
#include "countdowndialog.h"
#include "testwindow.h"
QT_BEGIN_NAMESPACE

namespace Ui
{
    class RecordingWindow;
}

QT_END_NAMESPACE

class RecordingWindow : public Fluent::FluentMainWindow
{
    Q_OBJECT

public:
    explicit RecordingWindow(const QString& loginUrl, const QString& token, int port = -1, QWidget* parent = nullptr);
    ~RecordingWindow() override;
    QScreen* findScreen() const;
    int calculateNameSimilarity(const QString& a, const QString& b) const;
    void resetVideo();
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

    bool isFullScreenMode() const;
    void setupPort(int port);
    void startRecord();
    void pauseRecord();
    void stopRecord();
    void saveConfig();

public:


public slots:
    void rebuildBackgroundWindow();
    static bool checkUpdate(const QString& serverVersiona);

    void invokePreviewWindow(const QString& file);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void init(int defaultPort = -1);
    void initSignalProxy();
    void initMiniWindow();
    void initBackgroundWindow();
    void initConfig();
    void initRecordingStatus();
    void initDebugWindow();
    void initFunctionalControls();
    void initHotKeys();
    void setupFluentWindowChrome();

private:
    bool m_isRecordingStarted = false;
    bool m_isPauseNow = false;

private:
    Ui::RecordingWindow* ui;
    QSharedPointer<ObsWrapper> m_obs;
    MinimizedRecordingWindow* m_miniWindow = nullptr;
    BackgroundWindow* m_backgroundWindow = nullptr;
    CountDownDialog* m_countDownDialog = nullptr;
    volatile bool m_isMicphoneEnable = true;
    volatile bool m_isPlayerEnable = true;
    volatile bool m_alreadyInited = false;
    testwindow* m_test = nullptr;
    Config m_config;
    QHotkey* m_recordHotKey;
    QHotkey* m_pauseHotKey;
    QHotkey* m_showCaptureKey;
    QTcpServer* m_server = nullptr;
    QTimer m_timer;
    qint64 m_seconds = 0;
    QPoint m_startPos = {0, 0};
    QPoint m_endPos = {0, 0};

    int m_socketPort{};
private:
    QString m_url{};
    QString m_token{};
    QString m_curUserAccount{};
    QString m_currentRecordingFile{};
    QString m_versionCode{""};

    volatile bool m_hasFirstInit = false;
    qint64 m_stopTimeInterval = 0;
};


#endif //RECORDINGWINDOW_H
