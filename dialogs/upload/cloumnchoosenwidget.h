//
// Created by 58226 on 2024/11/14.
//

#ifndef CLOUMNCHOOSENWIDGET_H
#define CLOUMNCHOOSENWIDGET_H

#include <QDialog>
#include <QWidget>
#include "onlineservice.h"
#include "cloumnmodel.h"
#include "dragmovedialog.h"
#include "foldermodel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CloumnChoosenWidget; }
QT_END_NAMESPACE

class CloumnChoosenDialog : public DragMoveDialog
{
    Q_OBJECT

    public:
    explicit CloumnChoosenDialog(const QString& baseUrl,const QString& token,const QStringList& uploadFiles,QWidget *parent = nullptr);
    ~CloumnChoosenDialog() override;



    void init();

public
    slots:
    ///当点击确认后开始上传
    void onUploadStart();
    ///当获取到上传地址后，开始传输文件
    void onGetUploadUrl(const QString& url);
protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::CloumnChoosenWidget *ui;

    QString m_baseUrl{};
    QString m_token{};
    QStringList m_uploadedFiles;
    OnlineService* m_api{nullptr};
    HttpResponse* m_requestHandler{nullptr};
    bool m_requestClose{false};
};


#endif //CLOUMNCHOOSENWIDGET_H
