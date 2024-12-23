//
// Created by 58226 on 2024/12/12.
//

#ifndef CREATEFOLDERDIALOG_H
#define CREATEFOLDERDIALOG_H

#include <QWidget>
#include "dragmovedialog.h"
QT_BEGIN_NAMESPACE
namespace Ui { class CreateFolderDialog; }
QT_END_NAMESPACE

class CreateFolderDialog : public DragMoveDialog {
Q_OBJECT

public:
    explicit CreateFolderDialog(QWidget *parent = nullptr);
    ~CreateFolderDialog() override;
    void init();
public:
      signals:
        void createFolder(const QString& FolderName);

private:
    Ui::CreateFolderDialog *ui;

};


#endif //CREATEFOLDERDIALOG_H
