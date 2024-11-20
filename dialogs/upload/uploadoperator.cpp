#include "uploadoperator.h"

#include <QApplication>
#include <qelapsedtimer.h>


QMap<QString, QString> UploadOperator::s_map;

UploadCanceller::UploadCanceller(QObject *parent)
    : QObject{parent}
{
    m_response = static_cast<HttpResponse **>(malloc(sizeof(size_t)));
    *m_response = nullptr;
}

UploadCanceller::~UploadCanceller()
{
    *m_response = nullptr;
    free((void*)m_response);
}

void UploadCanceller::cancel()
{
    if (*m_response) {
        (*m_response)->reply()->abort();
        qDebug() << "call cancel success";
    }
    qDebug() << "call cancel finished";
}

HttpResponse **UploadCanceller::response()
{
    return m_response;
}


UploadOperator::UploadOperator(OnlineService* apiInstance,QObject* parent)
    : QObject{parent},m_api(apiInstance)
    , m_coverSize(QSize(141, 79)) //project cover
{
    //    connect(this, &UploadOperator::downloadProjectSuccess, this, &UploadOperator::onDownloadProjectSuccess, Qt::QueuedConnection);
}
void UploadOperator::cancelUpload()
{
    m_stop = true;
    QMutexLocker lock(&m_mutex);
    while (!m_cancelList.isEmpty())
    {
        m_cancelList.first()->cancel();
        m_cancelList.removeFirst();
    }
}

StatusCode UploadOperator::getPersonalMedia(bool useMediaApi, QString folderId,
                                            QList<MaterialLibHelper::MaterialType> list, const QString& keyword,
                                            int page, int size, int order,
                                            QString orderKey)
{
    UploadCanceller* canceller = new UploadCanceller;
    HttpResponse** response = canceller->response();
    m_cancelList.append(canceller);

    QStringList strList;
    for (auto type : list)
    {
        strList << QString::number(type);
    }
    *response = m_api->materialHelper()->getUserPersonalMaterialListsPreExec([&](const QJsonDocument& doc)
                                                                             {
                                                                                 QJsonObject jsonObj = doc.object();
                                                                                 int total = jsonObj.value("total").
                                                                                     toInt();
                                                                                 QJsonArray jsonArray = jsonObj.value(
                                                                                     "data").toArray();
                                                                                 QList<FolderData> children;
                                                                                 for (int i = 0; i < jsonArray.size(); i
                                                                                     ++)
                                                                                 {
                                                                                     QJsonObject obj = jsonArray.at(i).
                                                                                         toObject();
                                                                                     FolderData folder;
                                                                                     folder.read(obj);
                                                                                     folder.column_id = "0";
                                                                                     folder.column_name =
                                                                                         QStringLiteral("我的素材");
                                                                                     folder.coverPath = downloadCover(
                                                                                         folder.material_id,
                                                                                         folder.cover_mobject_id,
                                                                                         obj.value("cover_url").
                                                                                         toString());
                                                                                     children << folder;
                                                                                 }
                                                                                 emit folder(total, children);
                                                                             }, page, size, list, folderId, keyword,
                                                                             std::nullopt, std::nullopt, order,
                                                                             std::move(orderKey), EMPTY_CALL)
                     .queryParam("child", 0)
                     .onError([&, canceller](QNetworkReply* reply)
                     {
                         {
                             QMutexLocker lock(&m_mutex);
                             m_cancelList.removeOne(canceller);
                             canceller->deleteLater();
                         }
                     })
                     .onResponse([&, canceller](QNetworkReply* reply)
                     {
                         QMutexLocker lock(&m_mutex);
                         m_cancelList.removeOne(canceller);
                         canceller->deleteLater();
                     }).timeout(TIMEOUTSECOND).exec();

    return 0;
}

QPair<int, QList<FolderData>> UploadOperator::getPersonalMediaSync(bool useMediaApi, QString folderId,
                                                                   QList<MaterialLibHelper::MaterialType> list,
                                                                   const QString& keyword, int page, int size,
                                                                   int order,
                                                                   QString orderKey)
{
    int total;
    QList<FolderData> children;
    UploadCanceller* canceller = new UploadCanceller;
    HttpResponse** response = canceller->response();
    m_cancelList.append(canceller);

    QStringList strList;
    for (auto type : list)
    {
        strList << QString::number(type);
    }
    auto request = m_api->materialHelper()->getUserPersonalMaterialListsPreExec([&](const QJsonDocument& doc)
                            {
                                QJsonObject jsonObj = doc.object();
                                total = jsonObj.value("total").toInt();
                                QJsonArray jsonArray = jsonObj.value("data").toArray();
                                for (int i = 0; i < jsonArray.size(); i++)
                                {
                                    QJsonObject obj = jsonArray.at(i).toObject();
                                    FolderData folder;
                                    folder.read(obj);
                                    folder.column_id = "0";
                                    folder.column_name = QStringLiteral("我的素材");
                                    folder.coverPath = downloadCover(folder.material_id, folder.cover_mobject_id,
                                                                     obj.value("cover_url").toString());
                                    children << folder;
                                }
                            }, page, size, list, folderId, keyword, std::nullopt, std::nullopt, order,
                            std::move(orderKey), EMPTY_CALL)
                        .queryParam("child", 0).onError([&, canceller](QNetworkReply* reply)
                        {
                            {
                                QMutexLocker lock(&m_mutex);
                                m_cancelList.removeOne(canceller);
                                canceller->deleteLater();
                            }
                        }).onResponse([&, canceller](QNetworkReply* reply)
                        {
                            QMutexLocker lock(&m_mutex);
                            m_cancelList.removeOne(canceller);
                            canceller->deleteLater();
                        }).timeout(TIMEOUTSECOND);
    BLOCK(*response, request.exec());

    return QPair<int, QList<FolderData>>(total, children);
}

