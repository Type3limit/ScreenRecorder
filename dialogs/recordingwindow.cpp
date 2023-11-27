//
// Created by 58226 on 2023/11/21.
//

#include "recordingwindow.h"
#include "dialog/ui_RecordingWindow.h"
#include <QMouseEvent>
#include <QMessageBox>
#include <QStringListModel>
#include <QScreen>
#include <QStyledItemDelegate>
#include <QListView>
#include <QTcpSocket>
#include <QFileDialog>

#include "backgroundwindow.h"
#include "countdowndialog.h"


#define POPVIEW(styledItem) ui->styledItem->setView(new QListView(ui->styledItem));


enum ScreenArea
{
    DeskTop = 0,
    Customized = 1
};

const QString ScreenAreaStr[2] = {u8"全屏", u8"自定义"};

bool isValidFilePath(const QString &filePath) {

    QRegExp rx("^[a-zA-Z0-9:/._-]+$");

    if (rx.exactMatch(filePath)) {
        qDebug() << "文件路径合法：" << filePath;
        return true;
    } else {
        qDebug() << "文件路径包含非法字符：" << filePath;
        return false;
    }
}


RecordingWindow::RecordingWindow(QWidget* parent) :
    QMainWindow(parent), ui(new Ui::RecordingWindow),
    m_obs(QSharedPointer<ObsWrapper>(new ObsWrapper())),
    m_recordHotKey(new QHotkey(nullptr)),
    m_pauseHotKey(new QHotkey(nullptr))
{
    ui->setupUi(this);
    // hide title bar
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    init();
}

RecordingWindow::~RecordingWindow()
{
    m_server->close();
    delete m_miniWindow;
    m_backgroundWindow->close();
    delete m_backgroundWindow;
    delete ui;
}

