//
// Created by 58226 on 2024/11/14.
//

// You may need to build the project (run Qt uic code generator) to get "ui_CloumnChoosenWidget.h" resolved

#include "cloumnchoosenwidget.h"
#include "ui_cloumnchoosenwidget.h"
#include "uploadoperator.h"
#include "usermessagebox.h"


CloumnChoosenDialog::CloumnChoosenDialog(const QString& baseUrl, const QString& token,const QStringList& uploadFiles, QWidget* parent) :
    m_baseUrl(baseUrl)
    ,m_uploadedFiles(uploadFiles)
    , m_token(token)
    , DragMoveDialog(parent)
    , ui(new Ui::CloumnChoosenWidget)
,m_api(new OnlineService((SubRequests::ApiType::nle)))
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowIcon(QIcon(QString(":/icons/images/recording.svg")));
    init();
}

CloumnChoosenDialog::~CloumnChoosenDialog()
{
    delete m_api;
    delete ui;
}

void CloumnChoosenDialog::init()
{
    qDebug()<<u8"开始上传文件，正在配置基础访问信息:["<<m_baseUrl<<"]["<<m_token<<"]";
    m_api->requestHelper()->setBaseAddress(m_baseUrl);
    m_api->requestHelper()->setToken(m_token);
    bool hasLoginFailed = true;
    m_requestHandler = nullptr;

    qDebug()<<u8"正在验证登录信息";
    //request user info to check if current url and token is valid
    BLOCK_DEBUG(m_requestHandler, m_api->userHelper()->getUserInfoPreExec([&](const QJsonDocument& doc)
                    { hasLoginFailed = false;},[&](int code,const QString& error){qDebug()<<code<<":"<<error;})
                    .timeout(5)
                    .exec());

    if(m_requestClose)
    {
        qDebug()<<u8"窗口关闭，终止后续步骤";
        return;
    }

    m_requestHandler = nullptr;
    if (hasLoginFailed)
    {
        qDebug()<<u8"登录配置无效,取消后续步骤";
        UserMessageBox::warning(nullptr, u8"失败", u8"当前登录配置无效");
        close();
        return;
    }
    qDebug()<<u8"正在初始化上传页面";
    ui->m_uploader->setApiInstance(m_api);
    ui->m_uploader->init();
    ui->m_uploader->setColumnType(NetworkColumnAndFoklerWidget::ColumnType::ColumnType_UploadMaterial);
    ui->m_uploader->hideFilterWidget(true);

    connect(ui->closeButton,&QPushButton::clicked,this,&CloumnChoosenDialog::close);
    connect(ui->cancelButton,&QPushButton::clicked,this,&CloumnChoosenDialog::close);
    connect(ui->sureButton,&QPushButton::clicked,this,&CloumnChoosenDialog::onUploadStart);
    connect(ui->m_uploader->apiOperator(),&UploadOperator::presonalPath,this,&CloumnChoosenDialog::onGetUploadUrl);
    connect(ui->m_uploader->apiOperator(),&UploadOperator::publicUploadPath,this,&CloumnChoosenDialog::onGetUploadUrl);
    connect(ui->m_uploader->apiOperator(),&UploadOperator::updateMediaProcess,this,[&](int curIndex,int totalCount,
        int curProcess,int totalProcess)
    {
        int percent = static_cast<int>((static_cast<double>(curProcess) / totalProcess) * 100.0);;
        qDebug()<<"文件上传"<<curIndex<<"/"<<totalCount<<":"<<percent;
        ui->m_disableView->setNoticeMessage(QString(u8"正在上传%1/%2,%3")
            .arg(curIndex)
            .arg(totalCount)
            .arg(percent));
    });
    connect(ui->m_uploader->apiOperator(),&UploadOperator::mediaFinish,this,[&](int code,int index, int size)
    {
        if(code<0)
        {
            qDebug()<<"文件上传失败"<<index<<"/"<<size <<":"<<code;
            UserMessageBox::warning(nullptr, u8"失败", QString(u8"文件%1上传失败").arg(index));
            return;
        }
        ui->m_disableView->setNoticeMessage(QString(u8"上传完成%1/%2").arg(index).arg(size));
        if((index) == (size-1) && code == 0 )
        {
            qDebug()<<"文件上传完成,上传窗口关闭";
            UserMessageBox::information(nullptr,u8"成功",u8"文件上传完成");
            close();
        }
    });
    qDebug()<<u8"初始化上传页面完成";
}

void CloumnChoosenDialog::onUploadStart()
{

    QString columnId = ui->m_uploader->columnId();
    QString folderId = ui->m_uploader->folderId();

    if(columnId.isEmpty())
    {
        UserMessageBox::warning(this,u8"提示",u8"请选择一个需要上传的栏目");
        return;
    }
    qDebug()<<"开始上传，选定栏目：["<<ui->m_uploader->columnName()<<"]["<<ui->m_uploader->columnId()
         <<"]选定文件夹：["<<ui->m_uploader->folderName()<<"]["<<ui->m_uploader->folderId()<<"]";
    auto indexOfColumns = ui->m_stackedContent->indexOf(ui->m_uploader);
    auto indexOfNotice = ui->m_stackedContent->indexOf(ui->m_disableView);
    ui->m_stackedContent->setCurrentIndex(indexOfNotice);
    ui->m_disableView->setNoticeMessage(u8"正在获取上传地址");
    int code = 0;
    qDebug()<<"开始获取上传地址";
    if(columnId == "0")
    {
        code =ui->m_uploader->apiOperator()->presonalUploadPath();
    }
    else
    {
        code = ui->m_uploader->apiOperator()->publicUploadPath(columnId);
    }
    if(code<0)
    {
        qDebug()<<"获取上传地址失败，终止后续操作";
        UserMessageBox::warning(this,u8"错误",u8"获取栏目上传路径失败！");
        ui->m_stackedContent->setCurrentIndex(indexOfColumns);
        return;
    }

}

void CloumnChoosenDialog::onGetUploadUrl(const QString& url)
{

    auto indexOfColumns = ui->m_stackedContent->indexOf(ui->m_uploader);
    auto indexOfNotice = ui->m_stackedContent->indexOf(ui->m_disableView);
    if(url.isEmpty())
    {
        qDebug()<<"获取上传地址为空，终止后续操作";
        UserMessageBox::warning(this,u8"错误",u8"获取栏目上传路径失败！");
        ui->m_stackedContent->setCurrentIndex(indexOfColumns);
        return;
    }
    qDebug()<<"上传地址获取完成，开始上传文件";
    ui->m_disableView->setNoticeMessage(u8"开始上传文件");
    ui->m_uploader->apiOperator()->uploadMediaFile(m_uploadedFiles,url,ui->m_uploader->columnId(),ui->m_uploader->folderId());
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