StatusCode UploadOperator::getPublicMaterial(MaterialLibHelper::Authority authority,
                                             QList<MaterialLibHelper::MaterialType> list, const QString& keyword,
                                             int page, int size)
{
    UploadCanceller* canceller = new UploadCanceller;
    HttpResponse** response = canceller->response();
    m_cancelList.append(canceller);
    auto request = m_api->requestHelper()->getRequest(QNetworkAccessManager::GetOperation,
                                                      "api/media/internal/columns/operation-columns/" +
                                                      m_api->materialHelper()->
                                                             AuthorityStr[authority]).
                          queryParam("nle", true).
                          onSuccess([&, list, keyword, page, size](QNetworkReply* reply)
                          {
                              if (!reply)
                              {
                                  return;
                              }
                              QJsonParseError errRpt{};
                              auto doc = QJsonDocument::fromJson(reply->readAll(), &errRpt);
                              if (errRpt.error != QJsonParseError::NoError)
                              {
                              }
                              else
                              {
                                  QList<QSharedPointer<SubRequests::ColumnBaseInfo>> tempList;
                                  if (doc.isArray())
                                  {
                                      auto array = doc.array();
                                      //先解析栏目列表
                                      for (const auto itr : array)
                                      {
                                          auto job = itr.toObject();
                                          auto columnInfo = QSharedPointer<SubRequests::ColumnBaseInfo>(
                                              new SubRequests::ColumnBaseInfo());
                                          columnInfo->id = QString::number(job["id"].toInt());
                                          columnInfo->parent_id = QString::number(job["parent_id"].toInt());
                                          columnInfo->number = job["number"].toInt();
                                          columnInfo->name = job["name"].toString();
                                          columnInfo->type = job["type"].toInt();
                                          columnInfo->has_permission = job["has_permission"].toBool();
                                          auto nleJob = job["nle_config"].toObject();
                                          columnInfo->nle_config.enabled = nleJob["enabled"].toBool();
                                          columnInfo->nle_config.limit_folder = nleJob["limit_folder"].toInt();
                                          tempList.append(columnInfo);
                                      }
                                  }
                                  else
                                  {
                                      auto job = doc.object();
                                      auto columnInfo = QSharedPointer<SubRequests::ColumnBaseInfo>(
                                          new SubRequests::ColumnBaseInfo());
                                      columnInfo->id = QString::number(job["id"].toInt());
                                      columnInfo->parent_id = QString::number(job["parent_id"].toInt());
                                      columnInfo->number = job["number"].toInt();
                                      columnInfo->name = job["name"].toString();
                                      columnInfo->type = job["type"].toInt();
                                      columnInfo->has_permission = job["has_permission"].toBool();
                                      auto nleJob = job["nle_config"].toObject();
                                      columnInfo->nle_config.enabled = nleJob["enabled"].toBool();
                                      columnInfo->nle_config.limit_folder = nleJob["limit_folder"].toInt();
                                      tempList.append(columnInfo);
                                  }
                                  int64_t total = 0;
                                  QList<FolderData> datas;
                                  for (auto columnInfo : tempList)
                                  {
                                      auto folders = getFolderSync(false, columnInfo->id, "0", list, keyword, page,
                                                                   size);
                                      total += folders.first;
                                      datas.append(folders.second);
                                  }
                                  //去重
                                  QMap<QString, QString> marks;
                                  QList<FolderData> removeDatas;
                                  for (auto data : datas)
                                  {
                                      if (!marks.contains(data.material_id))
                                      {
                                          marks[data.material_id] = "";
                                          removeDatas.append(data);
                                      }
                                  }
                                  //分类，排序，过滤
                                  QList<FolderData> resDatas;
                                  if (list.contains(MaterialLibHelper::folder))
                                  {
                                      QList<FolderData> folderDatas;
                                      for (auto data : removeDatas)
                                      {
                                          if (data.type == MaterialLibHelper::folder)
                                          {
                                              folderDatas.append(data);
                                          }
                                      }
                                      std::sort(folderDatas.begin(), folderDatas.end(),
                                                [](FolderData& a, FolderData& b)-> bool
                                                {
                                                    auto aTime = QDateTime::fromString(
                                                        a.last_modification_time, "yyyy.MM.dd hh:mm");
                                                    auto bTime = QDateTime::fromString(
                                                        b.last_modification_time, "yyyy.MM.dd hh:mm");
                                                    return aTime > bTime;
                                                });
                                      resDatas.append(folderDatas);
                                  }
                                  if (resDatas.size() >= size)
                                  {
                                      resDatas = resDatas.mid((page - 1) * size, size);
                                      emit folder(total, resDatas);
                                      return;
                                  }
                                  if (list.contains(MaterialLibHelper::project))
                                  {
                                      QList<FolderData> projectDatas;
                                      for (auto data : removeDatas)
                                      {
                                          if (data.type == MaterialLibHelper::project)
                                          {
                                              projectDatas.append(data);
                                          }
                                      }
                                      std::sort(projectDatas.begin(), projectDatas.end(),
                                                [](FolderData& a, FolderData& b)-> bool
                                                {
                                                    auto aTime = QDateTime::fromString(
                                                        a.last_modification_time, "yyyy.MM.dd hh:mm");
                                                    auto bTime = QDateTime::fromString(
                                                        b.last_modification_time, "yyyy.MM.dd hh:mm");
                                                    return aTime > bTime;
                                                });
                                      resDatas.append(projectDatas);
                                  }
                                  resDatas = resDatas.mid((page - 1) * size, size);
                                  emit folder(total, resDatas);
                              }
                          }).onError([&, canceller](QNetworkReply* reply)
                          {
                              {
                                  QMutexLocker lock(&m_mutex);
                                  m_cancelList.removeOne(canceller);
                                  canceller->deleteLater();
                              }
                          }).onResponse([&, canceller](QNetworkReply* reply)
                          {
                              QMutexLocker lock(&m_mutex);
                              m_cancelList.removeOne(canceller);
                              canceller->deleteLater();
                          }).timeout(TIMEOUTSECOND);
    *response = request.exec();
    return 0;
}

QPair<int, QList<FolderData>> UploadOperator::getPublicMaterialSync(MaterialLibHelper::Authority authority,
                                                                    QList<MaterialLibHelper::MaterialType> list,
                                                                    const QString& keyword, int page, int size)
{
    QList<QSharedPointer<SubRequests::ColumnBaseInfo>> columnList;
    UploadCanceller* canceller = new UploadCanceller;
    HttpResponse** response = canceller->response();
    m_cancelList.append(canceller);
    auto request = m_api->requestHelper()->getRequest(
                              QNetworkAccessManager::GetOperation,
                              "api/media/internal/columns/operation-columns/" +
                              m_api->materialHelper()
                                   ->AuthorityStr[authority]).
                          queryParam("nle", true).
                          onSuccess([&](QNetworkReply* reply)
                          {
                              if (!reply)
                              {
                                  return;
                              }
                              QJsonParseError errRpt{};
                              auto doc = QJsonDocument::fromJson(reply->readAll(), &errRpt);
                              if (errRpt.error != QJsonParseError::NoError)
                              {
                              }
                              else
                              {
                                  if (doc.isArray())
                                  {
                                      auto array = doc.array();
                                      //先解析栏目列表
                                      for (const auto itr : array)
                                      {
                                          auto job = itr.toObject();
                                          auto columnInfo = QSharedPointer<
                                              SubRequests::ColumnBaseInfo>(
                                              new SubRequests::ColumnBaseInfo());
                                          columnInfo->id = QString::number(job["id"].toInt());
                                          columnInfo->parent_id = QString::number(
                                              job["parent_id"].toInt());
                                          columnInfo->number = job["number"].toInt();
                                          columnInfo->name = job["name"].toString();
                                          columnInfo->type = job["type"].toInt();
                                          columnInfo->has_permission = job["has_permission"].
                                              toBool();
                                          auto nleJob = job["nle_config"].toObject();
                                          columnInfo->nle_config.enabled = nleJob["enabled"].
                                              toBool();
                                          columnInfo->nle_config.limit_folder = nleJob[
                                              "limit_folder"].toInt();
                                          columnList.append(columnInfo);
                                      }
                                  }
                                  else
                                  {
                                      auto job = doc.object();
                                      auto columnInfo = QSharedPointer<
                                          SubRequests::ColumnBaseInfo>(
                                          new SubRequests::ColumnBaseInfo());
                                      columnInfo->id = QString::number(job["id"].toInt());
                                      columnInfo->parent_id = QString::number(
                                          job["parent_id"].toInt());
                                      columnInfo->number = job["number"].toInt();
                                      columnInfo->name = job["name"].toString();
                                      columnInfo->type = job["type"].toInt();
                                      columnInfo->has_permission = job["has_permission"].
                                          toBool();
                                      auto nleJob = job["nle_config"].toObject();
                                      columnInfo->nle_config.enabled = nleJob["enabled"].
                                          toBool();
                                      columnInfo->nle_config.limit_folder = nleJob[
                                          "limit_folder"].toInt();
                                      columnList.append(columnInfo);
                                  }
                              }
                          }).onError([&, canceller](QNetworkReply* reply)
                          {
                              {
                                  QMutexLocker lock(&m_mutex);
                                  m_cancelList.removeOne(canceller);
                                  canceller->deleteLater();
                              }
                          }).onResponse([&, canceller](QNetworkReply* reply)
                          {
                              QMutexLocker lock(&m_mutex);
                              m_cancelList.removeOne(canceller);
                              canceller->deleteLater();
                          }).timeout(TIMEOUTSECOND);
    BLOCK(*response, request.exec());
    int64_t total = 0;
    QList<FolderData> datas;
    for (auto columnInfo : columnList)
    {
        auto folders = getFolderSync(false, columnInfo->id, "0", list, keyword, page, size);
        total += folders.first;
        datas.append(folders.second);
    }
    //去重
    QMap<QString, QString> marks;
    QList<FolderData> removeDatas;
    for (auto data : datas)
    {
        if (!marks.contains(data.material_id))
        {
            marks[data.material_id] = "";
            removeDatas.append(data);
        }
    }
    //分类，排序，过滤
    QList<FolderData> resDatas;
    if (list.contains(MaterialLibHelper::folder))
    {
        QList<FolderData> folderDatas;
        for (auto data : removeDatas)
        {
            if (data.type == MaterialLibHelper::folder)
            {
                folderDatas.append(data);
            }
        }
        std::sort(folderDatas.begin(), folderDatas.end(), [](FolderData& a, FolderData& b)-> bool
        {
            auto aTime = QDateTime::fromString(a.last_modification_time, "yyyy.MM.dd hh:mm");
            auto bTime = QDateTime::fromString(b.last_modification_time, "yyyy.MM.dd hh:mm");
            return aTime > bTime;
        });
        resDatas.append(folderDatas);
    }
    if (resDatas.size() >= size)
    {
        resDatas = resDatas.mid((page - 1) * size, size);
        return QPair<int, QList<FolderData>>(total, resDatas);
    }
    if (list.contains(MaterialLibHelper::project))
    {
        QList<FolderData> projectDatas;
        for (auto data : removeDatas)
        {
            if (data.type == MaterialLibHelper::project)
            {
                projectDatas.append(data);
            }
        }
        std::sort(projectDatas.begin(), projectDatas.end(), [](FolderData& a, FolderData& b)-> bool
        {
            auto aTime = QDateTime::fromString(a.last_modification_time, "yyyy.MM.dd hh:mm");
            auto bTime = QDateTime::fromString(b.last_modification_time, "yyyy.MM.dd hh:mm");
            return aTime > bTime;
        });
        resDatas.append(projectDatas);
    }
    resDatas = resDatas.mid((page - 1) * size, size);
    return QPair<int, QList<FolderData>>(total, resDatas);
}

