//
// Created by Type3Limit on 2023/11/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_countdowndialog.h" resolved

#include "countdowndialog.h"
#include "ui_countdowndialog.h"
#include "Fluent/FluentTheme.h"
#include <QKeyEvent>
#include <QScreen>

#include "obswrapper.h"

namespace {

QString countDownDialogStyleSheet()
{
    const auto &colors = Fluent::ThemeManager::instance().colors();
    const QColor accentHover = colors.accent.lighter(108);
    const QColor accentPressed = colors.accent.darker(108);

    return QString(
        "QFrame#contentFrame {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 14px;"
        "}"
        "QLabel#displayLabel {"
        "  color: %3;"
        "  font-size: 120px;"
        "  font-weight: 700;"
        "}"
        "QPushButton#startButton {"
        "  background-color: %3;"
        "  color: #FFFFFF;"
        "  border: 1px solid %3;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "}"
        "QPushButton#startButton:hover {"
        "  background-color: %4;"
        "  border-color: %4;"
        "}"
        "QPushButton#startButton:pressed {"
        "  background-color: %5;"
        "  border-color: %5;"
        "}")
        .arg(colors.surface.name())
        .arg(colors.border.name())
        .arg(colors.accent.name())
        .arg(accentHover.name())
        .arg(accentPressed.name());
}

}


CountDownDialog::CountDownDialog( const int countDownSeconds,const QSharedPointer<ObsWrapper>& obs,QScreen* curSc,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CountDownDialog),
m_countDownSeconds(countDownSeconds),
m_obs(obs),m_curScreen(curSc)
{
    ui->setupUi(this);
    setStyleSheet(countDownDialogStyleSheet());
    connect(&Fluent::ThemeManager::instance(), &Fluent::ThemeManager::themeChanged, this, [this]() {
        setStyleSheet(countDownDialogStyleSheet());
    });
    setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::SubWindow);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowIcon(QIcon(QString(":/icons/images/recording.svg")));
    setWindowTitle(u8"倒计时");
    setDialogCenter(m_curScreen);
    ui->displayLabel->setText(QString::number(countDownSeconds));
    m_timer.setInterval(1000);
    connect(&m_timer,&QTimer::timeout,this,[&]()
    {
        --m_countDownSeconds;
        ui->displayLabel->setText(QString::number(m_countDownSeconds));
        if(m_countDownSeconds<=0)
        {
            m_timer.stop();
            close();
            m_obs->startRecording();
        }
    });

    connect(ui->startButton,&QPushButton::clicked,this,[&]()
    {
            m_timer.stop();
            close();
            m_obs->startRecording();
    });
    m_timer.start();
}

CountDownDialog::~CountDownDialog() {
    delete ui;
}

void CountDownDialog::setDialogCenter(QScreen* scrren)
{

    QRect screenGeometry = scrren->geometry();
    QSize dialogSize = sizeHint();

    int x = screenGeometry.center().x() - dialogSize.width() / 2;
    int y = screenGeometry.center().y() - dialogSize.height() / 2;

    move(x, y);
}

void CountDownDialog::keyPressEvent(QKeyEvent *event)
{
    // 禁用Escape键关闭行为
    if (event->key() == Qt::Key_Escape)
    {
        event->ignore();
    }
    else
    {
        QDialog::keyPressEvent(event);
    }
}