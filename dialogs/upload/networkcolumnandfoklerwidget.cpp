#include "networkcolumnandfoklerwidget.h"
#include "qaction.h"
#include "qpainter.h"
#include "qscrollbar.h"
#include "ui_networkcolumnandfoklerwidget.h"
#include "cloumnmodel.h"
#include "foldermodel.h"
#include "uploadoperator.h"

#include <QEvent>

NetworkColumnAndFoklerWidget::NetworkColumnAndFoklerWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::NetworkColumnAndFoklerWidget)
    , m_uploadOperator(nullptr)
{
    ui->setupUi(this);
}

NetworkColumnAndFoklerWidget::~NetworkColumnAndFoklerWidget()
{
    delete ui;
}

void NetworkColumnAndFoklerWidget::setApiInstance(QSharedPointer<OnlineService> apiInstance)
{
    m_uploadOperator = new UploadOperator(apiInstance, this);
}

void NetworkColumnAndFoklerWidget::setColumnType(ColumnType type)
{
    m_type = type;
    if (m_type == ColumnType_UploadMaterial)
    {
        //无分享收藏，只有文件夹
        ui->m_filePathWidget->setDirs(QStringList() << QStringLiteral("我的素材"));
        QMap<int, QVariant> map;
        map[Qt::UserRole] = "0"; //columnId
        map[Qt::UserRole + 1] = "0"; //folderId
        ui->m_filePathWidget->setDatas(QList<QMap<int, QVariant>>() << map);

        ui->m_typeComboBox->setCurrentIndex(2);
        ui->m_typeComboBox->hide();
        ui->m_projectNameLabel->hide();
        ui->m_projectNameLineEdit->hide();
        ui->m_presonalMaterialView->setFixedHeight(37);
        m_presonalModel->onPresonalUploadColumns();
    }
    else if (m_type == ColumnType_UploadProject)
    {
        //无分享收藏，只有文件夹

        ui->m_filePathWidget->setDirs(QStringList() << QStringLiteral("我的素材"));
        QMap<int, QVariant> map;
        map[Qt::UserRole] = "0"; //columnId
        map[Qt::UserRole + 1] = "0"; //folderId
        ui->m_filePathWidget->setDatas(QList<QMap<int, QVariant>>() << map);


        ui->m_typeComboBox->setCurrentIndex(2);
        ui->m_typeComboBox->hide();
        ui->m_projectNameLabel->show();
        ui->m_projectNameLineEdit->show();
        ui->m_presonalMaterialView->setFixedHeight(37);
        m_presonalModel->onPresonalUploadColumns();
    }
    else if (m_type == ColumnType_CheckProject)
    {
        ui->m_filePathWidget->setDirs(QStringList() << QStringLiteral("公共素材"));
        QMap<int, QVariant> map;
        map[Qt::UserRole] = "-1"; //columnId
        map[Qt::UserRole + 1] = "0"; //folderId
        ui->m_filePathWidget->setDatas(QList<QMap<int, QVariant>>() << map);


        ui->m_typeComboBox->setCurrentIndex(1);
        ui->m_typeComboBox->show();
        ui->m_projectNameLabel->hide();
        ui->m_projectNameLineEdit->hide();
        ui->m_presonalMaterialView->setFixedHeight(93);
        m_presonalModel->onPresonalColumns();
    }
    else if (m_type == ColumnType_NewProject)
    {
        ui->m_filePathWidget->setDirs(QStringList() << QStringLiteral("我的素材"));
        QMap<int, QVariant> map;
        map[Qt::UserRole] = "0"; //columnId
        map[Qt::UserRole + 1] = "0"; //folderId
        ui->m_filePathWidget->setDatas(QList<QMap<int, QVariant>>() << map);


        ui->m_typeComboBox->setCurrentIndex(2);
        ui->m_typeComboBox->hide();
        ui->m_projectNameLabel->hide();
        ui->m_projectNameLineEdit->hide();
        ui->m_presonalMaterialView->setFixedHeight(37);
        m_presonalModel->onPresonalUploadColumns();
    }
    else if (m_type == ColumnType_NewSeriesProject)
    {
        //        ui->m_filePathWidget->setDirs(QStringList() << QStringLiteral("我的素材"));
        //        QMap<int, QVariant> map;
        //        map[Qt::UserRole] = "0";//columnId
        //        map[Qt::UserRole + 1] = "0";//folderId
        //        ui->m_filePathWidget->setDatas(QList<QMap<int, QVariant>>() << map);

        ui->m_presonalMaterialButton->hide();
        ui->m_presonalMaterialView->hide();

        ui->m_typeComboBox->setCurrentIndex(2);
        ui->m_typeComboBox->hide();
        ui->m_projectNameLabel->hide();
        ui->m_projectNameLineEdit->hide();
        //        ui->m_presonalMaterialView->setFixedHeight(37);
        //        m_presonalModel->onPresonalUploadColumns();
    }
    else if (m_type == ColumnType_NewProjectWithName)
    {
        ui->m_filePathWidget->setDirs(QStringList() << QStringLiteral("我的素材"));
        QMap<int, QVariant> map;
        map[Qt::UserRole] = "0"; //columnId
        map[Qt::UserRole + 1] = "0"; //folderId
        ui->m_filePathWidget->setDatas(QList<QMap<int, QVariant>>() << map);


        ui->m_typeComboBox->setCurrentIndex(2);
        ui->m_typeComboBox->hide();
        ui->m_projectNameLabel->show();
        ui->m_projectNameLineEdit->show();
        ui->m_presonalMaterialView->setFixedHeight(37);
        m_presonalModel->onPresonalUploadColumns();
    }
}

