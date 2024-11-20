//
// Created by 58226 on 2024/11/14.
//

#include "cloumnmodel.h"
UploadColumnModel::UploadColumnModel(QObject *parent)
    : QAbstractItemModel{parent},
      m_rootColumn(new SubRequests::ColumnBaseInfo)
{
}

QModelIndex UploadColumnModel::index(int row, int column, const QModelIndex &parent) const
{
    if(parent == QModelIndex()) {
        return createIndex(row, column, m_rootColumn->child[row]->id.toULongLong());
    }else {
        int columnRow = 0;
        auto col = findColumn(m_rootColumn, QString::number(parent.internalId()), columnRow);

        return createIndex(row, column, col->child[row]->id.toULongLong());
    }
}

QModelIndex UploadColumnModel::parent(const QModelIndex &child) const
{
    int row = 0;
    auto childColumn = findColumn(m_rootColumn, QString::number(child.internalId()), row);
    auto parentColumn = findColumn(m_rootColumn, QString::number(childColumn->parent_id.toLongLong()), row);
    if(parentColumn->id.toULongLong() != 0) {
        return createIndex(row, 0, parentColumn->id.toULongLong());
    }else {
        return QModelIndex();
    }
}

int UploadColumnModel::rowCount(const QModelIndex &parent) const
{
    if(parent == QModelIndex()) {
        return m_rootColumn->child.size();
    }else {
        int row = 0;
        auto column = findColumn(m_rootColumn, QString::number(parent.internalId()), row);
        return column->child.size();
    }
}

int UploadColumnModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant UploadColumnModel::data(const QModelIndex &index, int role) const
{
    switch(role)
    {
        case Qt::DisplayRole:
        {
            int row = 0;
            auto column = findColumn(m_rootColumn, QString::number(index.internalId()), row);
            return QVariant(column->name);
        }
        case Qt::UserRole + 1:
        {
            int row = 0;
            auto column = findColumn(m_rootColumn, QString::number(index.internalId()), row);
            return QVariant(column->id);
        }
        case Qt::UserRole + 2:
        {
            int row = 0;
            auto column = findColumn(m_rootColumn, QString::number(index.internalId()), row);
            return QVariant(column->parent_id);
        }
        case Qt::UserRole + 3:
        {
            int row = 0;
            auto column = findColumn(m_rootColumn, QString::number(index.internalId()), row);
            return QVariant(column->has_permission);
        }
        case Qt::UserRole + 4:
        {
            int row = 0;
            auto column = findColumn(m_rootColumn, QString::number(index.internalId()), row);
            return QVariant(column->nle_config.enabled);
        }
        case Qt::UserRole + 5:
        {
            int row = 0;
            auto column = findColumn(m_rootColumn, QString::number(index.internalId()), row);
            return QVariant(column->nle_config.limit_folder);
        }
    }
    return QVariant();
}

QModelIndex UploadColumnModel::index(const QString &columnId)
{
    int row = 0;
    auto column = findColumn(m_rootColumn, columnId, row);
    if(column) {
        return createIndex(row, 0, columnId.toULongLong());
    }else {
        return QModelIndex();
    }
}

void UploadColumnModel::onPresonalColumns()
{
    beginResetModel();
    m_rootColumn->id = "-1";
    m_rootColumn->name = QStringLiteral("个人素材");
    m_rootColumn->parent_id = "";
    auto columns1 = QSharedPointer<SubRequests::ColumnBaseInfo>(new SubRequests::ColumnBaseInfo);
    columns1->id = "0";
    columns1->name = QStringLiteral("我的素材");
    columns1->parent_id = "-1";
    auto columns2 = QSharedPointer<SubRequests::ColumnBaseInfo>(new SubRequests::ColumnBaseInfo);
    columns2->id = "1";
    columns2->name = QStringLiteral("我的分享");
    columns2->parent_id = "-1";
    auto columns3 = QSharedPointer<SubRequests::ColumnBaseInfo>(new SubRequests::ColumnBaseInfo);
    columns3->id = "2";
    columns3->name = QStringLiteral("我的收藏");
    columns3->parent_id = "-1";
    QList<QSharedPointer<SubRequests::ColumnBaseInfo>> colList;
    colList << columns1 << columns2 << columns3;
    m_rootColumn->child = colList;
    endResetModel();
}

