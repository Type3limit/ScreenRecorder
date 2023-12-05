#include "usermessagebox.h"
#include "ui_usermessagebox.h"

#include <QStyleOption>
#include <QPainter>


UserMessageBox::UserMessageBox(QWidget* parent) :
    FramelessDialog(parent),
    ui(new Ui::UserMessageBox)
{
    ui->setupUi(this);

    ui->m_button0->setProperty("class", "confirm");
    ui->m_button1->setProperty("class", "cancel");

    // change frame less dialog border radius
    // 1. set transparent,
    // 2. override painEvent
    // 3. set border-radius in style sheet
    setAttribute(Qt::WA_TranslucentBackground);
}

UserMessageBox::~UserMessageBox()
{
    delete ui;
}

UserMessageBox::ButtonType UserMessageBox::information(QWidget* parent, const QString& title, const QString& text,
                                                       ButtonType button0, ButtonType button1)
{
    return showMessageBox(parent, title, text, button0, button1, Info);
}

UserMessageBox::ButtonType UserMessageBox::question(QWidget* parent, const QString& title, const QString& text,
                                                    ButtonType button0, ButtonType button1)
{
    return showMessageBox(parent, title, text, button0, button1, Ques);
}

UserMessageBox::ButtonType UserMessageBox::warning(QWidget* parent, const QString& title, const QString& text,
                                                   ButtonType button0, ButtonType button1,
                                                   const QString& button0Text, const QString& button1Text)
{
    return showMessageBox(parent, title, text, button0, button1, Warn, button0Text, button1Text);
}

UserMessageBox::ButtonType UserMessageBox::critical(QWidget* parent, const QString& title, const QString& text,
                                                    ButtonType button0, ButtonType button1)
{
    return showMessageBox(parent, title, text, button0, button1, Fail);
}


void UserMessageBox::setTitle(const QString& title)
{
    ui->m_titleLabel->setText(title);
}

void UserMessageBox::setText(const QString& text)
{
    ui->m_contentLabel->setText(text);
}

void UserMessageBox::setButton1Hidden(bool hidden)
{
    ui->m_button1->setHidden(hidden);
}

void UserMessageBox::setButton0Name(const QString& buttonTitle)
{
    ui->m_button0->setText(buttonTitle);
}

void UserMessageBox::setButton1Name(const QString& buttonTitle)
{
    ui->m_button1->setText(buttonTitle);
}

void UserMessageBox::paintEvent(QPaintEvent*)
{
    QStyleOption option;
    option.init(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
}

UserMessageBox::ButtonType UserMessageBox::showMessageBox(QWidget* parent,
                                                          const QString& title,
                                                          const QString& text,
                                                          ButtonType button0,
                                                          ButtonType button1,
                                                          NoticeType noticeType,
                                                          const QString& button0Title,
                                                          const QString& button1Title)
{
    UserMessageBox messageBox;

    auto styleStr = QString(
        "QDialog {"
        "background-color: #28282E;"
        "border: 1px solid #0F0F0F;"
        "border-radius: 10px;"
        "}"
        "QDialog #m_notifyLabel {"
        "image: url(:/icons/images/{typeIcons}.svg)"
        "}"
        "QDialog #m_titleLabel {"
        "  color:#99A1B0;"
        "font-size:14px;"
        "font-family:Microsoft YaHei UI;"
        "}"
        "QDialog #m_contentLabel {"
        "color:#99A1B0;"
        "font-size:12px;"
        "font-family:Microsoft YaHei UI;"
        "}"
        "QPushButton"
        "{"
        "color:#99A1B0;"
        "font-size:12px;"
        "font-family:Microsoft YaHei UI;"
        "background-color:#0F0F0F;"
        "border: 1px solid #0F0F0F;"
        "border-radius:4px;"
        "}"
        "QPushButton::hover"
        "{"
        "background-color:#880F0F0F;"
        "}");
    switch (noticeType)
    {
    case Info:
        {
            messageBox.setStyleSheet(styleStr.replace("{typeIcons}", "information"));
            break;
        }

    case Warn:
        {
            messageBox.setStyleSheet(styleStr.replace("{typeIcons}", "warning"));
            break;
        }

    case Ques:
        {
            messageBox.setStyleSheet(styleStr.replace("{typeIcons}", "question"));
            break;
        }

    case Fail:
        {
            messageBox.setStyleSheet(styleStr.replace("{typeIcons}", "error"));
            break;
        }
    }
    messageBox.setTitle(title);
    messageBox.setText(text);

    if ("" != button0Title)
    {
        messageBox.setButton0Name(button0Title);
    }

    if ("" != button1Title)
    {
        messageBox.setButton1Name(button1Title);
    }

    if (button1 == NoButton)
    {
        messageBox.setButton1Hidden(true);
    }

    return QDialog::Accepted == messageBox.exec() ? button0 : button1;
}
