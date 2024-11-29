//
// Created by 58226 on 2024/11/29.
//

// You may need to build the project (run Qt uic code generator) to get "ui_LoginDialog.h" resolved

#include "logindialog.h"

#include <QIcon>
#include <QKeyEvent>
#include "ui_logindialog.h"
#include "usermessagebox.h"


LoginDialog::LoginDialog(const QString& originUrl,const QString& originUserName,
    QSharedPointer<OnlineService> api,QWidget *parent) :
   m_api(api),m_originUrl(originUrl),m_originUserName(originUserName),
DragMoveDialog(parent), ui(new Ui::LoginDialog) {
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowIcon(QIcon(QString(":/icons/images/recording.svg")));

    init();
}

LoginDialog::~LoginDialog() {
    delete ui;
}

void LoginDialog::init()
{
    qDebug()<<"base url with"<<m_originUrl;
    ui->urlEditor->setText(m_originUrl);
    ui->userNameEditor->setText(m_originUserName);
    if (m_originUrl.isEmpty())
    {
        ui->urlEditor->setText(m_config.loginAddress);
    }
    if (m_originUserName.isEmpty())
    {
        ui->userNameEditor->setText(m_config.loginUserName);
    }
    //connect()
    connect(ui->closeButton,&QPushButton::clicked,this,&LoginDialog::close);
    connect(ui->loginButton,&QPushButton::clicked,this,&LoginDialog::doLogin);

    connect(ui->urlEditor,&QLineEdit::textChanged,this,[&](QString curText)
    {
        if (m_isSelfChangeText)
            return;
        m_isSelfChangeText = true;
        if (!curText.startsWith("http"))
        {
            curText = "http://" + curText;
            ui->urlEditor->setText(curText);
        }
        m_isSelfChangeText = false;
    });
}

void LoginDialog::closeEvent(QCloseEvent* event)
{
    if (!m_originUrl.isEmpty())
    {
        m_api->loginHelper()->setBaseUrl(m_originUrl);
        qDebug()<<"base url replace with"<<m_originUrl;
    }
    if (m_requestHandler&&m_requestHandler->reply())
    {
        m_requestHandler->reply()->abort();
    }

    DragMoveDialog::closeEvent(event);
}

void LoginDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return)
    {
        event->accept();
    }
    DragMoveDialog::keyPressEvent(event);
}

void LoginDialog::doLogin()
{
    if (m_hasLogin)
        return;
    m_hasLogin = true;
    auto userName = ui->userNameEditor->text();
    auto passWord = ui->passwordEditor->text();
    auto url = ui->urlEditor->text();
    if (userName.isEmpty() || passWord.isEmpty() )
    {
        UserMessageBox::warning(this, u8"错误", u8"用户名、密码不能为空");
        m_hasLogin = false;
        return;
    }
    if (url.isEmpty())
    {
        UserMessageBox::warning(this, u8"错误", u8"服务器配置不能为空");
        m_hasLogin = false;
        return;
    }
    if (!url.startsWith("http://") && !url.startsWith("https://"))
    {
        url = "http://" + url;
    }
    if (!url.endsWith("/"))
    {
        url+="/";
    }
    ui->urlEditor->setText(url);
    m_api->requestHelper()->setBaseAddress(url);
    qDebug()<<"login with url"<<url;
    m_requestHandler = m_api->loginHelper()->loginPreExec(userName, passWord,
        [&,userName,passWord](bool succcess, const QString &error, const QJsonObject &staffInfo)
        {
        m_hasLogin = false;
        if (succcess) {
            qDebug() << "login success,with token:" << m_api->requestHelper()->token();
            afterLogin();
        } else {
            if (error.isEmpty()) {
                QMetaObject::invokeMethod(this,[&](){ UserMessageBox::warning(this,u8"失败",u8"登录失败!请检查当前网络配置是否正常！");});
                qDebug() << "login with empty reply !! ";
                qDebug() << "current address:" << m_api->requestHelper()->baseAddress();
            } else {
                auto doc = QJsonDocument::fromJson(error.toUtf8());
                QMetaObject::invokeMethod(this,[&]()
                    {UserMessageBox::warning( this,u8"失败",doc["error"].toString() + ":" + doc["message"].toString());});
                qDebug() << "login failed!error" << error;
            }
            emit loginFinished(false);
        }
    },true,[&](int code ,const QString& error)
    {
        m_hasLogin = false;
        UserMessageBox::warning(this,u8"失败",u8"登录失败!请检查当前网络配置是否正常！");
        qDebug()<<"login failed with code:"<<code<<"error:"<<error;
        emit loginFinished(false);
    })
    .timeout(5)
    .onTimeout([&]()
    {
        m_hasLogin = false;
        UserMessageBox::warning(this,u8"失败",u8"登陆超时");
       qDebug()<<"login timeout ";
       emit loginFinished(false);
    })
    .exec();
}

void LoginDialog::afterLogin()
{
    //登陆完成，替换originUrl
    m_originUrl = ui->urlEditor->text();
    m_config.loginAddress = m_originUrl;
    m_config.loginUserName = ui->userNameEditor->text();
    m_config.writeJson();
    emit loginFinished(true);
    close();
}

