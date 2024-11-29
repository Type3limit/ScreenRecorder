//
// Created by 58226 on 2024/11/14.
//

// You may need to build the project (run Qt uic code generator) to get "ui_CloumnChoosenWidget.h" resolved

#include "cloumnchoosenwidget.h"

#include "extensionmethods.h"
#include "ui_cloumnchoosenwidget.h"
#include "uploadoperator.h"
#include "usermessagebox.h"


CloumnChoosenDialog::CloumnChoosenDialog(QSharedPointer<OnlineService> apiInstance,const QStringList& uploadFiles, QWidget* parent) :
m_api(apiInstance)
    ,m_uploadedFiles(uploadFiles)
    , DragMoveDialog(parent)
    , ui(new Ui::CloumnChoosenWidget)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowIcon(QIcon(QString(":/icons/images/recording.svg")));
    init();
}

CloumnChoosenDialog::~CloumnChoosenDialog()
{
    delete ui;
}

void CloumnChoosenDialog::init()
{

    bool hasLoginFailed = true;
    m_requestHandler = nullptr;


    if(m_requestClose)
    {
        qDebug()<<u8"窗口关闭，终止后续步骤";
        return;
    }

    m_requestHandler = nullptr;

    qDebug()<<u8"正在初始化上传页面";
    ui->m_uploader->setApiInstance(m_api);
    ui->m_uploader->init();
    ui->m_uploader->setColumnType(NetworkColumnAndFoklerWidget::ColumnType::ColumnType_UploadMaterial);
    ui->m_uploader->hideFilterWidget(true);

    auto fileName = ExtensionMethods::QStringExtension::getFileInfo(m_uploadedFiles.first(),
        ExtensionMethods::QStringExtension::QStringExtensionPathOptionMode::FileNameWithoutExtension);

    qDebug()<<"curUploadFile:"<<m_uploadedFiles.first()<<"fileName:"<<fileName;

    ui->m_folderNameEditor->setText(fileName);

    connect(ui->closeButton,&QPushButton::clicked,this,&CloumnChoosenDialog::close);
    connect(ui->cancelButton,&QPushButton::clicked,this,&CloumnChoosenDialog::close);
    connect(ui->sureButton,&QPushButton::clicked,this,&CloumnChoosenDialog::onUploadStart);
    connect(ui->m_uploader->apiOperator(),&UploadOperator::presonalPath,this,&CloumnChoosenDialog::onGetUploadUrl);
    connect(ui->m_uploader->apiOperator(),&UploadOperator::publicPath,this,&CloumnChoosenDialog::onGetUploadUrl);
    connect(ui->m_uploader->apiOperator(),&UploadOperator::updateMediaProcess,this,[&](int curIndex,int totalCount,
        int curProcess,int totalProcess)
    {
        int percent = static_cast<int>((static_cast<double>(curProcess) / totalProcess) * 100.0);;
        qDebug()<<u8"文件上传"<<curIndex<<"/"<<totalCount<<":"<<percent;
        ui->m_disableView->setNoticeMessage(QString(u8"正在上传%1/%2,%3")
            .arg(curIndex)
            .arg(totalCount)
            .arg(percent));
    });
    connect(ui->m_uploader->apiOperator(),&UploadOperator::mediaFinish,this,[&](int code,int index, int size)
    {
        if(code<0)
        {
            qDebug()<<u8"文件上传失败"<<index<<"/"<<size <<":"<<code;
            UserMessageBox::warning(nullptr, u8"失败", QString(u8"文件%1上传失败").arg(index));
            return;
        }
        ui->m_disableView->setNoticeMessage(QString(u8"上传完成%1/%2").arg(index).arg(size));
        if((index) == (size-1) && code == 0 )
        {
            qDebug()<<u8"文件上传完成,上传窗口关闭";
            UserMessageBox::information(nullptr,u8"成功",u8"文件上传完成");
            emit hasUploadOptions();
            close();
        }
    });

    connect(ui->m_createFolderButton,&QPushButton::clicked,this,&CloumnChoosenDialog::onCreateFolder);
    connect(ui->m_uploader->apiOperator(),&UploadOperator::folderCreated,this,&CloumnChoosenDialog::onFolderCreated);

    qDebug()<<u8"初始化上传页面完成";
}