StatusCode UploadOperator::getAllColumns(bool useMediaApi, bool isNle)
{
    UploadCanceller* canceller = new UploadCanceller;
    HttpResponse** response = canceller->response();
    m_cancelList.append(canceller);

    *response = m_api->materialHelper()->
                       getAllColumnsPreExec(
                           [&](QList<QSharedPointer<SubRequests::ColumnBaseInfo>> colList)
                           {
                               emit publicColumns(colList);
                           }, std::nullopt, std::nullopt, EMPTY_CALL).onError(
                           [&, canceller](QNetworkReply* reply)
                           {
                               {
                                   QMutexLocker lock(&m_mutex);
                                   m_cancelList.removeOne(canceller);
                                   canceller->deleteLater();
                               }
                           }).onResponse([&, canceller](QNetworkReply* reply)
                       {
                           QMutexLocker lock(&m_mutex);
                           m_cancelList.removeOne(canceller);
                           canceller->deleteLater();
                       }).queryParam("nle", isNle).timeout(TIMEOUTSECOND).exec();

    return 0;
}

QList<QSharedPointer<SubRequests::ColumnBaseInfo>> UploadOperator::getAllColumnsSync(bool useMediaApi, bool isNle)
{
    QList<QSharedPointer<SubRequests::ColumnBaseInfo>> columnList;
    UploadCanceller* canceller = new UploadCanceller;
    HttpResponse** response = canceller->response();
    m_cancelList.append(canceller);

    auto request = m_api->materialHelper()->getAllColumnsPreExec(
        [&](QList<QSharedPointer<SubRequests::ColumnBaseInfo>> colList)
        {
            for (auto column : colList)
            {
                columnList.append(column);
            }
        }, std::nullopt, std::nullopt, EMPTY_CALL).onError([&, canceller](QNetworkReply* reply)
    {
        {
            QMutexLocker lock(&m_mutex);
            m_cancelList.removeOne(canceller);
            canceller->deleteLater();
        }
    }).onResponse([&, canceller](QNetworkReply* reply)
    {
        QMutexLocker lock(&m_mutex);
        m_cancelList.removeOne(canceller);
        canceller->deleteLater();
    }).queryParam("nle", isNle).timeout(TIMEOUTSECOND);
    BLOCK(*response, request.exec());

    return columnList;
}

StatusCode UploadOperator::getAllAuthorizedColumns(bool useMediaApi, MaterialLibHelper::Authority authority, bool isNle)
{

    UploadCanceller *canceller = new UploadCanceller;
    HttpResponse **response = canceller->response();
    m_cancelList.append(canceller);
    QString url;
    if(useMediaApi) {
        url = "api/media/internal/columns/operation-columns/" + m_api->materialHelper()->AuthorityStr[authority];
    }else {
        url = "api/nle/internal/columns/operation-columns/" +m_api->materialHelper()->AuthorityStr[authority];
    }
    auto request = m_api->requestHelper()->getRequest(QNetworkAccessManager::GetOperation, url).
                    queryParam("nle", isNle).
                    onSuccess([&, useMediaApi](QNetworkReply *reply) {
                        if(!reply) {
                            return;
                        }
                        QJsonParseError errRpt{};
                        auto doc = QJsonDocument::fromJson(reply->readAll(), &errRpt);
                        if (errRpt.error != QJsonParseError::NoError) {
                        }else {
                            if(doc.isArray()) {
                                auto array = doc.array();
                                QList<QSharedPointer<SubRequests::ColumnBaseInfo> > tempList;
                                //先解析栏目列表
                                for (const auto itr: array) {
                                    auto job = itr.toObject();
                                    auto columnInfo = QSharedPointer<SubRequests::ColumnBaseInfo>(new SubRequests::ColumnBaseInfo());
                                    columnInfo->id = QString::number(job["id"].toInt());
                                    columnInfo->parent_id = QString::number(job["parent_id"].toInt());
                                    columnInfo->number = job["number"].toInt();
                                    columnInfo->name = job["name"].toString();
                                    columnInfo->type = job["type"].toInt();
                                    columnInfo->has_permission = job["has_permission"].toBool();
                                    auto nleJob = job["nle_config"].toObject();
                                    columnInfo->nle_config.enabled = nleJob["enabled"].toBool();
                                    columnInfo->nle_config.limit_folder = nleJob["limit_folder"].toInt();
                                    tempList.append(columnInfo);
                                }
                                auto columnList = polishColumns(useMediaApi, tempList);
                                emit publicColumns(columnList);
//                                auto columnList = SubRequests::findChildColumns(doc);
//                                emit publicColumns(columnList);
                            }else {
                                auto job = doc.object();
                                QList<QSharedPointer<SubRequests::ColumnBaseInfo> > columnList;
                                auto columnInfo = QSharedPointer<SubRequests::ColumnBaseInfo>(new SubRequests::ColumnBaseInfo());
                                columnInfo->id = QString::number(job["id"].toInt());
                                columnInfo->parent_id = QString::number(job["parent_id"].toInt());
                                columnInfo->number = job["number"].toInt();
                                columnInfo->name = job["name"].toString();
                                columnInfo->type = job["type"].toInt();
                                columnInfo->has_permission = job["has_permission"].toBool();
                                auto nleJob = job["nle_config"].toObject();
                                columnInfo->nle_config.enabled = nleJob["enabled"].toBool();
                                columnInfo->nle_config.limit_folder = nleJob["limit_folder"].toInt();
                                columnList.append(columnInfo);
                                emit publicColumns(columnList);
                            }
                        }
                    }).onError([&, canceller](QNetworkReply::NetworkError error){
                       {
                           QMutexLocker lock(&m_mutex);
                           m_cancelList.removeOne(canceller);
                           canceller->deleteLater();
                       }
                    }).onResponse([&, canceller](QNetworkReply *reply) {
                        QMutexLocker lock(&m_mutex);
                        m_cancelList.removeOne(canceller);
                        canceller->deleteLater();
                    }).timeout(TIMEOUTSECOND);
    *response = request.exec();
    return 0;
}

