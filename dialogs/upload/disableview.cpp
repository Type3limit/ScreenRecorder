//
// Created by 58226 on 2024/11/19.
//

// You may need to build the project (run Qt uic code generator) to get "ui_DisableView.h" resolved

#include "disableview.h"
#include "ui_DisableView.h"


DisableView::DisableView(QWidget *parent) :
    QWidget(parent), ui(new Ui::DisableView) {
    ui->setupUi(this);
    ui->m_iconLabel->setPixmap(QPixmap(":/icons/images/information.svg").scaled(ui->m_iconLabel->size(), Qt::KeepAspectRatio));

}

DisableView::~DisableView() {
    delete ui;
}

void DisableView::resizeEvent(QResizeEvent* event)
{
    ui->m_iconLabel->setPixmap(QPixmap(":/icons/images/information.svg").scaled(ui->m_iconLabel->size(), Qt::KeepAspectRatio));
    QWidget::resizeEvent(event);
}

void DisableView::setNoticeMessage(const QString& message)
{
    ui->m_noticeLabel->setText(message);
}
