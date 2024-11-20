//
// Created by 58226 on 2024/11/14.
//

#ifndef CLOUMNMODEL_H
#define CLOUMNMODEL_H

#include <QAbstractItemModel>

#include "requestbase.h"

#include <QStyledItemDelegate>

class UploadColumnModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit UploadColumnModel(QObject *parent = nullptr);

    // QAbstractItemModel interface
public:
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(const QString &columnId);

    public slots:
        void onPresonalColumns();//个人素材栏目
    void onPresonalUploadColumns();//个人上传素材栏目(除去”我的分享“和“我的收藏”)
    void onPublicColumns(QList<QSharedPointer<SubRequests::ColumnBaseInfo>> colList);//公共素材栏目
    void onAllColumns(QList<QSharedPointer<SubRequests::ColumnBaseInfo>> colList);//所有栏目
    void onUploadColumns(QList<QSharedPointer<SubRequests::ColumnBaseInfo>> colList);//能够上传栏目(除去”我的分享“和“我的收藏”)

private:
    QSharedPointer<SubRequests::ColumnBaseInfo> findColumn(QSharedPointer<SubRequests::ColumnBaseInfo> column, QString columnId, int &row) const;

private:
    QSharedPointer<SubRequests::ColumnBaseInfo> m_rootColumn;
};


class UploadColumnDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit UploadColumnDelegate(QObject *parent = nullptr);


    // QAbstractItemDelegate interface
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};


#endif //CLOUMNMODEL_H