void UploadColumnModel::onPresonalUploadColumns()
{
    beginResetModel();
    m_rootColumn->id = "-1";
    m_rootColumn->name = QStringLiteral("个人素材");
    m_rootColumn->parent_id = "";
    auto columns1 = QSharedPointer<SubRequests::ColumnBaseInfo>(new SubRequests::ColumnBaseInfo);
    columns1->id = "0";
    columns1->name = QStringLiteral("我的素材");
    columns1->parent_id = "-1";
    QList<QSharedPointer<SubRequests::ColumnBaseInfo>> colList;
    colList << columns1;
    m_rootColumn->child = colList;
    endResetModel();
}

void UploadColumnModel::onPublicColumns(QList<QSharedPointer<SubRequests::ColumnBaseInfo> > colList)
{
    beginResetModel();
    m_rootColumn->id = "0";
    m_rootColumn->name = QStringLiteral("公共素材");
    m_rootColumn->parent_id = "-1";
    m_rootColumn->child = colList;
    endResetModel();
}

void UploadColumnModel::onAllColumns(QList<QSharedPointer<SubRequests::ColumnBaseInfo> > colList)
{
    beginResetModel();
    m_rootColumn->id = "-1";
    auto columns1 = QSharedPointer<SubRequests::ColumnBaseInfo>(new SubRequests::ColumnBaseInfo);
    columns1->id = "0";
    columns1->name = QStringLiteral("我的素材");
    columns1->parent_id = "-1";
    auto columns2 = QSharedPointer<SubRequests::ColumnBaseInfo>(new SubRequests::ColumnBaseInfo);
    columns2->id = "1";
    columns2->name = QStringLiteral("我的分享");
    columns2->parent_id = "-1";
    auto columns3 = QSharedPointer<SubRequests::ColumnBaseInfo>(new SubRequests::ColumnBaseInfo);
    columns3->id = "2";
    columns3->name = QStringLiteral("我的收藏");
    columns3->parent_id = "-1";
    QList<QSharedPointer<SubRequests::ColumnBaseInfo>> newColList;
    newColList << columns1 << columns2 << columns3 << colList;
    m_rootColumn->child = newColList;
    endResetModel();
}

void UploadColumnModel::onUploadColumns(QList<QSharedPointer<SubRequests::ColumnBaseInfo> > colList)
{
    beginResetModel();
    m_rootColumn->id = "-1";
    auto columns1 = QSharedPointer<SubRequests::ColumnBaseInfo>(new SubRequests::ColumnBaseInfo);
    columns1->id = "0";
    columns1->name = QStringLiteral("我的素材");
    columns1->parent_id = "-1";
    QList<QSharedPointer<SubRequests::ColumnBaseInfo>> newColList;
    newColList << columns1 << colList;
    m_rootColumn->child = newColList;
    endResetModel();
}

QSharedPointer<SubRequests::ColumnBaseInfo> UploadColumnModel::findColumn(QSharedPointer<SubRequests::ColumnBaseInfo> column, QString columnId, int &row) const
{
    if(column->id == columnId) {
        return column;
    }
    for (int i = 0; i < column->child.size(); i++) {
        row = i;
        auto resColumn = findColumn(column->child[i], columnId, row);
        if(resColumn) {
            return resColumn;
        }
    }
    return nullptr;
}

UploadColumnDelegate::UploadColumnDelegate(QObject *parent)
    : QStyledItemDelegate{parent}
{
}

void UploadColumnDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    bool has_permission = index.data(Qt::UserRole + 3).toBool();
    if(has_permission) {
        QStyledItemDelegate::paint(painter, option, index);
    }else {
        QStyleOptionViewItem opt(option);
        opt.palette.setColor(QPalette::Text, QColor("#99A1B0"));
        opt.palette.setBrush(QPalette::Text, QColor("#99A1B0"));
        QStyledItemDelegate::paint(painter, opt, index);
    }
}