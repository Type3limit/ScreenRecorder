#ifndef NETWORKCOLUMNANDFOKLERWIDGET_H
#define NETWORKCOLUMNANDFOKLERWIDGET_H

#include "materiallibhelper.h"
#include <QItemSelection>
#include <QWidget>

#include "onlineservice.h"

namespace Ui {
class NetworkColumnAndFoklerWidget;
}

class UploadColumnDelegate;
class UploadOperator;
class UploadColumnModel;
class UploadFolderModel;

Q_DECLARE_METATYPE(MaterialLibHelper::MaterialType)

class NetworkColumnAndFoklerWidget : public QWidget
{
    Q_OBJECT

public:
    enum ColumnType {
        ColumnType_UploadMaterial,  //显示上传素材栏目
        ColumnType_UploadProject,   //显示上传工程栏目
        ColumnType_CheckProject,    //显示查看工程栏目
        ColumnType_NewProject,      //显示新建工程栏目
        ColumnType_NewSeriesProject, //显示新建串联单栏目
        ColumnType_NewProjectWithName      //显示新建工程栏目显示栏目名称
    };

    explicit NetworkColumnAndFoklerWidget(QWidget *parent = nullptr);
    ~NetworkColumnAndFoklerWidget();

    void setApiInstance(OnlineService* apiInstance);

    void setColumnType(ColumnType type);

    void setProjectName(const QString &text);


    void hideFilterWidget(bool hide);

    UploadOperator* apiOperator();

    QString projectName();

    QString columnId() const;
    QString columnName() const;
    QString folderId() const;
    QString folderName() const;
    QString path() const;

    void setColumnIdAndFolderId(const QString& columnId, const QString& folderId);

    QModelIndex currentIndex();

    QVariant data();

public:
    void init();

signals:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void clicked(const QModelIndex &index);
    void doubleClicked(const QModelIndex &index);

    // QWidget interface
protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void initUi();
    void initConnect();
    void switchColumn(const QString &columnId);

private slots:
    void onDirsChanged();

private:
    Ui::NetworkColumnAndFoklerWidget *ui;

    ColumnType m_type;

    UploadColumnDelegate *m_delegate;
    UploadOperator *m_uploadOperator;
    UploadColumnModel *m_presonalModel;
    UploadColumnModel *m_publicModel;
    UploadFolderModel *m_folderModel;

    bool m_inited = false;
};

#endif // NETWORKCOLUMNANDFOKLERWIDGET_H