QList<QSharedPointer<SubRequests::ColumnBaseInfo>> UploadOperator::getAllAuthorizedColumnsSync(bool useMediaApi,
    MaterialLibHelper::Authority authority, bool isNle)
{

    QList<QSharedPointer<SubRequests::ColumnBaseInfo> > columnList;
    UploadCanceller *canceller = new UploadCanceller;
    HttpResponse **response = canceller->response();
    m_cancelList.append(canceller);
    QString url;
    if( useMediaApi) {
        url = "api/media/internal/columns/operation-columns/" + m_api->materialHelper()->AuthorityStr[authority];
    }else {
        url = "api/nle/internal/columns/operation-columns/" + m_api->materialHelper()->AuthorityStr[authority];
    }
    auto request = m_api->requestHelper()->getRequest(QNetworkAccessManager::GetOperation, url).
                    queryParam("nle", isNle).
                    onSuccess([&, useMediaApi](QNetworkReply *reply) {
                        if(!reply) {
                            return;
                        }
                        QJsonParseError errRpt{};
                        auto doc = QJsonDocument::fromJson(reply->readAll(), &errRpt);
                        if (errRpt.error != QJsonParseError::NoError) {
                        }else {
                            if(doc.isArray()) {
                                auto array = doc.array();
                                QList<QSharedPointer<SubRequests::ColumnBaseInfo> > tempList;
                                //先解析栏目列表
                                for (const auto itr: array) {
                                    auto job = itr.toObject();
                                    auto columnInfo = QSharedPointer<SubRequests::ColumnBaseInfo>(new SubRequests::ColumnBaseInfo());
                                    columnInfo->id = QString::number(job["id"].toInt());
                                    columnInfo->parent_id = QString::number(job["parent_id"].toInt());
                                    columnInfo->number = job["number"].toInt();
                                    columnInfo->name = job["name"].toString();
                                    columnInfo->type = job["type"].toInt();
                                    columnInfo->has_permission = job["has_permission"].toBool();
                                    auto nleJob = job["nle_config"].toObject();
                                    columnInfo->nle_config.enabled = nleJob["enabled"].toBool();
                                    columnInfo->nle_config.limit_folder = nleJob["limit_folder"].toInt();
                                    tempList.append(columnInfo);
                                }
                                columnList = polishColumns(useMediaApi, tempList);
//                                columnList = SubRequests::findChildColumns(doc);
                            }else {
                                auto job = doc.object();
                                auto columnInfo = QSharedPointer<SubRequests::ColumnBaseInfo>(new SubRequests::ColumnBaseInfo());
                                columnInfo->id = QString::number(job["id"].toInt());
                                columnInfo->parent_id = QString::number(job["parent_id"].toInt());
                                columnInfo->number = job["number"].toInt();
                                columnInfo->name = job["name"].toString();
                                columnInfo->type = job["type"].toInt();
                                columnInfo->has_permission = job["has_permission"].toBool();
                                auto nleJob = job["nle_config"].toObject();
                                columnInfo->nle_config.enabled = nleJob["enabled"].toBool();
                                columnInfo->nle_config.limit_folder = nleJob["limit_folder"].toInt();
                                columnList.append(columnInfo);
                            }
                        }
                    }).onError([&, canceller](QNetworkReply::NetworkError error){
                       {
                           QMutexLocker lock(&m_mutex);
                           m_cancelList.removeOne(canceller);
                           canceller->deleteLater();
                       }
                    }).onResponse([&, canceller](QNetworkReply *reply) {
                        QMutexLocker lock(&m_mutex);
                        m_cancelList.removeOne(canceller);
                        canceller->deleteLater();
                    }).timeout(TIMEOUTSECOND);
    BLOCK(*response, request.exec());
    return columnList;
}

QList<QSharedPointer<SubRequests::ColumnBaseInfo>> UploadOperator::getColumnDetails(bool useMediaApi,
    const QStringList& columnIds)
{

    UploadCanceller *canceller = new UploadCanceller;
    HttpResponse **response = canceller->response();
    m_cancelList.append(canceller);
    QList<QSharedPointer<SubRequests::ColumnBaseInfo>> columns;
    QString url;
    if(useMediaApi) {
        url = "api/media/internal/columns/" + columnIds.join(",");
    }else {
        url = "api/nle/internal/columns/" + columnIds.join(",");
    }
    auto request = m_api->requestHelper()->getRequest(QNetworkAccessManager::GetOperation, url).
                onSuccess([&](QNetworkReply *reply) {
                        if(!reply) {
                            return;
                        }
                        QJsonParseError errRpt{};
                        auto doc = QJsonDocument::fromJson(reply->readAll(), &errRpt);
                        if (errRpt.error != QJsonParseError::NoError) {

                        }else {
                            if(doc.isArray()) {
                                columns = SubRequests::findChildColumns(doc);
                            }else {
                                auto job = doc.object();
                                auto columnInfo = QSharedPointer<SubRequests::ColumnBaseInfo>(new SubRequests::ColumnBaseInfo());
                                columnInfo->id = QString::number(job["id"].toInt());
                                columnInfo->parent_id = QString::number(job["parent_id"].toInt());
                                columnInfo->number = job["number"].toInt();
                                columnInfo->name = job["name"].toString();
                                columnInfo->type = job["type"].toInt();
                                columnInfo->has_permission = job["has_permission"].toBool();
                                auto nleJob = job["nle_config"].toObject();
                                columnInfo->nle_config.enabled = nleJob["enabled"].toBool();
                                columnInfo->nle_config.limit_folder = nleJob["limit_folder"].toInt();
                                columns.append(columnInfo);
                            }
                        }
                    }).onError([&, canceller](QNetworkReply::NetworkError error){
                       {
                           QMutexLocker lock(&m_mutex);
                           m_cancelList.removeOne(canceller);
                           canceller->deleteLater();
                       }
                    }).onResponse([&, canceller](QNetworkReply *reply) {
                        QMutexLocker lock(&m_mutex);
                        m_cancelList.removeOne(canceller);
                        canceller->deleteLater();
                    }).timeout(TIMEOUTSECOND);
    BLOCK(*response, request.exec());
    return columns;
}

bool UploadOperator::hasColumnOperationsPermission(bool useMediaApi, const QString& columnId,
    MaterialLibHelper::Authority authority)
{

UploadCanceller *canceller = new UploadCanceller;
HttpResponse **response = canceller->response();
m_cancelList.append(canceller);
bool hasPremession = false;
QString url;
if(useMediaApi) {
    url = "api/media/internal/columns/" + columnId;
}else {
    url = "api/nle/internal/columns/" + columnId;
}
auto request = m_api->requestHelper()->getRequest(QNetworkAccessManager::GetOperation, url).
               onSuccess([&, authority](QNetworkReply *reply) {
                        if(!reply) {
                            return;
                        }
                        QJsonParseError errRpt{};
                        auto doc = QJsonDocument::fromJson(reply->readAll(), &errRpt);
                        if (errRpt.error != QJsonParseError::NoError) {

                        }else {
                            auto job = doc.object();
                            auto operationArray = job["member_permissions"].toArray();
                            for(int i = 0; i < operationArray.size(); i++) {
                                if(operationArray.at(i).toString().toLower() ==
                                   m_api->materialHelper()->AuthorityStr[authority].toLower()) {
                                    hasPremession = true;
                                    break;
                                }
                            }
                        }
                    }).onError([&, canceller](QNetworkReply::NetworkError error){
                       {
                           QMutexLocker lock(&m_mutex);
                           m_cancelList.removeOne(canceller);
                           canceller->deleteLater();
                       }
                   }).onResponse([&, canceller](QNetworkReply *reply) {
                        QMutexLocker lock(&m_mutex);
                        m_cancelList.removeOne(canceller);
                        canceller->deleteLater();
                   }).timeout(TIMEOUTSECOND);
BLOCK(*response, request.exec());
return hasPremession;
}

