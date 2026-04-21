#include "usermessagebox.h"
#include "ui_usermessagebox.h"

#include "Fluent/FluentTheme.h"

#include <QStyleOption>
#include <QPainter>

namespace {

QString noticeIconName(const UserMessageBox::NoticeType noticeType)
{
    switch (noticeType)
    {
    case UserMessageBox::Info:
        return "information";
    case UserMessageBox::Warn:
        return "warning";
    case UserMessageBox::Ques:
        return "question";
    case UserMessageBox::Fail:
        return "error";
    }

    return "information";
}

QString messageBoxStyleSheet(const UserMessageBox::NoticeType noticeType)
{
    const auto &colors = Fluent::ThemeManager::instance().colors();
    const QString iconName = noticeIconName(noticeType);
    const QColor primaryHover = colors.accent.lighter(110);
    const QColor primaryPressed = colors.accent.darker(110);

    return QString(
        "%1"
        "QDialog {"
        "  border-radius: 12px;"
        "}"
        "QDialog #m_notifyLabel {"
        "  image: url(:/icons/images/%2.svg);"
        "}"
        "QDialog #m_titleLabel {"
        "  color: %3;"
        "  font-size: 15px;"
        "  font-weight: 600;"
        "}"
        "QDialog #m_contentLabel {"
        "  color: %4;"
        "  font-size: 13px;"
        "}"
        "QPushButton#m_button0 {"
        "  color: #FFFFFF;"
        "  background-color: %5;"
        "  border: 1px solid %5;"
        "  border-radius: 6px;"
        "  padding: 6px 14px;"
        "}"
        "QPushButton#m_button0:hover {"
        "  background-color: %6;"
        "  border-color: %6;"
        "}"
        "QPushButton#m_button0:pressed {"
        "  background-color: %7;"
        "  border-color: %7;"
        "}"
        "QPushButton#m_button1 {"
        "  color: %3;"
        "  background-color: %8;"
        "  border: 1px solid %9;"
        "  border-radius: 6px;"
        "  padding: 6px 14px;"
        "}"
        "QPushButton#m_button1:hover {"
        "  background-color: %10;"
        "  border-color: %5;"
        "}"
        "QPushButton#m_button1:pressed {"
        "  background-color: %11;"
        "  border-color: %5;"
        "}")
        .arg(Fluent::Theme::dialogStyle(colors))
        .arg(iconName)
        .arg(colors.text.name())
        .arg(colors.subText.name())
        .arg(colors.accent.name())
        .arg(primaryHover.name())
        .arg(primaryPressed.name())
        .arg(colors.surface.name())
        .arg(colors.border.name())
        .arg(colors.hover.name())
        .arg(colors.pressed.name());
}

}


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
    UserMessageBox messageBox(parent);

    messageBox.setStyleSheet(messageBoxStyleSheet(noticeType));
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