void RecordingWindow::init()
{
    //init obs
    auto firstScreen = QGuiApplication::primaryScreen();
    auto pix = qApp->devicePixelRatio();

    if (!m_obs->initObs(firstScreen->geometry().width()*pix,
        firstScreen->geometry().height()*pix, 60))
    {
        QMessageBox::warning(this, u8"警告", u8"初始化OBS失败！");
        exit(-1);
        return;
    }


    //miniwindow
    m_miniWindow = new MinimizedRecordingWindow(m_obs, nullptr);

    connect(m_miniWindow, &MinimizedRecordingWindow::onRecover, this, [&]()
    {
        m_miniWindow->hide();
        this->show();
    });
    connect(m_miniWindow, &MinimizedRecordingWindow::closed, this, [&]()
    {
        m_miniWindow->hide();
        this->show();
    });
    connect(m_miniWindow,&MinimizedRecordingWindow::onRecordAct,this,&RecordingWindow::startRecord);
    //backgroundWindow
    m_backgroundWindow = new BackgroundWindow(true, nullptr, firstScreen);
    connect(m_backgroundWindow,&BackgroundWindow::areaChanged,this,[&](
        int x1,int y1,int x2,int y2)
    {
        auto curScreen = QGuiApplication::screens().at(ui->screenDeviceComboBox->currentIndex());
        qreal devicePixelRatio = curScreen->devicePixelRatio();
        m_startPos = {x1,y1};
        m_endPos = {x2,y2};
        auto curRect = QRect{m_startPos,m_endPos};
        ui->areaWidthEdit->blockSignals(true);
        ui->areaHeightEdit->blockSignals(true);
        ui->areaWidthEdit->setText(QString::number(curRect.width()*devicePixelRatio));
        ui->areaHeightEdit->setText(QString::number(curRect.height()*devicePixelRatio));
        ui->areaWidthEdit->blockSignals(false);
        ui->areaHeightEdit->blockSignals(false);
    });


    //button connection
    connect(ui->closeButton, &QPushButton::clicked, this, [&]()
    {
        if (m_isRecordingStarted)
        {
            auto res = QMessageBox::question(this, "提示", "当前正在录制，确认退出？");
            if (res == QMessageBox::StandardButton::Yes)
            {
                stopRecord();
                close();
            }
        }
        else
        {
            close();
        }
    });

    connect(ui->minimalButton, &QPushButton::clicked, this, [&]()
    {
        this->hide();
        m_miniWindow->show();
    });

    //layout
    ui->SettingWidget->setHidden(true);
    this->setFixedHeight(295);
    connect(ui->settingExpander, &QCheckBox::stateChanged, this, [&](int status)
    {
        if (status == 0)
        {
            ui->SettingWidget->setHidden(true);
            this->setFixedHeight(295);
        }
        else
        {
            ui->SettingWidget->setHidden(false);
            this->setFixedHeight(565);
        }
    });
    //init config
    if (m_config.isDefault())
    {
        m_config.frameRateInUse = "25FPS";
        m_config.bitRateInUse = "4MB";
        m_config.savePath = QApplication::applicationDirPath();
        m_config.startRecordShortCut = QKeySequence::fromString("F7").toString();
        m_config.pauseRecordShortCut = QKeySequence::fromString("F8").toString();
        m_config.countDownEnable = false;
        m_config.countDownSeconds = 3;
        m_config.tcpPort = 29989;
        m_config.writeJson();
    }
    ui->nameEdit->setText(QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm"));
    POPVIEW(bitrateComboBox)
    ui->bitrateComboBox->setModel(new QStringListModel({"2MB", "4MB", "8MB"}, this));
    ui->bitrateComboBox->setCurrentText(m_config.bitRateInUse);
    POPVIEW(frameRateComboBox)
    ui->frameRateComboBox->setModel(new QStringListModel({"25FPS", "30FPS", "50FPS", "60FPS"}, this));
    ui->frameRateComboBox->setCurrentText(m_config.frameRateInUse);
    ui->savePathEdit->setText(m_config.savePath);
    ui->StartShortCut->setKeySequence(QKeySequence::fromString(m_config.startRecordShortCut));
    ui->PauseShortCut->setKeySequence(QKeySequence::fromString(m_config.pauseRecordShortCut));
    m_recordHotKey->setShortcut(ui->StartShortCut->keySequence(), true);
    m_pauseHotKey->setShortcut(ui->PauseShortCut->keySequence(), true);
    ui->countDownCheckBox->setChecked(m_config.countDownEnable);
    auto* validator = new QIntValidator(1, INT_MAX, this);
    ui->countDownEdit->setValidator(validator);

    ui->countDownEdit->setText(QString::number(m_config.countDownSeconds));


    //init server for communicate between different process
    m_server = new QTcpServer(this);
    m_server->setMaxPendingConnections(50);
    if (!m_server->listen(QHostAddress::Any, m_config.tcpPort))
    {
        qDebug() << "tcp server start listen at [" << m_config.tcpPort << "] failed!";
        QMessageBox::warning(this, u8"警告", u8"tcp服务监听端口：" + QString::number(m_config.tcpPort) + u8"失败！");
        return;
    }

    //timer for display recording status
    m_seconds = 0;
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, [&]()
    {
        ++m_seconds;
        int hours = m_seconds / 3600;
        int mins = (m_seconds - hours * 3600) / 60;
        int secs = m_seconds - hours * 3600 - mins * 60;
        QString formattedTime = QString("%1:%2:%3").arg(hours, 4, 10, QLatin1Char('0'))
                                                   .arg(mins, 2, 10, QLatin1Char('0'))
                                                   .arg(secs, 2, 10, QLatin1Char('0'));
        ui->statusLabel->setText(QString(formattedTime));
    });
    //widgets connection
    //audio signal display
    connect(m_obs.get(), &ObsWrapper::MicVolumeDataChange, ui->micphoneVolume, &VolumeControl::setLevels);
    connect(m_obs.get(), &ObsWrapper::PlayerVolumeDataChange, ui->playerVolume, &VolumeControl::setLevels);
    int audioChannel = m_obs->audioChannel();
    ui->micphoneVolume->setChannelCount(m_obs->micChannelCount());
    ui->micphoneVolume->setAudioChannel(audioChannel);
    ui->playerVolume->setChannelCount(m_obs->playerChannelCount());
    ui->playerVolume->setAudioChannel(audioChannel);
    //get desktops
    m_obs->addSceneSource(REC_DESKTOP);
    m_obs->SearchRecTargets(REC_DESKTOP);
    //screen mode
    POPVIEW(screenDeviceComboBox)
    ui->screenDeviceComboBox->setModel(new QStringListModel(m_obs->getRecTargets(), this));
    //area Init
    POPVIEW(areaComboBox)
    ui->areaComboBox->setModel(new QStringListModel({ScreenAreaStr[DeskTop], ScreenAreaStr[Customized]}, this));
    //capture type changed
    connect(ui->areaComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&](int curIndex)
    {
        auto style = QString("QPushButton{"
           "qproperty-icon:url(:/icons/images/{iconName}.svg);"
           "qproperty-iconSize:44px;"
           "background-color:#2F2F34;"
           "border-radius:4px;"
           "}");
        ui->areaButton->setStyleSheet(isFullScreenMode()?
            style.replace("{iconName}", "fullscreen")
                :style.replace("{iconName}","areas"));
        ui->areaWidthEdit->setDisabled(isFullScreenMode());
        ui->areaHeightEdit->setDisabled(isFullScreenMode());
        auto screens = QGuiApplication::screens();
        auto curScreen = screens.at(ui->screenDeviceComboBox->currentIndex());
        if (isFullScreenMode())
        {
            ui->areaWidthEdit->blockSignals(true);
            ui->areaHeightEdit->blockSignals(true);
            qreal devicePixelRatio = curScreen->devicePixelRatio();
            auto WValidator = new QIntValidator(0, curScreen->geometry().width() * devicePixelRatio, this);
            auto HValidator = new QIntValidator(0, curScreen->geometry().height() * devicePixelRatio, this);
            ui->areaWidthEdit->setText(QString::number(curScreen->geometry().width() * devicePixelRatio));
            ui->areaWidthEdit->setValidator(WValidator);
            ui->areaHeightEdit->setText(QString::number(curScreen->geometry().height() * devicePixelRatio));
            ui->areaHeightEdit->setValidator(HValidator);
            ui->areaWidthEdit->blockSignals(false);
            ui->areaHeightEdit->blockSignals(false);
        }
        QMetaObject::invokeMethod(this, "rebuildBackgroundWindow");
        QMetaObject::invokeMethod(this, "rebuildBackgroundWindow");
    });
    //screen comboBox changed
    connect(ui->screenDeviceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&](int curIndex)
    {
        auto curScreen = QGuiApplication::screens().at(curIndex);
        ui->areaWidthEdit->blockSignals(true);
        ui->areaHeightEdit->blockSignals(true);
        qreal devicePixelRatio = curScreen->devicePixelRatio();
        auto WValidator = new QIntValidator(0, curScreen->geometry().width() * devicePixelRatio, this);
        auto HValidator = new QIntValidator(0, curScreen->geometry().height() * devicePixelRatio, this);
        ui->areaWidthEdit->setText(QString::number(curScreen->geometry().width() * devicePixelRatio));
        ui->areaWidthEdit->setValidator(WValidator);
        ui->areaHeightEdit->setText(QString::number(curScreen->geometry().height() * devicePixelRatio));
        ui->areaHeightEdit->setValidator(HValidator);
        ui->areaWidthEdit->blockSignals(false);
        ui->areaHeightEdit->blockSignals(false);
        QMetaObject::invokeMethod(this, "rebuildBackgroundWindow");
    });
    ui->screenDeviceComboBox->setCurrentIndex(0);
    ui->areaComboBox->setCurrentIndex(0);
    ui->areaWidthEdit->blockSignals(true);
    ui->areaHeightEdit->blockSignals(true);
    ui->areaWidthEdit->setText(QString::number(firstScreen->geometry().width() * pix));
    ui->areaHeightEdit->setText(QString::number(firstScreen->geometry().height() * pix));
    rebuildBackgroundWindow();
    ui->areaWidthEdit->blockSignals(false);
    ui->areaHeightEdit->blockSignals(false);

    //Sound device name
    POPVIEW(micphoneDeviceComboBox)
    ui->micphoneDeviceComboBox->setModel(new QStringListModel({m_obs->micphoneDeviceName()}, this));
    ui->micphoneDeviceComboBox->setCurrentIndex(0);
    //player device name
    POPVIEW(playerDeviceComboBox)
    ui->playerDeviceComboBox->setModel(new QStringListModel({m_obs->playerDeviceName()}, this));
    ui->playerDeviceComboBox->setCurrentIndex(0);

    //button status
    connect(m_obs.get(), &ObsWrapper::recordStatusChanged, this, [&](int status)
    {
        m_backgroundWindow->hide();
        auto style = QString("QPushButton{"
            "qproperty-icon:url(:/icons/images/{iconName}.svg);"
            "qproperty-iconSize:44px;"
            "background-color:#2F2F34;"
            "border-radius:4px;"
            "}");
        if (status == RecordingStatus::Recording)
        {
            m_timer.start();
            ui->RecordingButton->setStyleSheet(style.replace("{iconName}", "stop_recoding"));
            ui->statusLabel->setText("");
            ui->ScreenAreaWidget->setDisabled(true);
            ui->MicphoneWidget->setDisabled(true);
            ui->PlayerWidget->setDisabled(true);
            ui->SettingWidget->setDisabled(true);
            m_isPauseNow = false;
        }
        else if (status == RecordingStatus::Paused)
        {
            m_timer.stop();
            ui->RecordingButton->setStyleSheet(style.replace("{iconName}", "stop_recoding"));
            ui->statusLabel->setText("暂停中...");
            ui->ScreenAreaWidget->setDisabled(true);
            ui->MicphoneWidget->setDisabled(true);
            ui->PlayerWidget->setDisabled(true);
            ui->SettingWidget->setDisabled(true);
            m_isPauseNow = true;
        }
        else
        {
            m_timer.stop();
            m_seconds = 0;
            ui->RecordingButton->setStyleSheet(style.replace("{iconName}", "start"));
            ui->statusLabel->setText("");
            ui->ScreenAreaWidget->setDisabled(false);
            ui->MicphoneWidget->setDisabled(false);
            ui->PlayerWidget->setDisabled(false);
            ui->SettingWidget->setDisabled(false);
            if (m_server != nullptr) //to notice other process
            {
                QTcpSocket* socket = nullptr;
                do
                {
                    socket = m_server->nextPendingConnection();
                    if (socket != nullptr)
                    {
                        socket->write(
                            QString(ui->savePathEdit->text() + ui->nameEdit->text() + ".mp4").toUtf8().data());
                        socket->flush();
                        socket->close();
                    }
                }
                while (socket != nullptr);
            }
            m_isPauseNow = false;
            m_backgroundWindow->show();
        }
    });

    //region set
    connect(ui->areaButton, &QPushButton::clicked, this, [&]()
    {
        if (isFullScreenMode())
            return;
        auto sc = QGuiApplication::screens().at(ui->screenDeviceComboBox->currentIndex());
        m_backgroundWindow->resetSelectionPos(sc->geometry().width(), sc->geometry().height());
    });

    auto setSizeAct = [&]()
    {
        if (isFullScreenMode())
            return;
        auto sc = QGuiApplication::screens().at(ui->screenDeviceComboBox->currentIndex());
        qreal devicePixelRatio = sc->devicePixelRatio();
        m_backgroundWindow->setSize(ui->areaWidthEdit->text().toInt()/devicePixelRatio,
            ui->areaHeightEdit->text().toInt()/devicePixelRatio);
    };
    connect(ui->areaWidthEdit, &QLineEdit::textEdited, this, [&,setSizeAct]()
    {
        setSizeAct();
    });

    connect(ui->areaHeightEdit, &QLineEdit::textEdited, this, [&,setSizeAct]()
    {
        setSizeAct();
    });
    //hotkey
    connect(ui->PauseShortCut, &QKeySequenceEdit::keySequenceChanged, this, [&](const QKeySequence& s)
    {
        m_pauseHotKey->setRegistered(false);
        delete m_pauseHotKey;
        m_pauseHotKey = new QHotkey(s, true);
        connect(m_pauseHotKey, &QHotkey::activated, this, &RecordingWindow::pauseRecord);
    });
    connect(ui->StartShortCut, &QKeySequenceEdit::keySequenceChanged, this, [&](const QKeySequence& s)
    {
        m_recordHotKey->setRegistered(false);
        delete m_recordHotKey;
        m_recordHotKey = new QHotkey(s, true);
        connect(m_recordHotKey, &QHotkey::activated, this, &RecordingWindow::startRecord);
    });
    connect(m_pauseHotKey, &QHotkey::activated, this, &RecordingWindow::pauseRecord);
    connect(m_recordHotKey, &QHotkey::activated, this, &RecordingWindow::startRecord);
    //save path
    connect(ui->pathSelectionButton,&QPushButton::clicked,this,[&]()
    {
        auto curStr =  QFileDialog::getExistingDirectory(this, tr("Open Directory"),"./");
        ui->savePathEdit->setText(curStr);
    });
    //recording button
    connect(ui->RecordingButton,&QPushButton::clicked,this,[&]()
    {
       startRecord();
    });
}