StatusCode UploadOperator::getFolder(bool useMediaApi, QString columnId, QString folderId,
    QList<MaterialLibHelper::MaterialType> list, const QString& keyword, int page, int size, int order,
    QString orderKey)
{

QList<MaterialLibHelper::SystemType> sys{ MaterialLibHelper::MediaLib, MaterialLibHelper::NLE, MaterialLibHelper::AIGC };
UploadCanceller *canceller = new UploadCanceller;
HttpResponse **response = canceller->response();
m_cancelList.append(canceller);
if(useMediaApi) {
    *response = m_api->materialHelper()->getMaterialListsPreExec([&](const QJsonDocument & doc) {
        QJsonObject jsonObj = doc.object();
        int total = jsonObj.value("total").toInt();
        QJsonArray jsonArray = jsonObj.value("data").toArray();
        QList<FolderData> children;
        for(int i = 0; i < jsonArray.size(); i++) {
            QJsonObject obj = jsonArray.at(i).toObject();
            FolderData folder;
            folder.read(obj);
            folder.coverPath = downloadCover(folder.material_id, folder.cover_mobject_id, obj.value("cover_url").toString());
            children << folder;
        }
        emit folder(total, children);
    }, page, size, list, columnId, folderId, keyword, std::nullopt, std::nullopt, order, std::move(orderKey)
    , true, std::nullopt, sys, EMPTY_CALL).queryParam("child", 0).onError([&, canceller](QNetworkReply *reply) {
        {
            QMutexLocker lock(&m_mutex);
            m_cancelList.removeOne(canceller);
            canceller->deleteLater();
        }
    }).onResponse([&, canceller](QNetworkReply *reply) {
        QMutexLocker lock(&m_mutex);
        m_cancelList.removeOne(canceller);
        canceller->deleteLater();
    }).timeout(TIMEOUTSECOND).exec();
}else {
    QStringList strList;
    for(auto type: list) {
        strList << QString::number(type);
    }
    *response = m_api->requestHelper()->
                getRequest(QNetworkAccessManager::GetOperation, "/api/nle/internal/projects")
                    .queryParam("page", page)
                    .queryParam("size", size)
                    .queryParam("type", strList.join(","))
                    .queryParam("keyword", keyword)
                    .queryParam("start", "")
                    .queryParam("stop", "")
                    .queryParam("column", columnId)
                    .queryParam("folder", folderId)
                    .queryParam("orderKey", orderKey)
                    .queryParam("order", order)
                    .queryParam("child", 0)
                    .onSuccess([&](QNetworkReply * reply) {
                        if(!reply) {
                            return;
                        }
                        QJsonParseError errRpt{};
                        auto doc = QJsonDocument::fromJson(reply->readAll(), &errRpt);
                        if (errRpt.error != QJsonParseError::NoError) {

                        } else {
                            QJsonObject jsonObj = doc.object();
                            int total = jsonObj.value("total").toInt();
                            QJsonArray jsonArray = jsonObj.value("data").toArray();
                            QList<FolderData> children;
                            for(int i = 0; i < jsonArray.size(); i++) {
                                QJsonObject obj = jsonArray.at(i).toObject();
                                FolderData folder;
                                folder.read(obj);
                                folder.coverPath = downloadCover(folder.material_id, folder.cover_mobject_id, obj.value("cover_url").toString());
                                children << folder;
                            }
                            emit folder(total, children);
                        }
                    }).onError([&, canceller](QNetworkReply *reply) {
                        {
                            QMutexLocker lock(&m_mutex);
                            m_cancelList.removeOne(canceller);
                            canceller->deleteLater();
                        }
                    }).onResponse([&, canceller](QNetworkReply *reply) {
                        QMutexLocker lock(&m_mutex);
                        m_cancelList.removeOne(canceller);
                        canceller->deleteLater();
                    }).timeout(TIMEOUTSECOND).exec();
}
return 0;
}

QPair<int, QList<FolderData>> UploadOperator::getFolderSync(bool useMediaApi, QString columnId, QString folderId,
    QList<MaterialLibHelper::MaterialType> list, const QString& keyword, int page, int size, int order,
    QString orderKey)
{

    int total;
    QList<FolderData> children;
    QList<MaterialLibHelper::SystemType> sys{ MaterialLibHelper::MediaLib, MaterialLibHelper::NLE, MaterialLibHelper::AIGC };
    UploadCanceller *canceller = new UploadCanceller;
    HttpResponse **response = canceller->response();
    m_cancelList.append(canceller);
    if(useMediaApi) {
        auto request = m_api->materialHelper()->getMaterialListsPreExec([&](const QJsonDocument & doc) {
            QJsonObject jsonObj = doc.object();
            total = jsonObj.value("total").toInt();
            QJsonArray jsonArray = jsonObj.value("data").toArray();
            for(int i = 0; i < jsonArray.size(); i++) {
                QJsonObject obj = jsonArray.at(i).toObject();
                FolderData folder;
                folder.read(obj);
                folder.coverPath = downloadCover(folder.material_id, folder.cover_mobject_id, obj.value("cover_url").toString());
                children << folder;
            }
        }, page, size, list, columnId, folderId, keyword, std::nullopt, std::nullopt, order, std::move(orderKey), true, std::nullopt, sys
        , EMPTY_CALL).queryParam("child", 0).onError([&, canceller](QNetworkReply *reply) {
           {
               QMutexLocker lock(&m_mutex);
               m_cancelList.removeOne(canceller);
               canceller->deleteLater();
           }
       }).onResponse([&, canceller](QNetworkReply *reply) {
            QMutexLocker lock(&m_mutex);
            m_cancelList.removeOne(canceller);
            canceller->deleteLater();
        }).timeout(TIMEOUTSECOND);
        BLOCK(*response, request.exec());
    }else {
        QStringList strList;
        for(auto type: list) {
            strList << QString::number(type);
        }
        auto request = m_api->requestHelper()->
                       getRequest(QNetworkAccessManager::GetOperation, "/api/nle/internal/projects")
                           .queryParam("page", page)
                           .queryParam("size", size)
                           .queryParam("type", strList.join(","))
                           .queryParam("keyword", keyword)
                           .queryParam("start", "")
                           .queryParam("stop", "")
                           .queryParam("column", columnId)
                           .queryParam("folder", folderId)
                           .queryParam("orderKey", orderKey)
                           .queryParam("order", order)
                           .queryParam("child", 0)
                           .onSuccess([&](QNetworkReply * reply) {
                               if(!reply) {
                                   return;
                               }
                               QJsonParseError errRpt{};
                               auto doc = QJsonDocument::fromJson(reply->readAll(), &errRpt);
                               if (errRpt.error != QJsonParseError::NoError) {
                               } else {
                                   QJsonObject jsonObj = doc.object();
                                   total = jsonObj.value("total").toInt();
                                   QJsonArray jsonArray = jsonObj.value("data").toArray();
                                   for(int i = 0; i < jsonArray.size(); i++) {
                                       QJsonObject obj = jsonArray.at(i).toObject();
                                       FolderData folder;
                                       folder.read(obj);
                                       folder.coverPath = downloadCover(folder.material_id, folder.cover_mobject_id, obj.value("cover_url").toString());
                                       children << folder;
                                   }
                               }
                           }).onError([&, canceller](QNetworkReply *reply) {
                               {
                                   QMutexLocker lock(&m_mutex);
                                   m_cancelList.removeOne(canceller);
                                   canceller->deleteLater();
                               }
                           }).onResponse([&, canceller](QNetworkReply *reply) {
                               QMutexLocker lock(&m_mutex);
                               m_cancelList.removeOne(canceller);
                               canceller->deleteLater();
                           }).timeout(TIMEOUTSECOND);
        BLOCK(*response, request.exec());
    }
    return QPair<int, QList<FolderData>>(total, children);
}

