//
// Created by 58226 on 2024/11/29.
//

#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QWidget>

#include "config.h"
#include "dragmovedialog.h"
#include "onlineservice.h"


QT_BEGIN_NAMESPACE
namespace Ui { class LoginDialog; }
QT_END_NAMESPACE

class LoginDialog : public DragMoveDialog
{
    Q_OBJECT

    public:
    explicit LoginDialog(const QString& originUrl,const QString& orginUserName,QSharedPointer<OnlineService> apiInstance,QWidget *parent = nullptr);
    ~LoginDialog();

    void init();

    void closeEvent(QCloseEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;

    void doLogin();

    void afterLogin();


public:
    signals:
    void loginFinished(bool success);
private:
    Ui::LoginDialog *ui;

private:
    QSharedPointer<OnlineService> m_api{nullptr};
    QString m_originUrl;
    QString m_originUserName;
    Config m_config;
    HttpResponse* m_requestHandler{nullptr};
    bool m_hasLogin = false;

    bool m_isSelfChangeText = false;
};



#endif //LOGINDIALOG_H