void RecordingWindow::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton)
    {
        return;
    }
    m_pos = e->globalPos() - mapToGlobal({0, 0});
    m_leftButtonPressed = true;
}

void RecordingWindow::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_leftButtonPressed)
    {
        return;
    }
    QPoint posOffset = e->globalPos() - m_pos;
    move(posOffset);
}

void RecordingWindow::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton)
    {
        return;
    }
    m_leftButtonPressed = false;
}

bool RecordingWindow::isFullScreenMode() const
{
    return ui->areaComboBox->currentText() == u8"全屏";
}


void RecordingWindow::closeEvent(QCloseEvent* event)
{
    m_obs->release();
}


void RecordingWindow::startRecord()
{
    if (!m_isRecordingStarted)
    {

        auto sc = QGuiApplication::screens().at(ui->screenDeviceComboBox->currentIndex());
        qreal devicePixelRatio = sc->devicePixelRatio();
        auto left = m_startPos.x();
        auto right = sc->geometry().width() - m_endPos.x();
        auto top = m_startPos.y();
        auto bottom = sc->geometry().height() - m_endPos.y();

        QRect cropRect = isFullScreenMode()?
            (QRect{0,0,sc->geometry().width(),sc->geometry().height()}):
            (QRect{m_startPos,m_endPos});

        auto fps = ui->frameRateComboBox->currentText().replace(QString("FPS"),QString("")).toInt();
        auto bitRate = ui->bitrateComboBox->currentText().replace(QString("MB"),QString("")).toInt();

        if(OBS_VIDEO_SUCCESS != m_obs->ResetVideo(sc->geometry().width(),sc->geometry().height(),
                   cropRect.width(),
                   cropRect.height(),fps))
        {
            qDebug()<<u8"reset video failed!";
            QMessageBox::warning(this,u8"警告",u8"输出重置失败!");
            return;
        }
        if(!m_obs->UpdateRecItem(ui->screenDeviceComboBox->currentText().toStdString().c_str(),
            REC_TYPE::REC_DESKTOP,!isFullScreenMode(),left,right,top,bottom))
        {
            qDebug()<<"find "<<ui->screenDeviceComboBox->currentText()<<"failed";
            QMessageBox::warning(this,u8"警告",u8"无法找到"+ui->screenDeviceComboBox->currentText());
            return;
        }

        auto dir = QDir (ui->savePathEdit->text());
        if(!dir.exists())
        {
            dir.mkpath(ui->savePathEdit->text());
        }
        if(!dir.exists())
        {
            qDebug()<<u8"save path"<<ui->savePathEdit->text()<<"invalid!";
            QMessageBox::warning(this,u8"警告",u8"当前存储路径不合法!");
            return;
        }


        auto fileName = ui->savePathEdit->text()+"/"+ui->nameEdit->text();

        if(!isValidFilePath(fileName))
        {
            qDebug()<<u8"name "<<ui->nameEdit->text()<<"invalid!";
            QMessageBox::warning(this,u8"警告",u8"文件名称或路径非法!");
            return;
        }

        m_obs->recSystemAudio();
        m_obs->recOutAudio();
        m_obs->setupFFmpeg(fileName,sc->geometry().width(),
            sc->geometry().height(),
            fps,bitRate);

        saveConfig();

        if(ui->countDownCheckBox->isChecked())//with count down dialog to start
        {
            if(m_countDownDialog!=nullptr)
            {
                delete m_countDownDialog;
                m_countDownDialog = nullptr;
            }
            //count down dialog
            m_countDownDialog= new CountDownDialog(ui->countDownEdit->text().toInt(),m_obs,sc,nullptr);
            m_countDownDialog->show();
        }
        else  // else, start immediately
        {
            if(0!=m_obs->startRecording())
            {
                qDebug()<<u8"start recording"<<fileName+".mp4"<<"falied!";
                QMessageBox::warning(this,u8"警告",u8"开始录屏失败!");
                return;
            }
        }
        m_isRecordingStarted = true;
    }
    else
    {
        stopRecord();
    }
}

