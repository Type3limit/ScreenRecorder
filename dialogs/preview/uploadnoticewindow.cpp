//
// Created by 58226 on 2024/11/26.
//

// You may need to build the project (run Qt uic code generator) to get "ui_UploadNoticeWindow.h" resolved

#include "uploadnoticewindow.h"
#include "ui_UploadNoticeWindow.h"
#include <QDebug>

#include "cloumnchoosenwidget.h"
#include "videopreviewdialog.h"

UploadNoticeWindow::UploadNoticeWindow(const QString& recordedFile,QSharedPointer<OnlineService> apiInstance,QWidget *parent)
:m_recordFile(recordedFile),m_api(apiInstance),DragMoveDialog(parent),ui(new Ui::UploadNoticeWindow)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowIcon(QIcon(QString(":/icons/images/recording.svg")));
    init();
}

UploadNoticeWindow::~UploadNoticeWindow() {
    delete ui;
}

void UploadNoticeWindow::init()
{
    connect(ui->closeButton,&QPushButton::clicked,this,&UploadNoticeWindow::close);
    connect(ui->cancelButton,&QPushButton::clicked,this,&UploadNoticeWindow::close);
    connect(ui->sureButton,&QPushButton::clicked,this,&UploadNoticeWindow::invokeUploadWindow);
    connect(ui->previewButton,&QPushButton::clicked,this,&UploadNoticeWindow::invokePreviewWindow);


}

void UploadNoticeWindow::invokeUploadWindow()
{
    if(!m_api->loginHelper()->isLogin())
    {
        qWarning()<<"not login , invoke login window";
        emit requestLogin();
        if (!m_api->loginHelper()->isLogin())
            return;
    }
    if (m_recordFile.isEmpty()||!QFile::exists(m_recordFile))
    {
        qWarning()<<"recorded file"<<m_recordFile<<" is empty or not exist,ignore upload option";
        return;
    }
    CloumnChoosenDialog * dialog = new CloumnChoosenDialog(m_api,{m_recordFile},nullptr);
    connect(dialog,&CloumnChoosenDialog::hasUploadOptions,this,&UploadNoticeWindow::close);
    dialog->exec();
    dialog->deleteLater();
}

void UploadNoticeWindow::invokePreviewWindow()
{
    if (m_recordFile.isEmpty()||!QFile::exists(m_recordFile))
    {
        qWarning()<<"recorded file is empty or not exist,ignore preview option";
        return;
    }
    VideoPreviewDialog * dialog = new VideoPreviewDialog(m_recordFile,m_api,nullptr);
    connect(dialog,&VideoPreviewDialog::hasUploadOption,this,&UploadNoticeWindow::close);
    connect(dialog,&VideoPreviewDialog::requestLogin,this,&UploadNoticeWindow::requestLogin);
    dialog->exec();
    dialog->deleteLater();
}
