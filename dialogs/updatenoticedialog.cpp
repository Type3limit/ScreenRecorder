//
// Created by 58226 on 2024/11/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_UpdateNoticeDialog.h" resolved


#include <QThread>
#ifdef _WIN32
#include <windows.h>
#include <QString>
#include <QFile>
#include <QDebug>
void UACrunProcess(QString Path) {
    // QFile file(Path);
    // if (!file.exists()) {
    //     qWarning() << "create dump bat path is not exists";
    //     return;
    // }
    // std::wstring wstr = Path.toStdWString();
    // const wchar_t* filePtr = wstr.c_str();
    //
    // SHELLEXECUTEINFOW sei = { sizeof(SHELLEXECUTEINFOW) };
    // sei.lpVerb = (L"open");
    // sei.lpFile = filePtr;
    // sei.nShow = SW_SHOWNORMAL;//without this,the windows will be hiden
    // //    sei.nShow = SW_HIDE;
    // if (!ShellExecuteExW(&sei)) {
    //     DWORD dwStatus = GetLastError();
    //     if (dwStatus == ERROR_CANCELLED) {
    //         qWarning() << "execute canceld";
    //     } else if (dwStatus == ERROR_FILE_NOT_FOUND) {
    //         qWarning() << "execute not found execute file";
    //     }
    //     else
    //     {
    //         qWarning() << "execute failed with error code:" << dwStatus;
    //     }
    // }
    QFile file(Path);
    if (!file.exists()) {
        qWarning() << "create dump bat path is not exists";
        return;
    }

    std::wstring wstr = Path.toStdWString();
    const wchar_t* filePtr = wstr.c_str();

    // Set up the STARTUPINFO and PROCESS_INFORMATION structures
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi;

    // Set the dwCreationFlags to CREATE_NEW_CONSOLE or DETACHED_PROCESS
    DWORD dwCreationFlags = DETACHED_PROCESS;

    if (!CreateProcessW(filePtr, nullptr, nullptr, nullptr, FALSE, dwCreationFlags, nullptr, nullptr, &si, &pi)) {
        DWORD dwStatus = GetLastError();
        qWarning() << "CreateProcess failed with error code:" << dwStatus;
    } else {
        // Close process and thread handles as they are no longer needed
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}
#endif

#include "updatenoticedialog.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QProcess>

#include "config.h"
#include "ui_updatenoticedialog.h"
#include "usermessagebox.h"



UpdateNoticeDialog::UpdateNoticeDialog(QSharedPointer<OnlineService> apiInstance,QJsonDocument doc, QWidget* parent) :
    DragMoveDialog(parent)
    , ui(new Ui::UpdateNoticeDialog)
    , m_api(apiInstance)
    , m_updateContent(doc)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowIcon(QIcon(QString(":/icons/images/recording.svg")));
    init();
}

UpdateNoticeDialog::~UpdateNoticeDialog()
{
    delete ui;
}

void UpdateNoticeDialog::init()
{
    m_isForceUpdate = m_updateContent[FORCE_UPDATE_KEY].toBool();
    if (m_isForceUpdate)
    {
        ui->cancelButton->setVisible(false);
        m_canCloseNow = false;
    }

    ui->progressBar->setVisible(false);

    connect(ui->cancelButton, &QPushButton::clicked, this, &UpdateNoticeDialog::beforeClose);

    connect(ui->closeButton, &QPushButton::clicked, this, &UpdateNoticeDialog::beforeClose);

    connect(ui->sureButton, &QPushButton::clicked, this, &UpdateNoticeDialog::onStartDownload);

    m_downloadPath = UPDATE_FILE_LOCAL_NAME;

    m_downloadUrl = m_updateContent[DOWNLOAD_FILE_KEY].toString();

    qDebug()<<u8"更新程序下载地址:【"<<m_downloadUrl << u8"】目标下载地址:【"<<UPDATE_FILE_LOCAL_NAME<<u8"】";

    ui->versionLabel->setText(u8"当前程序版本:"+CURRENT_PROGRAM_VERSION
        +u8", 最新程序版本:"+m_updateContent[VERSION_KEY].toString());

    ui->contentNoticeLabel->setText(m_updateContent[UPDATE_DESCRIBE_KEY].toString());

    ui->progressBar->setVisible(false);

    m_requestHandler = nullptr;
}

