//
// Created by 58226 on 2024/11/15.
//

#include "foldermodel.h"
#include <QDebug>
#include <QJsonObject>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QEvent>
#include <QPainter>
#include <QTextDocument>

int FolderData::read(const QJsonObject& obj)
{
    type = obj.value("type").toInt();
    folder_id = QString::number(obj.value("folder_id").isNull() ? 0 : obj.value("folder_id").toInt());
    material_id = QString::number(obj.value("id").toInt());
    mobject_id = obj.value("mobject_id").toString();
    mobject_url = obj.value("mobject_url").toString();
    column_id = QString::number(obj.value("column_id").toInt());;
    name = obj.value("name").toString().remove("<em>").remove("</em>");
    column_name = obj.value("column_name").toString();
    creator_nickname = obj.value("creator_nickname").toString();
    secondsDuration = obj.value("duration").toInt();
    duration = QTime(0, 0, 0).addSecs(obj.value("duration").toInt()).toString("hh:mm:ss");
    quality = obj.value("quality_type").toInt();
    if (type == -1)
    {
        qualityName = QStringLiteral("文件夹");
    }
    else if (type == 5)
    {
        if (quality == 1)
        {
            qualityName = QStringLiteral("标清");
        }
        else if (quality == 2)
        {
            qualityName = QStringLiteral("高清");
        }
        else if (quality == 3)
        {
            qualityName = QStringLiteral("4K");
        }
        else if (quality == 4)
        {
            qualityName = QStringLiteral("8K");
        }
        else
        {
            qualityName = QStringLiteral("未知");
        }
    }
    cover_mobject_id = obj.value("cover_mobject_id").toString();
    creator_time = QDateTime::fromTime_t(obj.value("creation_time").toInt()).toString("yyyy.MM.dd hh:mm");
    creator_timestamp = obj.value("creation_time").toVariant().toLongLong();
    last_modification_time = QDateTime::fromTime_t(obj.value("last_modification_time").toInt()).toString(
        "yyyy.MM.dd hh:mm");
    last_modification_timestamp = obj.value("last_modification_time").toVariant().toLongLong();
    id_path = obj.value("id_path").toString().split("/", Qt::SkipEmptyParts);
    name_path = obj.value("name_path").toString().split("/", Qt::SkipEmptyParts);
    sys_type = obj.value("sys_type").toInt();
    if (!obj.value("file_metadata").isNull())
    {
        auto metaObj = obj.value("file_metadata").toObject();
        nle_height = metaObj.value("nle_height").toInt();
        nle_width = metaObj.value("nle_width").toInt();
    }
    isCollected = obj.value("collection").toBool(false);
    return 0;
}

UploadFolderModel::UploadFolderModel(QObject* parent)
    : QAbstractTableModel{parent}
    , m_folderImage(":/icons/image/folder.svg")
{
}

int UploadFolderModel::rowCount(const QModelIndex& parent) const
{
    return m_folders.size();
}

int UploadFolderModel::columnCount(const QModelIndex& parent) const
{
    return 7;
}

