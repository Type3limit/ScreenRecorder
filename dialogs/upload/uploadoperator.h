//
// Created by 58226 on 2024/11/15.
//

#ifndef UPLOADOPERATOR_H
#define UPLOADOPERATOR_H

#include <QMutex>
#include <QObject>

#include "foldermodel.h"
#include "materiallibhelper.h"
#include "onlineservice.h"

class HttpResponse;

#define TIMEOUTSECOND 60

typedef int StatusCode;

class UploadCanceller : public QObject
{
 Q_OBJECT
public:
 explicit UploadCanceller(QObject *parent = nullptr);
 ~UploadCanceller() override;

 void cancel();

 HttpResponse **response();
private:
 HttpResponse **m_response;
};


class UploadOperator : public QObject
{
    Q_OBJECT
public:
    explicit UploadOperator(QSharedPointer<OnlineService> apiInstance,QObject *parent = nullptr);

    //1.获取个人上传地址
    StatusCode presonalUploadPath();
    QString presonalUploadPathSync();

    StatusCode publicUploadPath(QString columnId);
    QString publicUploadPathSync(QString columnId);
    //2.创建文件夹
    StatusCode createFolder(QString columnId, QString folderId, QString fileName, bool useMediaApi);
    QString createFolderSync(QString columnId, QString folderId, QString fileName, bool useMediaApi);
    //3.上传媒体文件
    StatusCode uploadMediaFile(QStringList filePathList, QString url, QString columnId, QString folderId);
    bool uploadMediaFileSync(QStringList filePathList, QString url, QString columnId, QString folderId);

    void cancelUpload();

    /**
     * @brief getCurrentMedia 我的素材
     * @param folderId
     * @return
     */
    StatusCode getPersonalMedia(bool useMediaApi, QString folderId, QList<MaterialLibHelper::MaterialType> list, const QString &keyword, int page = 1, int size = 30, int order = 0, QString orderKey = "last_modification_time");
    QPair<int, QList<FolderData>> getPersonalMediaSync(bool useMediaApi, QString folderId, QList<MaterialLibHelper::MaterialType> list, const QString &keyword, int page = 1, int size = 30, int order = 0, QString orderKey = "last_modification_time");

    StatusCode getPublicMaterial(MaterialLibHelper::Authority authority, QList<MaterialLibHelper::MaterialType> list, const QString &keyword, int page = 1, int size = 30);
    QPair<int, QList<FolderData>> getPublicMaterialSync(MaterialLibHelper::Authority authority, QList<MaterialLibHelper::MaterialType> list, const QString &keyword, int page = 1, int size = 30);


    StatusCode getAllColumns(bool useMediaApi, bool isNle = false);
    QList<QSharedPointer<SubRequests::ColumnBaseInfo>> getAllColumnsSync(bool useMediaApi, bool isNle = false);

    StatusCode getAllAuthorizedColumns(bool useMediaApi, MaterialLibHelper::Authority authority, bool isNle = false);
    QList<QSharedPointer<SubRequests::ColumnBaseInfo>> getAllAuthorizedColumnsSync(bool useMediaApi, MaterialLibHelper::Authority authority, bool isNle = false);

    QList<QSharedPointer<SubRequests::ColumnBaseInfo>> getColumnDetails(bool useMediaApi, const QStringList& columnIds);
    bool hasColumnOperationsPermission(bool useMediaApi, const QString &columnId, MaterialLibHelper::Authority authority);

    StatusCode getFolder(bool useMediaApi, QString columnId, QString folderId, QList<MaterialLibHelper::MaterialType> list, const QString &keyword, int page = 1, int size = 30, int order = 0, QString orderKey = "last_modification_time");
    QPair<int, QList<FolderData>> getFolderSync(bool useMediaApi, QString columnId, QString folderId, QList<MaterialLibHelper::MaterialType> list, const QString &keyword, int page = 1, int size = 30, int order = 0, QString orderKey = "last_modification_time");

    /**
     * @brief getMaterialDetails 获取素材详情
     * @param materialIds
     * @return
     */
    QList<FolderData> getMaterialDetails(bool useMediaApi, const QStringList& materialIds);
    FolderData getFolderDetails(const QString& folderId);

public:
    signals:
    //上传地址
    void presonalPath(const QString& url);
    void publicPath(const QString& url);
    //创建文件夹
    void folderCreated(StatusCode code, QString newFolderId);

    //上传本地素材完成
    void mediaFinish(StatusCode code, int index, int size);
    void updateMediaProcess(int index, int size, int pos, int duration);

    //文件目录
    void folder(int total, QList<FolderData> children);
    //公共栏目
    void publicColumns(QList<QSharedPointer<SubRequests::ColumnBaseInfo>> &colList);

    //封面
    void cover(const QString &id, const QString &coverPath);
private:
    //下载封面
    QString downloadCover(const QString &materialId, const QString & mobjectId, const QString &url);
    //补齐栏目
    QList<QSharedPointer<SubRequests::ColumnBaseInfo>> polishColumns(bool useMediaApi, QList<QSharedPointer<SubRequests::ColumnBaseInfo>> &colList);

private:
    QMutex m_mutex;
    QList<UploadCanceller *> m_cancelList;
    QAtomicInt m_stop;
    static QMap<QString, QString> s_map;
    QSize m_coverSize;
    QMap<QString, QTimer *> m_lockTimer;
    QSharedPointer<OnlineService> m_api;
};




#endif //UPLOADOPERATOR_H