void RecordingWindow::pauseRecord()
{
    if(m_obs->isRecordingStart())
    {
        m_obs->pauseRecording();
    }
}

void RecordingWindow::stopRecord()
{
    if(m_obs->isRecordingStart())
    {
        m_isRecordingStarted = false;
        m_obs->stopRecording();
    }
}

void RecordingWindow::saveConfig()
{
    //m_config.nameOfRecord = ui->nameEdit->text();
    m_config.frameRateInUse = ui->frameRateComboBox->currentText();
    m_config.bitRateInUse = ui->bitrateComboBox->currentText();
    m_config.savePath = ui->savePathEdit->text();
    m_config.startRecordShortCut = ui->StartShortCut->keySequence().toString();
    m_config.pauseRecordShortCut = ui->PauseShortCut->keySequence().toString();
    m_config.countDownEnable = ui->countDownCheckBox->isChecked();
    m_config.countDownSeconds = ui->countDownEdit->text().toInt();
    m_config.writeJson();
}

void RecordingWindow::rebuildBackgroundWindow()
{
    auto curScreen = QGuiApplication::screens().at(ui->screenDeviceComboBox->currentIndex());
    m_backgroundWindow->resetStatus(isFullScreenMode(), curScreen);
    m_backgroundWindow->showFullScreen();
}