QVariant UploadFolderModel::data(const QModelIndex& index, int role) const
{
    switch (role)
    {
    case Qt::DisplayRole:
        {
            if (index.column() == 0)
            {
                //名称
                return QVariant(m_folders[index.row()].name);
            }
            else if (index.column() == 1)
            {
                //所属栏目
                return QVariant(m_folders[index.row()].column_name);
            }
            else if (index.column() == 2)
            {
                //创建人
                return QVariant(m_folders[index.row()].creator_nickname);
            }
            else if (index.column() == 3)
            {
                //时长
                return QVariant(m_folders[index.row()].duration);
            }
            else if (index.column() == 4)
            {
                //类型
                return QVariant(m_folders[index.row()].qualityName);
            }
            else if (index.column() == 5)
            {
                //创建时间
                return QVariant(m_folders[index.row()].creator_time);
            }
            else if (index.column() == 6)
            {
                //最后修改时间
                return QVariant(m_folders[index.row()].last_modification_time);
            }
        }
    case Qt::DecorationRole:
        {
            if (index.column() == 0 && m_folders[index.row()].type == -1)
            {
                //文件夹类型
                return QVariant(m_folderImage);
            }
            else if (index.column() == 0 && m_folders[index.row()].sys_type == 2)
            {
                //aigc
                return QVariant(m_titleImage);
            }
            else
            {
                return QVariant();
            }
        }
    case Qt::ToolTipRole:
        {
            if (index.column() == 0)
            {
                //名称
                return QVariant(m_folders[index.row()].name);
            }
            else if (index.column() == 1)
            {
                //所属栏目
                return QVariant(m_folders[index.row()].column_name);
            }
            else if (index.column() == 2)
            {
                //创建人
                return QVariant(m_folders[index.row()].creator_nickname);
            }
            else if (index.column() == 3)
            {
                //时长
                return QVariant(m_folders[index.row()].duration);
            }
            else if (index.column() == 4)
            {
                //类型
                return QVariant(m_folders[index.row()].qualityName);
            }
            else if (index.column() == 5)
            {
                //创建时间
                return QVariant(m_folders[index.row()].creator_time);
            }
            else if (index.column() == 6)
            {
                //最后修改时间
                return QVariant(m_folders[index.row()].last_modification_time);
            }
        }
    case Qt::UserRole:
        {
            return QVariant::fromValue(m_folders[index.row()]);
        }
    case Qt::UserRole + 1:
        {
            return QVariant(m_folders[index.row()].material_id);
        }
    case Qt::UserRole + 2:
        {
            return QVariant(m_folders[index.row()].column_id);
        }
    case Qt::UserRole + 3:
        {
            return QVariant(m_folders[index.row()].folder_id);
        }
    case Qt::UserRole + 4:
        {
            return QVariant(m_folders[index.row()].child.isEmpty());
        }
    case Qt::UserRole + 5:
        {
            return QVariant(m_folders[index.row()].type);
        }
    case Qt::UserRole + 6:
        {
            return QVariant(m_folders[index.row()].mobject_url);
        }
    case Qt::UserRole + 7:
        {
            return QVariant(m_folders[index.row()].name);
        }
    }
    return QVariant();
}

bool UploadFolderModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
{
    if (role == Qt::DisplayRole)
    {
        m_nameList.insert(section, value.toString());
    }
    else if (role == Qt::SizeHintRole)
    {
        m_sizeList.insert(section, value.toSize());
    }
    return true;
}

QVariant UploadFolderModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        return m_nameList[section];
    }
    else if (role == Qt::SizeHintRole)
    {
        return m_sizeList[section];
    }
    return QVariant();
}

void UploadFolderModel::fetchMore(const QModelIndex& parent)
{
    if (m_folders.size() < m_total)
    {
        emit getMoreData(m_folders.size() / 30 + 1, 30);
    }
}

bool UploadFolderModel::canFetchMore(const QModelIndex& parent) const
{
    //    if(m_folders.size() < m_total) {
    //        return true;
    //    }else {
    return false;
    //    }
}

void UploadFolderModel::onFolder(int total, QList<FolderData> children)
{
    m_total = total;
    if (!children.isEmpty())
    {
        beginInsertRows(QModelIndex(), m_folders.size(), m_folders.size() + children.size() - 1);
        m_folders.append(children);
        endInsertRows();
    }
}

void UploadFolderModel::onFolderSync(QPair<int, QList<FolderData>> data)
{
    m_total = data.first;
    if (!data.second.isEmpty())
    {
        beginInsertRows(QModelIndex(), m_folders.size(), m_folders.size() + data.second.size() - 1);
        m_folders.append(data.second);
        endInsertRows();
    }
}

void UploadFolderModel::onCover(const QString& id, const QString& coverPath)
{
    beginResetModel();
    for (int i = 0; i < m_folders.size(); i++)
    {
        if (m_folders[i].material_id == id)
        {
            m_folders[i].coverPath = coverPath;
            break;
        }
    }
    endResetModel();
}

void UploadFolderModel::clearData()
{
    m_total = 0;
    beginResetModel();
    m_folders.clear();
    endResetModel();
}
FolderAndProjectDelegate::FolderAndProjectDelegate(QObject *parent)
    : QStyledItemDelegate{parent}
{

}

void FolderAndProjectDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    //画分割线
    painter->setPen(QColor("#17181A"));
    painter->drawLine(QLine(option.rect.bottomLeft(), option.rect.bottomRight()));
}

bool FolderAndProjectDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if(event->type() == QEvent::HoverMove) {

    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