void NetworkColumnAndFoklerWidget::setProjectName(const QString& text)
{
    ui->m_projectNameLineEdit->setText(text);
}

void NetworkColumnAndFoklerWidget::hideFilterWidget(bool hide)
{
    ui->m_fliterWidget->setHidden(hide);
}

UploadOperator* NetworkColumnAndFoklerWidget::apiOperator()
{
    return m_uploadOperator;
}

QString NetworkColumnAndFoklerWidget::projectName()
{
    return ui->m_projectNameLineEdit->text();
}

QString NetworkColumnAndFoklerWidget::columnId() const
{
    return ui->m_filePathWidget->data(ui->m_filePathWidget->dirs().size() - 1, Qt::UserRole).toString();
}

QString NetworkColumnAndFoklerWidget::columnName() const
{
    return ui->m_filePathWidget->dirs().first();
}

QString NetworkColumnAndFoklerWidget::folderId() const
{
    return m_currentHighlightedFolderId.isEmpty()?
        ui->m_filePathWidget->data(ui->m_filePathWidget->dirs().size() - 1, Qt::UserRole + 1).toString()
            :m_currentHighlightedFolderId;
}

QString NetworkColumnAndFoklerWidget::folderName() const
{
    return ui->m_filePathWidget->dirs().last();
}

QString NetworkColumnAndFoklerWidget::path() const
{
    return ui->m_filePathWidget->dirs().join("/");
}

void NetworkColumnAndFoklerWidget::setColumnIdAndFolderId(const QString& columnId, const QString& folderId)
{
    if (columnId != "-1" && folderId == "0")
    {
        bool enabled;
        int limit_folder;
        bool has_permission;
        QString name;
        if (columnId == "0")
        {
            enabled = true;
            limit_folder = 0;
            has_permission = true;
            name = QStringLiteral("我的素材");
        }
        else
        {
            auto columns = m_uploadOperator->getColumnDetails(m_type == ColumnType_UploadMaterial, {columnId});
            if (columns.isEmpty())
            {
                return;
            }
            enabled = columns[0]->nle_config.enabled;
            limit_folder = columns[0]->nle_config.limit_folder;
            has_permission = columns[0]->has_permission;
            name = columns[0]->name;
        }
        QMap<int, QVariant> map;
        map[Qt::UserRole] = columnId;
        map[Qt::UserRole + 1] = folderId;
        map[Qt::UserRole + 2] = enabled; //nleEnabled
        map[Qt::UserRole + 3] = limit_folder; //limit_folder
        map[Qt::UserRole + 4] = has_permission; //has_permission
        ui->m_filePathWidget->setDirs({name});
        ui->m_filePathWidget->setDatas({map});
    }
    else if (columnId != "-1")
    {
        auto data = m_uploadOperator->getFolderDetails(folderId);
        bool enabled;
        int limit_folder;
        bool has_permission;
        if (columnId == "0")
        {
            enabled = true;
            limit_folder = 0;
            has_permission = true;
        }
        else
        {
            auto columns = m_uploadOperator->getColumnDetails(m_type == ColumnType_UploadMaterial, {columnId});
            if (columns.isEmpty())
            {
                return;
            }
            enabled = columns[0]->nle_config.enabled;
            limit_folder = columns[0]->nle_config.limit_folder;
            has_permission = columns[0]->has_permission;
        }
        if (columnId == "0")
        {
            data.name_path.prepend(QStringLiteral("我的素材"));
        }
        else
        {
            data.name_path.prepend(data.column_name);
        }
        if (data.id_path.isEmpty())
        {
            data.id_path.prepend("0");
        }
        data.name_path.append(data.name);
        data.id_path.append(data.folder_id);
        QList<QMap<int, QVariant>> dataList;
        for (int i = 0; i < data.name_path.size(); i++)
        {
            QMap<int, QVariant> map;
            map[Qt::UserRole] = columnId;
            map[Qt::UserRole + 1] = data.id_path[i];
            dataList.append(map);
        }
        dataList[0][Qt::UserRole + 2] = enabled;
        dataList[0][Qt::UserRole + 3] = limit_folder;
        dataList[0][Qt::UserRole + 4] = has_permission;
        ui->m_filePathWidget->setDirs(data.name_path);
        ui->m_filePathWidget->setDatas(dataList);
    }
    m_currentHighlightedFolderId = folderId;
}

