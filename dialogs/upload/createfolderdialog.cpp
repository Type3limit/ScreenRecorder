//
// Created by 58226 on 2024/12/12.
//

// You may need to build the project (run Qt uic code generator) to get "ui_CreateFolderDialog.h" resolved

#include "createfolderdialog.h"
#include "ui_CreateFolderDialog.h"
#include "usermessagebox.h"


CreateFolderDialog::CreateFolderDialog(QWidget *parent) :
                                                        DragMoveDialog(parent), ui(new Ui::CreateFolderDialog) {
    ui->setupUi(this);
    init();
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowIcon(QIcon(QString(":/icons/images/recording.svg")));
}

CreateFolderDialog::~CreateFolderDialog() {
    delete ui;
}

void CreateFolderDialog::init()
{
    connect(ui->closeButton,&QPushButton::clicked,this,&CreateFolderDialog::close);
    connect(ui->cancelButton,&QPushButton::clicked,this,&CreateFolderDialog::close);
    connect(ui->sureButton,&QPushButton::clicked,this,[&]()
    {
        auto curName =ui->folderNameEditor->text();
        if (curName.isEmpty())
        {
            UserMessageBox::warning(nullptr,u8"警告",u8"文件夹名不能为空");
            return;
        }
        emit createFolder(curName);
        close();
    });
}
