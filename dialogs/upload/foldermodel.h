//
// Created by 58226 on 2024/11/15.
//

#ifndef FOLDERMODEL_H
#define FOLDERMODEL_H
#include <QAbstractItemModel>
#include <QPixmap>
#include <QStyledItemDelegate>
#include <qstyleoption.h>

struct FolderData {
    FolderData(){}

    int read(const QJsonObject& obj);


    int type;
    QString folder_id;  //所在文件夹id
    QString column_id;  //栏目id
    QString material_id;  //素材id
    QString mobject_id; //媒体id
    QString mobject_url;//媒体路径
    QString name;       //媒体名称
    QString column_name;//栏目名称
    QString creator_nickname;//创建人
    int secondsDuration;    //时长（秒）
    QString duration;    //时长（格式化后的文本内容）
    int quality;            //质量
    QString qualityName;   //类型名
    QString cover_mobject_id;
    QString coverPath;  //封面路径（下载过后的本地路径）
    QString creator_time;//创建时间
    qint64 creator_timestamp;//创建时间戳
    QString last_modification_time;//最后修改时间
    qint64 last_modification_timestamp;//最后修改时间戳
    QStringList id_path;        //id路径（/0/123456/ 注：根目录无）
    QStringList name_path;      //名称路径（/adbc 注：根目录无，比id_path少一个）
    int sys_type;       //0-素材库，1-非编生成，2-aigc
    int nle_height;
    int nle_width;
    QList<FolderData> child;
    bool isCollected;
};

Q_DECLARE_METATYPE(FolderData)

class UploadFolderModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit UploadFolderModel(QObject *parent = nullptr);

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void fetchMore(const QModelIndex &parent) override;
    bool canFetchMore(const QModelIndex &parent) const override;

    signals:
        void getMoreData(int page, int size);

    public slots:
        void onFolder(int total, QList<FolderData> children);
    void onFolderSync(QPair<int, QList<FolderData> > data);
    void onCover(const QString &id, const QString &coverPath);
    void clearData();

private:
    QPixmap m_folderImage;
    QPixmap m_projectImage;
    QPixmap m_titleImage;
    int m_total;
    QList<FolderData> m_folders;
    QStringList m_nameList;
    QList<QSize> m_sizeList;
};

class FolderAndProjectDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit FolderAndProjectDelegate(QObject *parent = nullptr);

    // QAbstractItemDelegate interface
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;

private:

};
#endif //FOLDERMODEL_H
