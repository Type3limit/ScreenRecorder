//
// Created by 58226 on 2023/11/21.
//

#ifndef RECORDINGWINDOW_H
#define RECORDINGWINDOW_H

#include <QMainWindow>
#include "obswrapper.h"
#include "config.h"
#include "minimizedrecordingwindow.h"
#include "qhotkey.h"
#include "backgroundwindow.h"
#include <QTcpServer>
#include "countdowndialog.h"
#include "onlineservice.h"
#include "testwindow.h"
QT_BEGIN_NAMESPACE

namespace Ui
{
    class RecordingWindow;
}

QT_END_NAMESPACE

class RecordingWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit RecordingWindow(const QString& loginUrl, const QString& token, int port = -1, QWidget* parent = nullptr);
    ~RecordingWindow() override;
    QScreen* findScreen() const;
    int calculateNameSimilarity(const QString& a, const QString& b) const;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void closeEvent(QCloseEvent* event) override;

    void showEvent(QShowEvent* event) override;

    bool isFullScreenMode() const;
    void setupPort(int port);
    void startRecord();
    void pauseRecord();
    void stopRecord();
    void saveConfig();

    void checkForUpdate();
public:


public slots:
    void rebuildBackgroundWindow();
    void invokeUploadWindow(const QStringList& uploadedFiles);
    static bool checkUpdate(const QString& serverVersiona);
    void invokeUpdateWindow(QJsonDocument doc);

    void invokeUploadNoticeWindow(const QString& file);

    void invokePreviewWindow(const QString& file);

    void invokeLoginWindow();
protected:
    void init(int defaultPort = -1);

private:
    QPoint m_pos = {0, 0};
    bool m_leftButtonPressed = false;
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

private:
    QString m_url{};
    QString m_token{};
    QString m_curUserAccount{};
    QSharedPointer<OnlineService> m_api{nullptr};
    QString m_currentRecordingFile{};
    HttpResponse* m_requestHandler{nullptr};
    QString m_versionCode{""};
};


#endif //RECORDINGWINDOW_H