void UpdateNoticeDialog::closeEvent(QCloseEvent* event)
{
    if (!m_canCloseNow)
    {
        event->ignore();
    }
    DragMoveDialog::closeEvent(event);
}

void UpdateNoticeDialog::beforeClose()
{
    if (m_isDownloadStarted)
    {
        if (UserMessageBox::ButtonType::Ok !=
            UserMessageBox::question(this, u8"取消", u8"安装包正在下载中，确认取消？"))
        {
            return;
        }
    }

    if (m_requestHandler != nullptr&&m_requestHandler->reply()!=nullptr)
    {
        m_requestHandler->reply()->abort();
    }
    close();
}

void UpdateNoticeDialog::onStartDownload()
{

    if (m_isDownloadStarted)
        return;
    if (m_downloadUrl.isEmpty())
    {
        qDebug()<<" download url is empty!";
        m_canCloseNow = true;
        UserMessageBox::warning(this, u8"错误", u8"下载地址为空，请检查服务器配置");
        close();
        return;
    }




    bool shouldContinueLoop = false;
    do
    {
        QFile file(m_downloadPath);

        auto dirPath = ExtensionMethods::QStringExtension::getFileInfo(m_downloadPath,ExtensionMethods::QStringExtension::ParentDir);
        QDir dir(dirPath);
        if (!dir.exists())
        {
            dir.mkpath(dirPath);
        }
        if (!(file.permissions() & QFile::WriteOwner)) {
            qDebug() << "No write permission.";
        }
        if (file.isOpen()) {
            file.close();
        }
        shouldContinueLoop = !file.open(QIODevice::WriteOnly);
        if (shouldContinueLoop)
        {
            qDebug()<<"can not create download file,please check the permission of install folder:"<<file.errorString();
            UserMessageBox::warning(nullptr,u8"失败",u8"无法创建下载文件，请选择可用的保存位置");
            auto curStr = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                           QApplication::applicationDirPath().isEmpty()
                                                               ? "."
                                                               : QApplication::applicationDirPath());
            if (curStr.isEmpty())
            {
                return ;
            }
            m_downloadPath = curStr + UPDATE_FILE_NAME;
            qDebug()<<"download path changed to :"<<m_downloadPath;
        }
        file.close();
    }
    while (shouldContinueLoop);


    m_isDownloadStarted = true;
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);
    ui->sureButton->setVisible(false);
    ui->cancelButton->setVisible(false);
    bool downloadSuccess = false;
    BLOCK_DEBUG(m_requestHandler ,
        m_api->fileHelper()->downloadFile(m_downloadPath, m_downloadUrl,
            [&](int cur, int total)
            {
                auto percent = (cur / static_cast<double>(total))*100;
                ui->sureButton->setEnabled(false);
                ui->versionLabel->setText(
                    QString(u8"正在下载安装包...已下载%1 %")
                    .arg(percent));

                ui->progressBar->setValue(percent);


            }, [&](const QString& path)
            {
                m_isDownloadStarted = false;
                ui->progressBar->setVisible(false);
                ui->versionLabel->setText(u8"下载完成,正在启动安装包");
                downloadSuccess = true;
            }, [&](const QString& error)
            {
                m_isDownloadStarted = false;
                ui->versionLabel->setText(QString(u8"下载失败！%1").arg(error));
                ui->progressBar->setVisible(false);
                ui->sureButton->setVisible(true);
                ui->cancelButton->setVisible(true);
                qDebug() << "download failed:" << error;
            }));

    if (downloadSuccess)
        onDownloadFinished(m_downloadPath);
}

void UpdateNoticeDialog::onDownloadFinished(const QString& downloadedPath)
{
    QThread::msleep(1500);
#ifdef _WIN32
    UACrunProcess(downloadedPath);
#endif
    // qDebug()<<"start invoke "<<downloadedPath;
    // QProcess process;
    // connect(&process, &QProcess::errorOccurred, [](QProcess::ProcessError error) {
    // qDebug() << "Error occurred: " << error;
    // });
    // connect(&process, &QProcess::started, [](){
    //     qDebug() << "Process started successfully.";
    // });
    // QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // process.setProcessEnvironment(env);
    // if (!QProcess::startDetached(downloadedPath)) {
    //     qDebug() << "Failed to start program : " << QProcess::error();
    // }
    //QProcess::startDetached(downloadedPath);
    close();
    emit requestCloseProgram();
}