void CloumnChoosenDialog::onUploadStart()
{

    QString columnId = ui->m_uploader->columnId();
    QString folderId =m_targetFolderId.isEmpty()? ui->m_uploader->folderId():m_targetFolderId;

    if(columnId.isEmpty())
    {
        UserMessageBox::warning(this,u8"提示",u8"请选择一个需要上传的栏目");
        return;
    }
    qDebug()<<u8"开始上传，选定栏目：["<<ui->m_uploader->columnName()<<u8"]["<<columnId
         <<u8"]选定文件夹：["<<ui->m_uploader->folderName()<<u8"]["<<folderId<<u8"]";
    auto indexOfColumns = ui->m_stackedContent->indexOf(ui->m_uploader);
    auto indexOfNotice = ui->m_stackedContent->indexOf(ui->m_disableView);
    ui->m_stackedContent->setCurrentIndex(indexOfNotice);
    ui->m_disableView->setNoticeMessage(u8"正在获取上传地址");
    int code = 0;
    qDebug()<<u8"开始获取上传地址";
    if(columnId == "0")
    {
        code =ui->m_uploader->apiOperator()->presonalUploadPath();
    }
    else
    {
        code = ui->m_uploader->apiOperator()->publicUploadPath(columnId);
    }
    if(m_requestClose)
    {
        qDebug()<<u8"窗口关闭，终止获取上传地址步骤";
        return;
    }
    if(code<0)
    {
        qDebug()<<u8"获取上传地址失败，终止后续操作";
        UserMessageBox::warning(this,u8"错误",u8"获取栏目上传路径失败！");
        ui->m_stackedContent->setCurrentIndex(indexOfColumns);
        return;
    }

}

void CloumnChoosenDialog::onGetUploadUrl(const QString& url)
{
    if(m_requestClose)
    {
        qDebug()<<u8"窗口关闭，终止上传步骤";
        return;
    }
    auto indexOfColumns = ui->m_stackedContent->indexOf(ui->m_uploader);
    auto indexOfNotice = ui->m_stackedContent->indexOf(ui->m_disableView);
    if(url.isEmpty())
    {
        qDebug()<<u8"获取上传地址为空，终止后续操作";
        UserMessageBox::warning(this,u8"错误",u8"获取栏目上传路径失败！");
        ui->m_stackedContent->setCurrentIndex(indexOfColumns);
        return;
    }
    qDebug()<<u8"上传地址获取完成，开始上传文件";
    ui->m_disableView->setNoticeMessage(u8"开始上传文件");
    ui->m_uploader->apiOperator()->uploadMediaFile(m_uploadedFiles,url,ui->m_uploader->columnId(),
        m_targetFolderId.isEmpty()?ui->m_uploader->folderId():m_targetFolderId);

}

void CloumnChoosenDialog::onCreateFolder()
{
    if (m_requestCreateFolder)
        return;
    m_requestCreateFolder = true;
    QString columnId = ui->m_uploader->columnId();
    QString folderId = ui->m_uploader->folderId();

    if(columnId.isEmpty())
    {
        UserMessageBox::warning(this,u8"提示",u8"请选择一个栏目用于创建文件夹");
        m_requestCreateFolder = false;
        return;
    }
    qDebug()<<u8"开始创建文件夹，选定栏目：["<<ui->m_uploader->columnName()<<"]["<<ui->m_uploader->columnId()
            <<u8"]选定文件夹：["<<ui->m_uploader->folderName()<<"]["<<ui->m_uploader->folderId()<<"]";
    auto code =ui->m_uploader->apiOperator()->createFolder(columnId,folderId,
        ui->m_folderNameEditor->text(),true);
    if(code<0)
    {
        qDebug()<<u8"创建文件夹失败，终止后续操作";
        UserMessageBox::warning(this,u8"错误",u8"创建文件夹失败！");
        m_requestCreateFolder = false;
        return;
    }

}

void CloumnChoosenDialog::onFolderCreated(int code, const QString& folderId)
{
    m_requestCreateFolder = false;
    qDebug()<<u8"文件夹创建完成，id:"<<folderId;
    QString columnId = ui->m_uploader->columnId();
    ui->m_uploader->switchColumn(columnId);
    ui->m_uploader->switchDir(folderId);
    ui->m_uploader->setColumnIdAndFolderId(columnId,folderId);
    m_targetFolderId = folderId;
}

void CloumnChoosenDialog::closeEvent(QCloseEvent* event)
{
    m_requestClose = true;
    if(ui->m_uploader->apiOperator())
    {
        qDebug()<<"start cancel current request";
        ui->m_uploader->apiOperator()->cancelUpload();
    }
    QDialog::closeEvent(event);
}
