//
// Created by 58226 on 2024/11/26.
//

#ifndef VIDEOPREVIEWDIALOG_H
#define VIDEOPREVIEWDIALOG_H

#include "dragmovedialog.h"
#include "QtAVPlayer/qavplayer.h"
#include "QtAVPlayer/qavvideoframe.h"
#include "QtAVPlayer/qavaudiooutput.h"
#include "onlineservice.h"
#include "videorenderer.h"


QT_BEGIN_NAMESPACE
namespace Ui { class VideoPreviewDialog; }
QT_END_NAMESPACE

class VideoPreviewDialog : public DragMoveDialog {
Q_OBJECT

public:
    explicit VideoPreviewDialog(const QString& file,QSharedPointer<OnlineService> apiInstance,QWidget *parent = nullptr);
    ~VideoPreviewDialog() override;

    void closeEvent(QCloseEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;

    void init();
    void onDurationChanged(qint64 duration);
    void onPositionChanged(qint64 position);

    void invokeUploadWindow();

    void seek(qint64 position);
public: signals:
    void hasUploadOption();
    void requestLogin();
private:
    Ui::VideoPreviewDialog *ui;
    QAVPlayer* m_player{nullptr};
    QAVAudioOutput* m_audioOutPut{nullptr};

    QString m_previewFile{};

    QString m_durationTime;
    QString m_positionTime;

    bool m_isSelfInvokeSliderChange = false;

    bool m_closed = false;

    bool m_ignoreTimerUpdate = false;

    QSharedPointer<OnlineService> m_api;

    VideoRenderer* m_renderer{nullptr};
    MediaObject* m_mediaObject{nullptr};

    QTimer m_timer;
};


#endif //VIDEOPREVIEWDIALOG_H
