//
// Created by 58226 on 2024/11/26.
//

#ifndef UPLOADNOTICEWINDOW_H
#define UPLOADNOTICEWINDOW_H

#include "dragmovedialog.h"
#include "onlineservice.h"


QT_BEGIN_NAMESPACE
namespace Ui { class UploadNoticeWindow; }
QT_END_NAMESPACE

class UploadNoticeWindow : public DragMoveDialog {
Q_OBJECT

public:
    explicit UploadNoticeWindow(const QString& recodedFile,QSharedPointer<OnlineService> apiInstance,QWidget *parent = nullptr);
    ~UploadNoticeWindow() override;


    void init();

    void invokeUploadWindow();

    void invokePreviewWindow();

private:
    Ui::UploadNoticeWindow *ui;
    QString m_recordFile;
    QSharedPointer<OnlineService> m_api{nullptr};
};


#endif //UPLOADNOTICEWINDOW_H