QList<FolderData> UploadOperator::getMaterialDetails(bool useMediaApi, const QStringList& materialIds)
{

    QList<FolderData> datas;
    UploadCanceller* canceller = new UploadCanceller;
    HttpResponse** response = canceller->response();
    m_cancelList.append(canceller);
    QString url= "api/media/internal/media/multi/" + materialIds.join(",");

    auto request = m_api->requestHelper()->getRequest(QNetworkAccessManager::GetOperation, url)
                                                   .onSuccess([&](QNetworkReply* reply)
                                                   {
                                                       if (!reply)
                                                       {
                                                           return;
                                                       }
                                                       QJsonParseError errRpt{};
                                                       auto doc = QJsonDocument::fromJson(reply->readAll(), &errRpt);
                                                       if (errRpt.error != QJsonParseError::NoError)
                                                       {
                                                       }
                                                       else
                                                       {
                                                           QJsonArray jsonArray = doc.array();
                                                           for (int i = 0; i < jsonArray.size(); i++)
                                                           {
                                                               QJsonObject obj = jsonArray.at(i).toObject();
                                                               FolderData folder;
                                                               folder.read(obj);
                                                               folder.coverPath = downloadCover(
                                                                   folder.material_id, folder.cover_mobject_id,
                                                                   obj.value("cover_url").toString());
                                                               datas << folder;
                                                           }
                                                       }
                                                   }).onError([&, canceller](QNetworkReply* reply)
                                                   {
                                                       {
                                                           QMutexLocker lock(&m_mutex);
                                                           m_cancelList.removeOne(canceller);
                                                           canceller->deleteLater();
                                                       }
                                                   }).onResponse([&, canceller](QNetworkReply* reply)
                                                   {
                                                       QMutexLocker lock(&m_mutex);
                                                       m_cancelList.removeOne(canceller);
                                                       canceller->deleteLater();
                                                   }).timeout(TIMEOUTSECOND);
    BLOCK(*response, request.exec());
    return datas;
}

