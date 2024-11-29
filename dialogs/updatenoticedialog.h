//
// Created by 58226 on 2024/11/22.
//

#ifndef UPDATENOTICEDIALOG_H
#define UPDATENOTICEDIALOG_H

#include <QWidget>

#include "dragmovedialog.h"
#include "onlineservice.h"


QT_BEGIN_NAMESPACE
namespace Ui { class UpdateNoticeDialog; }
QT_END_NAMESPACE

class UpdateNoticeDialog : public DragMoveDialog {
Q_OBJECT

public:
    explicit UpdateNoticeDialog(QSharedPointer<OnlineService> apiInstance ,QJsonDocument updateConetent, QWidget *parent = nullptr);
    ~UpdateNoticeDialog() override;

    void init();

    void closeEvent(QCloseEvent* event) override;

    void beforeClose();

    public slots:

    void onStartDownload();
    void onDownloadFinished(const QString& downloadedPath);

public:
    signals:
    void requestCloseProgram();

private:
    Ui::UpdateNoticeDialog *ui;

    QSharedPointer<OnlineService> m_api{nullptr};
    QJsonDocument m_updateContent;
    bool m_isForceUpdate = false;
    bool m_canCloseNow = true;
    bool m_isDownloadStarted = false;
    QString m_downloadPath{};
    QString m_downloadUrl{};
    HttpResponse* m_requestHandler{nullptr};
};


#endif //UPDATENOTICEDIALOG_H