QModelIndex NetworkColumnAndFoklerWidget::currentIndex()
{
    return ui->m_fileTreeView->currentIndex();
}

QVariant NetworkColumnAndFoklerWidget::data()
{
    return ui->m_fileTreeView->currentIndex().data(Qt::UserRole);
}

void NetworkColumnAndFoklerWidget::init()
{
    initUi();
    initConnect();

    m_inited = true;
}

void NetworkColumnAndFoklerWidget::paintEvent(QPaintEvent* event)
{
    QStyleOption option;
    option.init(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
}

void NetworkColumnAndFoklerWidget::closeEvent(QCloseEvent* event)
{
    //    ui->m_presonalMaterialButton->setChecked(true);
}

void NetworkColumnAndFoklerWidget::showEvent(QShowEvent* event)
{
    if (m_inited)
    {
        m_folderModel->clearData();
        if (m_type == ColumnType_UploadMaterial)
        {
            //无分享收藏，只有文件夹
            m_uploadOperator->getAllAuthorizedColumns(m_type == ColumnType_UploadMaterial, MaterialLibHelper::Upload);
        }
        else if (m_type == ColumnType_UploadProject)
        {
            m_uploadOperator->getAllAuthorizedColumns(m_type == ColumnType_UploadMaterial,
                                                      MaterialLibHelper::AddNleProject, true);
        }
        else if (m_type == ColumnType_CheckProject)
        {
            m_uploadOperator->getAllAuthorizedColumns(m_type == ColumnType_UploadMaterial,
                                                      MaterialLibHelper::ViewNleProject, true);
        }
        else if (m_type == ColumnType_NewProject || m_type == ColumnType_NewProjectWithName)
        {
            m_uploadOperator->getAllAuthorizedColumns(m_type == ColumnType_UploadMaterial,
                                                      MaterialLibHelper::AddNleProject, true);
        }
        else if (m_type == ColumnType_NewSeriesProject)
        {
            m_uploadOperator->getAllAuthorizedColumns(m_type == ColumnType_UploadMaterial,
                                                      MaterialLibHelper::AddNleListProject, true);
        }
    }
}

void NetworkColumnAndFoklerWidget::initUi()
{
    ui->m_presonalMaterialView->hide();
    ui->m_publicMaterialView->hide();

    m_delegate = new UploadColumnDelegate(this);
    m_presonalModel = new UploadColumnModel(this);
    m_publicModel = new UploadColumnModel(this);
    ui->m_presonalMaterialView->setModel(m_presonalModel);
    ui->m_publicMaterialView->setModel(m_publicModel);
    ui->m_publicMaterialView->setItemDelegate(m_delegate);

    auto searchAction = new QAction(this);
    searchAction->setIcon(QIcon(":/navigation/icons/navigation/systemset_search_icon.svg"));
    ui->m_searchLineEdit->addAction(searchAction, QLineEdit::LeadingPosition);

    m_folderModel = new UploadFolderModel(this);
    QStringList nameList;
    nameList << QStringLiteral("工程名/文件夹名")
        << QStringLiteral("所属栏目")
        << QStringLiteral("创建人")
        << QStringLiteral("时长")
        << QStringLiteral("类型")
        << QStringLiteral("创建时间")
        << QStringLiteral("最后修改时间");
    QList<QSize> sizeList;
    sizeList << QSize(225, 34)
        << QSize(70, 34)
        << QSize(70, 34)
        << QSize(60, 34)
        << QSize(55, 34)
        << QSize(115, 34)
        << QSize(115, 34);
    for (int i = 0; i < nameList.size(); i++)
    {
        m_folderModel->setHeaderData(i, Qt::Horizontal, nameList[i], Qt::DisplayRole);
        m_folderModel->setHeaderData(i, Qt::Horizontal, sizeList[i], Qt::SizeHintRole);
    }

    ui->m_fileTreeView->setModel(m_folderModel);
    ui->m_fileTreeView->setItemDelegate(new FolderAndProjectDelegate(this));
    ui->m_fileTreeView->header()->resizeSections(QHeaderView::ResizeToContents);
    //    ui->m_fileTreeView->setFocusPolicy(Qt::NoFocus);

    ui->m_typeComboBox->addItem(QStringLiteral("所有类型"), QVariant::fromValue(
                                    QList<MaterialLibHelper::MaterialType>() << MaterialLibHelper::project <<
                                    MaterialLibHelper::folder));
    ui->m_typeComboBox->addItem(
        QStringLiteral("工程文件"),
        QVariant::fromValue(QList<MaterialLibHelper::MaterialType>() << MaterialLibHelper::project));
    ui->m_typeComboBox->addItem(
        QStringLiteral("文件夹"),
        QVariant::fromValue(QList<MaterialLibHelper::MaterialType>() << MaterialLibHelper::folder));
    ui->m_typeComboBox->setCurrentIndex(0);

    ui->m_presonalMaterialButton->setChecked(true);
    ui->m_presonalMaterialView->show();
    ui->m_publicMaterialButton->setChecked(false);
    ui->m_publicMaterialView->hide();

    //    ui->m_filePathWidget->setDirs(QStringList() << QStringLiteral("公共素材"));
    //    QMap<int, QVariant> map;
    //    map[Qt::UserRole] = "-1";//columnId
    //    map[Qt::UserRole + 1] = "0";//folderId
    //    ui->m_filePathWidget->setDatas(QList<QMap<int, QVariant>>() << map);
}

void NetworkColumnAndFoklerWidget::initConnect()
{
    connect(m_uploadOperator, &UploadOperator::publicColumns,
            [&](QList<QSharedPointer<SubRequests::ColumnBaseInfo>>& colList)
            {
                m_publicModel->onPublicColumns(colList);

                if (m_type == ColumnType_NewSeriesProject)
                {
                    if (!colList.isEmpty())
                    {
                        QString columnId = colList[0]->id;
                        ui->m_filePathWidget->setDirs(QStringList() << colList[0]->name);
                        QMap<int, QVariant> map;
                        map[Qt::UserRole] = columnId; //columnId
                        map[Qt::UserRole + 1] = "0"; //folderId
                        map[Qt::UserRole + 2] = colList[0]->nle_config.enabled; //nleEnabled
                        map[Qt::UserRole + 3] = colList[0]->nle_config.limit_folder; //limit_folder
                        map[Qt::UserRole + 4] = colList[0]->has_permission; //has_permission
                        ui->m_filePathWidget->setDatas(QList<QMap<int, QVariant>>() << map);
                        switchColumn(columnId);
                        onDirsChanged();
                    }
                }
                else
                {
                    QString columnId = ui->m_filePathWidget->data(ui->m_filePathWidget->dirs().size() - 1, Qt::UserRole)
                                         .toString();
                    if (columnId.isEmpty())
                    {
                        return;
                    }
                    switchColumn(columnId);
                    onDirsChanged();
                }
            });
    connect(m_uploadOperator, &UploadOperator::folder, m_folderModel, &UploadFolderModel::onFolder);
    connect(ui->m_fileTreeView->verticalScrollBar(), &QScrollBar::valueChanged, [&](int value)
    {
        auto vBar = ui->m_fileTreeView->verticalScrollBar();
        if (vBar->isVisible() && value == vBar->maximum())
        {
            m_folderModel->fetchMore(QModelIndex());
        }
    });
    connect(ui->m_presonalMaterialButton, &QPushButton::clicked, [&](bool checked)
    {
        if (checked)
        {
            ui->m_presonalMaterialView->show();
            ui->m_publicMaterialButton->setChecked(false);
            ui->m_publicMaterialView->hide();
        }
        else
        {
            ui->m_presonalMaterialView->hide();
        }
    });
    connect(ui->m_publicMaterialButton, &QPushButton::clicked, [&](bool checked)
    {
        if (checked)
        {
            if (m_type == ColumnType_CheckProject)
            {
                ui->m_filePathWidget->setDirs(QStringList() << QStringLiteral("公共素材"));
                QMap<int, QVariant> map;
                map[Qt::UserRole] = "-1";
                map[Qt::UserRole + 1] = "0";
                ui->m_filePathWidget->setDatas(QList<QMap<int, QVariant>>() << map);
                m_folderModel->clearData();
                m_uploadOperator->getFolder(false, "0", "0",
                                            ui->m_typeComboBox->currentData().value<QList<
                                                MaterialLibHelper::MaterialType>>(), ui->m_searchLineEdit->text());
            }
            ui->m_publicMaterialView->show();
            ui->m_presonalMaterialButton->setChecked(false);
            ui->m_presonalMaterialView->hide();
        }
        else
        {
            ui->m_publicMaterialView->hide();
        }
    });
    connect(ui->m_presonalMaterialView, &QTreeView::clicked, [&](const QModelIndex& index)
    {
        ui->m_filePathWidget->setDirs(QStringList() << index.data().toString());
        QMap<int, QVariant> map;
        map[Qt::UserRole] = index.data(Qt::UserRole + 1); //columnId
        map[Qt::UserRole + 1] = "0"; //folderId
        ui->m_filePathWidget->setDatas(QList<QMap<int, QVariant>>() << map);
        m_folderModel->clearData();
        if (map[Qt::UserRole].toString() == "0")
        {
            //我的素材
            m_folderModel->onFolderSync(m_uploadOperator->getPersonalMediaSync(
                m_type == ColumnType_UploadMaterial, "0",
                ui->m_typeComboBox->currentData().value<QList<MaterialLibHelper::MaterialType>>(),
                ui->m_searchLineEdit->text()));
        }
        m_currentHighlightedFolderId = "";
    });
    connect(ui->m_publicMaterialView, &QTreeView::clicked, [&](const QModelIndex& index)
    {
        bool hasPermission = index.data(Qt::UserRole + 3).toBool();
        bool nleEnabled = index.data(Qt::UserRole + 4).toBool();
        int limit_folder = index.data(Qt::UserRole + 5).toInt();
        if (!hasPermission)
        {
            return;
        }
        ui->m_filePathWidget->setDirs(QStringList() << index.data().toString());
        QMap<int, QVariant> map;
        map[Qt::UserRole] = index.data(Qt::UserRole + 1);
        map[Qt::UserRole + 1] = "0";
        map[Qt::UserRole + 2] = nleEnabled; //nleEnabled
        map[Qt::UserRole + 3] = limit_folder; //limit_folder
        map[Qt::UserRole + 4] = hasPermission; //has_permission
        ui->m_filePathWidget->setDatas(QList<QMap<int, QVariant>>() << map);
        m_folderModel->clearData();
        if ((nleEnabled && limit_folder == 0) || m_type == ColumnType_UploadMaterial || m_type ==
            ColumnType_CheckProject)
        {
            //启用，且不限制才获取文件夹
            m_folderModel->onFolderSync(m_uploadOperator->getFolderSync(m_type == ColumnType_UploadMaterial,
                                                                        index.data(Qt::UserRole + 1).toString(), "0",
                                                                        ui->m_typeComboBox->currentData().value<QList<
                                                                            MaterialLibHelper::MaterialType>>(),
                                                                        ui->m_searchLineEdit->text()));
        }
        m_currentHighlightedFolderId = "";
    });

    connect(ui->m_searchLineEdit, &QLineEdit::textEdited, this, &NetworkColumnAndFoklerWidget::onDirsChanged);
    connect(ui->m_typeComboBox, &QComboBox::currentTextChanged, this, &NetworkColumnAndFoklerWidget::onDirsChanged);
    connect(ui->m_filePathWidget, &UploadPathWidget::dirsChanged, this, &NetworkColumnAndFoklerWidget::onDirsChanged);

    connect(ui->m_fileTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &NetworkColumnAndFoklerWidget::selectionChanged);
    connect(ui->m_fileTreeView, &QTreeView::clicked, this, &NetworkColumnAndFoklerWidget::clicked);

    ui->m_fileTreeView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    auto selectionMode = ui->m_fileTreeView->selectionModel();
    connect(selectionMode, &QItemSelectionModel::selectionChanged, [&](
            const QItemSelection& selected,
            const QItemSelection& deselected)
            {
                QModelIndexList indexes = selected.indexes();
                foreach(const QModelIndex &index, indexes)
                {
                    int type = index.data(Qt::UserRole + 5).toInt();
                    if (type == -1)
                    {
                        auto data = index.data(Qt::UserRole).value<FolderData>();
                        QString columnId = data.column_id;
                        QString folderId = data.material_id;
                        m_currentHighlightedFolderId = folderId;
                    }
                }
            });

    connect(ui->m_fileTreeView, &QTreeView::doubleClicked, this, &NetworkColumnAndFoklerWidget::doubleClicked);
    connect(ui->m_fileTreeView, &QTreeView::doubleClicked, [&](const QModelIndex& index)
    {
        int type = index.data(Qt::UserRole + 5).toInt();
        if (type == -1)
        {
            auto data = index.data(Qt::UserRole).value<FolderData>();
            QString columnId = data.column_id;
            QString folderId = data.material_id;
            m_currentHighlightedFolderId = folderId;
            bool enabled;
            int limit_folder;
            bool has_permission;
            if (columnId == "0")
            {
                enabled = true;
                limit_folder = 0;
                has_permission = true;
            }
            else
            {
                auto columns = m_uploadOperator->getColumnDetails(m_type == ColumnType_UploadMaterial, {columnId});
                if (columns.isEmpty())
                {
                    return;
                }
                enabled = columns[0]->nle_config.enabled;
                limit_folder = columns[0]->nle_config.limit_folder;
                has_permission = columns[0]->has_permission;
            }
            if (columnId == "0")
            {
                data.name_path.prepend(QStringLiteral("我的素材"));
            }
            else
            {
                data.name_path.prepend(data.column_name);
            }
            if (data.id_path.isEmpty())
            {
                data.id_path.prepend("0");
            }
            data.name_path.append(data.name);
            data.id_path.append(data.material_id);
            QList<QMap<int, QVariant>> dataList;
            for (int i = 0; i < data.name_path.size(); i++)
            {
                QMap<int, QVariant> map;
                map[Qt::UserRole] = columnId;
                map[Qt::UserRole + 1] = data.id_path[i];
                dataList.append(map);
            }
            dataList[0][Qt::UserRole + 2] = enabled;
            dataList[0][Qt::UserRole + 3] = limit_folder;
            dataList[0][Qt::UserRole + 4] = has_permission;
            ui->m_filePathWidget->setDirs(data.name_path);
            ui->m_filePathWidget->setDatas(dataList);
            switchColumn(columnId);
            m_folderModel->clearData();
            if (columnId == "0")
            {
                //我的素材
                m_folderModel->onFolderSync(m_uploadOperator->getPersonalMediaSync(
                    m_type == ColumnType_UploadMaterial, folderId,
                    ui->m_typeComboBox->currentData().value<QList<MaterialLibHelper::MaterialType>>(),
                    ui->m_searchLineEdit->text()));
            }
            else
            {
                //公共栏目
                m_folderModel->onFolderSync(m_uploadOperator->getFolderSync(
                    m_type == ColumnType_UploadMaterial, columnId, folderId,
                    ui->m_typeComboBox->currentData().value<QList<MaterialLibHelper::MaterialType>>(),
                    ui->m_searchLineEdit->text()));
            }
        }
    });

    connect(m_folderModel, &UploadFolderModel::getMoreData, [&](int page, int size)
    {
        QString columnId = ui->m_filePathWidget->data(ui->m_filePathWidget->dirs().size() - 1, Qt::UserRole).toString();
        QString folderId = ui->m_filePathWidget->data(ui->m_filePathWidget->dirs().size() - 1, Qt::UserRole + 1).
                               toString();
        if (columnId.isEmpty() || folderId.isEmpty())
        {
            return;
        }
        if (columnId == "0")
        {
            //我的素材
            m_folderModel->onFolderSync(m_uploadOperator->getPersonalMediaSync(
                m_type == ColumnType_UploadMaterial, folderId,
                ui->m_typeComboBox->currentData().value<QList<MaterialLibHelper::MaterialType>>(),
                ui->m_searchLineEdit->text(), page, size));
        }
        else if (columnId == "-1")
        {
            //公共素材
            //            m_folderModel->onFolderSync(m_uploadOperator->getPublicMaterialSync(MaterialLibHelper::ViewNleProject, ui->m_typeComboBox->currentData().value<QList<MaterialLibHelper::MaterialType>>(), ui->m_searchLineEdit->text(), page, size));
            m_folderModel->onFolderSync(m_uploadOperator->getFolderSync(m_type == ColumnType_UploadMaterial, "0", "0",
                                                                        ui->m_typeComboBox->currentData().value<QList<
                                                                            MaterialLibHelper::MaterialType>>(),
                                                                        ui->m_searchLineEdit->text(), page, size));
        }
        else
        {
            //公共栏目
            m_folderModel->onFolderSync(m_uploadOperator->getFolderSync(m_type == ColumnType_UploadMaterial, columnId,
                                                                        folderId,
                                                                        ui->m_typeComboBox->currentData().value<QList<
                                                                            MaterialLibHelper::MaterialType>>(),
                                                                        ui->m_searchLineEdit->text(), page, size));
        }
    });
}

void NetworkColumnAndFoklerWidget::switchColumn(const QString& columnId)
{
    if (columnId == "0" || columnId == "1" || columnId == "2")
    {
        //我的素材
        ui->m_presonalMaterialButton->setChecked(true);
        ui->m_presonalMaterialView->show();
        ui->m_publicMaterialButton->setChecked(false);
        ui->m_publicMaterialView->hide();
        auto index = m_presonalModel->index(columnId);
        if (!index.isValid())
        {
            index = m_presonalModel->index(0, 0, QModelIndex());
        }
        ui->m_presonalMaterialView->setCurrentIndex(index);
        auto pos = ui->m_presonalMaterialView->mapToParent(ui->m_presonalMaterialView->visualRect(index).topLeft());
        ui->m_columnScrollArea->ensureVisible(pos.x(), pos.y());
    }
    else if (columnId == "-1")
    {
        //公共素材
        ui->m_presonalMaterialButton->setChecked(false);
        ui->m_presonalMaterialView->hide();
        ui->m_publicMaterialButton->setChecked(true);
        ui->m_publicMaterialView->show();
    }
    else
    {
        //公共栏目
        ui->m_presonalMaterialButton->setChecked(false);
        ui->m_presonalMaterialView->hide();
        ui->m_publicMaterialButton->setChecked(true);
        ui->m_publicMaterialView->show();
        auto index = m_publicModel->index(columnId);
        if (!index.isValid())
        {
            index = m_publicModel->index(0, 0, QModelIndex());
        }
        ui->m_publicMaterialView->setCurrentIndex(index);
        auto pos = ui->m_publicMaterialView->mapToParent(ui->m_publicMaterialView->visualRect(index).topLeft());
        ui->m_columnScrollArea->ensureVisible(pos.x(), pos.y());
    }
}

void NetworkColumnAndFoklerWidget::onDirsChanged()
{
    QString columnId = ui->m_filePathWidget->data(ui->m_filePathWidget->dirs().size() - 1, Qt::UserRole).toString();
    QString folderId = ui->m_filePathWidget->data(ui->m_filePathWidget->dirs().size() - 1, Qt::UserRole + 1).toString();
    bool nleEnabled = ui->m_filePathWidget->data(ui->m_filePathWidget->dirs().size() - 1, Qt::UserRole + 2).toBool();
    int limit_folder = ui->m_filePathWidget->data(ui->m_filePathWidget->dirs().size() - 1, Qt::UserRole + 3).toInt();
    bool hasPermission = ui->m_filePathWidget->data(ui->m_filePathWidget->dirs().size() - 1, Qt::UserRole + 4).toBool();
    if (columnId.isEmpty() || folderId.isEmpty())
    {
        return;
    }
    m_folderModel->clearData();
    if (columnId == "0")
    {
        //我的素材
        m_folderModel->onFolderSync(m_uploadOperator->getPersonalMediaSync(
            m_type == ColumnType_UploadMaterial, folderId,
            ui->m_typeComboBox->currentData().value<QList<MaterialLibHelper::MaterialType>>(),
            ui->m_searchLineEdit->text()));
    }
    else if (columnId == "-1")
    {
        //公共素材
        //        m_folderModel->onFolderSync(m_uploadOperator->getPublicMaterialSync(MaterialLibHelper::ViewNleProject, ui->m_typeComboBox->currentData().value<QList<MaterialLibHelper::MaterialType>>(), ui->m_searchLineEdit->text()));
        m_folderModel->onFolderSync(m_uploadOperator->getFolderSync(m_type == ColumnType_UploadMaterial, "0", "0",
                                                                    ui->m_typeComboBox->currentData().value<QList<
                                                                        MaterialLibHelper::MaterialType>>(),
                                                                    ui->m_searchLineEdit->text()));
    }
    else
    {
        //公共栏目
        if ((nleEnabled && limit_folder == 0) || m_type == ColumnType_UploadMaterial || m_type ==
            ColumnType_CheckProject)
        {
            m_folderModel->onFolderSync(m_uploadOperator->getFolderSync(m_type == ColumnType_UploadMaterial, columnId,
                                                                        folderId,
                                                                        ui->m_typeComboBox->currentData().value<QList<
                                                                            MaterialLibHelper::MaterialType>>(),
                                                                        ui->m_searchLineEdit->text()));
        }
    }
}

void NetworkColumnAndFoklerWidget::switchDir(const QString& folderId)
{
    QString columnId = ui->m_filePathWidget->data(ui->m_filePathWidget->dirs().size() - 1, Qt::UserRole).toString();
    bool nleEnabled = ui->m_filePathWidget->data(ui->m_filePathWidget->dirs().size() - 1, Qt::UserRole + 2).toBool();
    int limit_folder = ui->m_filePathWidget->data(ui->m_filePathWidget->dirs().size() - 1, Qt::UserRole + 3).toInt();
    bool hasPermission = ui->m_filePathWidget->data(ui->m_filePathWidget->dirs().size() - 1, Qt::UserRole + 4).toBool();
    if (columnId.isEmpty() || folderId.isEmpty())
    {
        return;
    }
    m_folderModel->clearData();
    if (columnId == "0")
    {
        //我的素材
        m_folderModel->onFolderSync(m_uploadOperator->getPersonalMediaSync(
            m_type == ColumnType_UploadMaterial, folderId,
            ui->m_typeComboBox->currentData().value<QList<MaterialLibHelper::MaterialType>>(),
            ui->m_searchLineEdit->text()));
    }
    else if (columnId == "-1")
    {
        //公共素材
        //        m_folderModel->onFolderSync(m_uploadOperator->getPublicMaterialSync(MaterialLibHelper::ViewNleProject, ui->m_typeComboBox->currentData().value<QList<MaterialLibHelper::MaterialType>>(), ui->m_searchLineEdit->text()));
        m_folderModel->onFolderSync(m_uploadOperator->getFolderSync(m_type == ColumnType_UploadMaterial, "0", "0",
                                                                    ui->m_typeComboBox->currentData().value<QList<
                                                                        MaterialLibHelper::MaterialType>>(),
                                                                    ui->m_searchLineEdit->text()));
    }
    else
    {
        //公共栏目
        if ((nleEnabled && limit_folder == 0) || m_type == ColumnType_UploadMaterial || m_type ==
            ColumnType_CheckProject)
        {
            m_folderModel->onFolderSync(m_uploadOperator->getFolderSync(m_type == ColumnType_UploadMaterial, columnId,
                                                                        folderId,
                                                                        ui->m_typeComboBox->currentData().value<QList<
                                                                            MaterialLibHelper::MaterialType>>(),
                                                                        ui->m_searchLineEdit->text()));
        }
    }
}