QString UploadOperator::downloadCover(const QString& materialId, const QString& mobjectId, const QString& url)
{
    QDir coverDir(QApplication::applicationDirPath() + QString("/cache/cover/netproject_cover/") + materialId);
    if(!coverDir.exists()) {
        coverDir.mkpath(coverDir.absolutePath());
    }
    QString filePath = coverDir.absolutePath() + "/" + mobjectId + "." +QFileInfo(url).suffix();
    if(QFile::exists(filePath)) {
        return filePath;
    }else {
        UploadCanceller *canceller = new UploadCanceller;
        HttpResponse **response = canceller->response();
        m_cancelList.append(canceller);
        *response =m_api->fileHelper()->downloadFilePreExec(filePath
            , url
            , [&](qint64 pos, qint64 duration){

            }
            , [&, materialId](const QString &success) {
                QPixmap pixmap(success);
                pixmap = pixmap.scaled(m_coverSize.width(), m_coverSize.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                pixmap.save(success);
                emit cover(materialId, success);
            }
            , [&, canceller](const QString &error) {
                {
                    QMutexLocker lock(&m_mutex);
                    m_cancelList.removeOne(canceller);
                    canceller->deleteLater();
                }
            }).onResponse([&, canceller](QNetworkReply *reply) {
                QMutexLocker lock(&m_mutex);
                m_cancelList.removeOne(canceller);
                canceller->deleteLater();
            }).exec();
    }
    return QString();
}

QList<QSharedPointer<SubRequests::ColumnBaseInfo>> UploadOperator::polishColumns(bool useMediaApi,
                                                                                 QList<QSharedPointer<SubRequests::ColumnBaseInfo>>& colList)
{
    QList<QSharedPointer<SubRequests::ColumnBaseInfo>> resList;
    QMap<QString, QSharedPointer<SubRequests::ColumnBaseInfo>> marker;
    for(auto col: colList) {
        if(marker.contains(col->id)) {
            continue;
        }
        //需要补齐
        if(col->parent_id != "0") {
            if(marker.contains(col->parent_id)) {
                marker[col->parent_id]->child.append(col);
                marker[col->id] = col;
            }else {
                marker[col->id] = col;
                QSharedPointer<SubRequests::ColumnBaseInfo> child = col;
                while(child->parent_id != "0" && !marker.contains(child->parent_id)) {
                    auto columns = getColumnDetails(useMediaApi, {child->parent_id});
                    if(columns.isEmpty()) {
                        break;
                    }
                    auto parent = columns[0];
                    marker[parent->id] = parent;
                    parent->child.append(child);
                    child = parent;
                }
                if(child->parent_id == "0") {
                    resList.append(child);
                }else if(marker.contains(child->parent_id)){
                    marker[child->parent_id]->child.append(child);
                }
            }
        }else{
            marker[col->id] = col;
            resList.append(col);
        }
    }
    return resList;
}

StatusCode UploadOperator::presonalUploadPath()
{
    UploadCanceller* canceller = new UploadCanceller;
    HttpResponse** response = canceller->response();
    m_cancelList.append(canceller);
    QString url = "api/media/internal/users/current/uploader";

    *response = m_api->requestHelper()->getRequest(QNetworkAccessManager::GetOperation, url).
                       onSuccess([&](QNetworkReply* reply)
                       {
                           if (!reply)
                           {
                               emit presonalPath("");
                               qDebug() << __FUNCTION__ << QStringLiteral("获取上传地址失败");
                               return;
                           }
                           QString url = QString::fromUtf8(reply->readAll()).remove(QString("\""));
                           emit presonalPath(url);
                       }).onError([&, canceller](QNetworkReply::NetworkError error)
                       {
                           {
                               QMutexLocker lock(&m_mutex);
                               m_cancelList.removeOne(canceller);
                               canceller->deleteLater();
                           }
                           qDebug() << "error" << error;
                           emit presonalPath("");
                           qDebug() << __FUNCTION__ << QStringLiteral("获取上传地址失败");
                       }).onResponse([&, canceller](QNetworkReply* reply)
                       {
                           QMutexLocker lock(&m_mutex);
                           m_cancelList.removeOne(canceller);
                           canceller->deleteLater();
                       }).timeout(TIMEOUTSECOND).exec();
    return 0;
}

QString UploadOperator::presonalUploadPathSync()
{
    QString res;
    UploadCanceller* canceller = new UploadCanceller;
    HttpResponse** response = canceller->response();
    m_cancelList.append(canceller);
    QString url = "api/media/internal/users/current/uploader";

    auto request = m_api->requestHelper()->getRequest(QNetworkAccessManager::GetOperation, url).
                          onSuccess([&](QNetworkReply* reply)
                          {
                              if (!reply)
                              {
                                  qDebug() << __FUNCTION__ << QStringLiteral("获取上传地址失败");
                                  return;
                              }
                              res = QString::fromUtf8(reply->readAll()).remove(QString("\""));
                          }).onError([&, canceller](QNetworkReply::NetworkError error)
                          {
                              {
                                  QMutexLocker lock(&m_mutex);
                                  m_cancelList.removeOne(canceller);
                                  canceller->deleteLater();
                              }
                              qDebug() << "error" << error;
                              qDebug() << __FUNCTION__ << QStringLiteral("获取上传地址失败");
                          }).onResponse([&, canceller](QNetworkReply* reply)
                          {
                              QMutexLocker lock(&m_mutex);
                              m_cancelList.removeOne(canceller);
                              canceller->deleteLater();
                          }).timeout(TIMEOUTSECOND);
    BLOCK(*response, request.exec());
    return res;
}

StatusCode UploadOperator::publicUploadPath(QString columnId)
{
    UploadCanceller* canceller = new UploadCanceller;
    HttpResponse** response = canceller->response();
    m_cancelList.append(canceller);
    QString url = "api/media/internal/uploader/" + columnId;

    *response = m_api->requestHelper()->getRequest(QNetworkAccessManager::GetOperation, url).
                       onSuccess([&](QNetworkReply* reply)
                       {
                           if (!reply)
                           {
                               emit publicPath("");
                               return;
                           }
                           QString url = QString::fromUtf8(reply->readAll()).remove(QString("\""));
                           emit publicPath(url);
                       }).onError([&, canceller](QNetworkReply::NetworkError error)
                       {
                           {
                               QMutexLocker lock(&m_mutex);
                               m_cancelList.removeOne(canceller);
                               canceller->deleteLater();
                           }
                           qDebug() << "error" << error;
                           emit publicPath("");
                       }).onResponse([&, canceller](QNetworkReply* reply)
                       {
                           QMutexLocker lock(&m_mutex);
                           m_cancelList.removeOne(canceller);
                           canceller->deleteLater();
                       }).timeout(TIMEOUTSECOND).exec();
    return 0;
}

QString UploadOperator::publicUploadPathSync(QString columnId)
{
    //1.先获取上传地址
    QString res;
    UploadCanceller* canceller = new UploadCanceller;
    HttpResponse** response = canceller->response();
    m_cancelList.append(canceller);
    QString url = "api/media/internal/uploader/" + columnId;
    auto request = m_api->requestHelper()->getRequest(QNetworkAccessManager::GetOperation, url).
                          onSuccess([&](QNetworkReply* reply)
                          {
                              if (!reply)
                              {
                                  return;
                              }
                              res = QString::fromUtf8(reply->readAll()).remove(QString("\""));
                          }).
                          onError([&, canceller](QNetworkReply::NetworkError error)
                          {
                              {
                                  QMutexLocker lock(&m_mutex);
                                  m_cancelList.removeOne(canceller);
                                  canceller->deleteLater();
                              }
                              qDebug() << "error" << error;
                          }).onResponse([&, canceller](QNetworkReply* reply)
                          {
                              QMutexLocker lock(&m_mutex);
                              m_cancelList.removeOne(canceller);
                              canceller->deleteLater();
                          }).timeout(TIMEOUTSECOND);
    BLOCK(*response, request.exec());
    return res;
}

StatusCode UploadOperator::createFolder(QString columnId, QString folderId, QString fileName, bool useMediaApi)
{
    UploadCanceller* canceller = new UploadCanceller;
    HttpResponse** response = canceller->response();
    m_cancelList.append(canceller);
    QJsonObject job;
    job["column_id"] = columnId.toInt();
    job["folder_id"] = folderId.toInt();
    job["name"] = fileName;
    QString url = "api/media/internal/folders";

    *response = m_api->requestHelper()->getRequest(QNetworkAccessManager::PostOperation, url)
                     .body(job)
                     .onSuccess([&](QNetworkReply* reply)
                     {
                         if (!reply)
                         {
                             emit folderCreated(-1, "");
                             qDebug() << __FUNCTION__ << QStringLiteral("创建文件夹失败");
                             return;
                         }
                         QString folderId = QString::number(reply->readAll().toInt());
                         emit folderCreated(0, folderId);
                     }).onError([&, canceller](QNetworkReply::NetworkError error)
                     {
                         {
                             QMutexLocker lock(&m_mutex);
                             m_cancelList.removeOne(canceller);
                             canceller->deleteLater();
                         }
                         emit folderCreated(-1, "");
                         qDebug() << __FUNCTION__ << QStringLiteral("创建文件夹失败");
                     }).onResponse([&, canceller](QNetworkReply* reply)
                     {
                         QMutexLocker lock(&m_mutex);
                         m_cancelList.removeOne(canceller);
                         canceller->deleteLater();
                     }).timeout(TIMEOUTSECOND).exec();
    return 0;
}

QString UploadOperator::createFolderSync(QString columnId, QString folderId, QString fileName, bool useMediaApi)
{
    UploadCanceller* canceller = new UploadCanceller;
    HttpResponse** response = canceller->response();
    m_cancelList.append(canceller);
    QJsonObject job;
    job["column_id"] = columnId.toInt();
    job["folder_id"] = folderId.toInt();
    job["name"] = fileName;
    QString res;
    QString url = "api/media/internal/folders";

    auto request = m_api->requestHelper()->getRequest(QNetworkAccessManager::PostOperation, url)
                        .body(job)
                        .onSuccess([&](QNetworkReply* reply)
                        {
                            if (!reply)
                            {
                                qDebug() << __FUNCTION__ << QStringLiteral("创建文件夹失败");
                                return;
                            }
                            res = QString::number(reply->readAll().toInt());
                        }).onError([&, canceller](QNetworkReply::NetworkError error)
                        {
                            {
                                QMutexLocker lock(&m_mutex);
                                m_cancelList.removeOne(canceller);
                                canceller->deleteLater();
                            }
                            qDebug() << __FUNCTION__ << QStringLiteral("创建文件夹失败");
                        }).onResponse([&, canceller](QNetworkReply* reply)
                        {
                            QMutexLocker lock(&m_mutex);
                            m_cancelList.removeOne(canceller);
                            canceller->deleteLater();
                        }).timeout(TIMEOUTSECOND);
    BLOCK(*response, request.exec());
    return res;
}

StatusCode UploadOperator::uploadMediaFile(QStringList filePathList, QString url, QString columnId, QString folderId)
{
    int filePathSize = filePathList.size();
    m_stop = false;
    for (int i = 0; i < filePathSize; i++)
    {
        auto filePath = filePathList[i];
        UploadCanceller* canceller = new UploadCanceller;
        HttpResponse** response = canceller->response();
        m_cancelList.append(canceller);
        auto request = m_api->fileHelper()->uploadFilePreExec(filePath, url, [&, i, filePathSize]
                                                          (qint64 pos, qint64 duration)
                                                              {
                                                                  emit updateMediaProcess(
                                                                      i, filePathSize, pos, duration);
                                                              }, [&, i, filePathSize, filePath, columnId, folderId](
                                                              const QJsonDocument& doc)
                                                              {
                                                                  QJsonObject obj = doc.object();
                                                                  QJsonObject job;
                                                                  job["column_id"] = columnId.toInt();
                                                                  job["source"] = "nle";
                                                                  QJsonArray jsonArray;
                                                                  QJsonObject subJob;
                                                                  subJob["mobject_id"] = obj.value("id").toString();
                                                                  subJob["privacy"] = 1;
                                                                  subJob["name"] = FileHelper::getFileInfo(
                                                                      filePath, FileHelper::FileName);
                                                                  subJob["key_words"];
                                                                  subJob["description"];
                                                                  subJob["folder_id"] = folderId.toInt();
                                                                  subJob["metadata"];
                                                                  jsonArray.append(subJob);
                                                                  job["media"] = jsonArray;
                                                                  //拿到mobjectid 进行移动到指定栏目
                                                                  UploadCanceller* moveCanceller = new UploadCanceller;
                                                                  HttpResponse** moveResponse = moveCanceller->
                                                                      response();
                                                                  m_cancelList.append(moveCanceller);
                                                                  *moveResponse = m_api->requestHelper()->getRequest(
                                                                          QNetworkAccessManager::PostOperation,
                                                                          "api/media/internal/media")
                                                                      .body(job)
                                                                      .onSuccess([&, i, filePathSize, filePath](
                                                                          QNetworkReply* reply)
                                                                          {
                                                                              if (!reply)
                                                                              {
                                                                                  emit mediaFinish(-1, i, filePathSize);
                                                                                  qDebug() << __FUNCTION__ <<
                                                                                      QStringLiteral("移动到指定栏目解析失败");
                                                                                  return;
                                                                              }
                                                                              QJsonParseError errRpt{};
                                                                              auto doc = QJsonDocument::fromJson(
                                                                                  reply->readAll(), &errRpt);
                                                                              if (errRpt.error !=
                                                                                  QJsonParseError::NoError)
                                                                              {
                                                                                  emit mediaFinish(-1, i, filePathSize);
                                                                                  qDebug() << __FUNCTION__ <<
                                                                                      QStringLiteral("移动到指定栏目解析失败");
                                                                              }
                                                                              else
                                                                              {
                                                                                  //拿到素材详细信息 素材id:媒体id
                                                                                  s_map[doc.object().value(
                                                                                          doc.object().keys()[0]).
                                                                                      toString()] = filePath;
                                                                                  emit mediaFinish(0, i, filePathSize);
                                                                              }
                                                                          }).onFailed(
                                                                          [&, moveCanceller](QNetworkReply* reply)
                                                                          {
                                                                              {
                                                                                  QMutexLocker lock(&m_mutex);
                                                                                  m_cancelList.removeOne(moveCanceller);
                                                                                  moveCanceller->deleteLater();
                                                                              }
                                                                              emit mediaFinish(-1, i, filePathSize);
                                                                              qDebug() << __FUNCTION__ <<
                                                                                  QStringLiteral("移动到指定栏目失败");
                                                                          }).onResponse(
                                                                          [&, moveCanceller](QNetworkReply* reply)
                                                                          {
                                                                              QMutexLocker lock(&m_mutex);
                                                                              m_cancelList.removeOne(moveCanceller);
                                                                              moveCanceller->deleteLater();
                                                                          }).timeout(TIMEOUTSECOND).exec();
                                                              }, [&, i, filePathSize, canceller](QNetworkReply* reply)
                                                              {
                                                                  {
                                                                      QMutexLocker lock(&m_mutex);
                                                                      m_cancelList.removeOne(canceller);
                                                                      canceller->deleteLater();
                                                                  }
                                                                  emit mediaFinish(-1, i, filePathSize);
                                                                  qDebug() << __FUNCTION__ << QStringLiteral("上传素材失败");
                                                              }, EMPTY_CALL)
                            .onResponse([&, canceller](QNetworkReply* reply)
                            {
                                QMutexLocker lock(&m_mutex);
                                m_cancelList.removeOne(canceller);
                                canceller->deleteLater();
                            });
        *response = request.exec();
        if (m_stop)
        {
            return -1;
        }
    }
    return 0;
}

bool UploadOperator::uploadMediaFileSync(QStringList filePathList, QString url, QString columnId, QString folderId)
{
    int filePathSize = filePathList.size();
    m_stop = false;
    for (int i = 0; i < filePathSize; i++)
    {
        auto filePath = filePathList[i];
        UploadCanceller* canceller = new UploadCanceller;
        HttpResponse** response = canceller->response();
        m_cancelList.append(canceller);
        //同步的方式上传
        bool uploadFlag = false;
        QString mobjectId;
        QElapsedTimer timer;
        timer.start();
        auto request = m_api->fileHelper()->uploadFilePreExec(filePath, url,
                                                              [&, i, filePathSize](qint64 pos, qint64 duration)
                                                              {
                                                                  emit updateMediaProcess(
                                                                      i, filePathSize, pos, duration);
                                                              }, [&](const QJsonDocument& doc)
                                                              {
                                                                  QJsonObject obj = doc.object();
                                                                  mobjectId = obj.value("id").toString();
                                                                  uploadFlag = true;
                                                              }, [&, canceller](QNetworkReply* reply)
                                                              {
                                                                  {
                                                                      QMutexLocker lock(&m_mutex);
                                                                      m_cancelList.removeOne(canceller);
                                                                      canceller->deleteLater();
                                                                  }
                                                                  uploadFlag = false;
                                                              }, EMPTY_CALL)
                            .onResponse(
                                [&, canceller](QNetworkReply* reply)
                                {
                                    QMutexLocker lock(&m_mutex);
                                    m_cancelList.removeOne(canceller);
                                    canceller->deleteLater();
                                });
        BLOCK(*response, request.exec());
        qDebug() << "upload" << filePathList[i] << "spend" << timer.elapsed() / 1000.0 << "s";
        if (!uploadFlag)
        {
            qDebug() << filePathList[i] << "upload faild";
            return false;
        }
        if (m_stop)
        {
            qDebug() << "upload " << filePathList[i] << " stop";
            return false;
        }
        //拿到mobjectid 进行移动到指定栏目
        UploadCanceller* moveCanceller = new UploadCanceller;
        HttpResponse** moveResponse = moveCanceller->response();
        m_cancelList.append(moveCanceller);
        QJsonObject job;
        job["column_id"] = columnId.toInt();
        job["source"] = "nle";
        QJsonArray jsonArray;
        QJsonObject subJob;
        subJob["mobject_id"] = mobjectId;
        subJob["privacy"] = 1;
        subJob["name"] = FileHelper::getFileInfo(filePath, FileHelper::FileName);
        subJob["key_words"];
        subJob["description"];
        subJob["folder_id"] = folderId.toInt();
        subJob["metadata"];
        jsonArray.append(subJob);
        job["media"] = jsonArray;
        bool moveFlag = false;
        timer.restart();
        auto moveRequest = m_api->requestHelper()->getRequest(QNetworkAccessManager::PostOperation,
                                                              "api/media/internal/media")
                                .body(job)
                                .onSuccess([&](QNetworkReply* reply)
                                {
                                    if (!reply)
                                    {
                                        qDebug() << __FUNCTION__ << QStringLiteral("移动到指定栏目解析失败");
                                        return;
                                    }
                                    QJsonParseError errRpt{};
                                    auto doc = QJsonDocument::fromJson(reply->readAll(), &errRpt);
                                    if (errRpt.error != QJsonParseError::NoError)
                                    {
                                        moveFlag = false;
                                    }
                                    else
                                    {
                                        s_map[mobjectId] = filePath;
                                        moveFlag = true;
                                    }
                                })
                                .onFailed([&, moveCanceller](QNetworkReply* reply)
                                {
                                    {
                                        QMutexLocker lock(&m_mutex);
                                        m_cancelList.removeOne(moveCanceller);
                                        moveCanceller->deleteLater();
                                    }
                                    moveFlag = false;
                                })
                                .onResponse([&, moveCanceller](QNetworkReply* reply)
                                {
                                    QMutexLocker lock(&m_mutex);
                                    m_cancelList.removeOne(moveCanceller);
                                    moveCanceller->deleteLater();
                                }).timeout(TIMEOUTSECOND);
        BLOCK(*moveResponse, moveRequest.exec());
        qDebug() << "upload move" << filePathList[i] << "spend" << timer.elapsed() / 1000.0 << "s";
        if (!moveFlag)
        {
            qDebug() << mobjectId << "move to column " << columnId << " folder " << folderId << " faild";
            return false;
        }
        if (m_stop)
        {
            return false;
        }
    }
    return true;
}
