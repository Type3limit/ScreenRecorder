
#include "debug.h"
#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QSharedMemory>
#include <QMessageBox>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QAction>
#include <QMenu>
#include <QDateTime>
#include <QLocalServer>
#include <QLocalSocket>
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
inline LONG WINAPI exceptionCallback(struct _EXCEPTION_POINTERS* exceptionInfo)
{
    QCoreApplication *app = QApplication::instance();

    QString savePath = app->applicationDirPath() + "/dump/";
    qDebug()<<"save path :"<<savePath;
    QDir dir(savePath);
    if (!dir.exists() && !dir.mkpath(savePath)) {
        app->exit(E_UNEXPECTED);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    savePath.append("assit_");
    savePath.append(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"));
    savePath.append(".dmp");

    HANDLE dump = CreateFileW(savePath.toStdWString().c_str(), GENERIC_WRITE,
                              0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == dump) {
        app->exit(E_UNEXPECTED);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    MINIDUMP_EXCEPTION_INFORMATION miniDumpExceptionInfo;
    miniDumpExceptionInfo.ExceptionPointers = exceptionInfo;
    miniDumpExceptionInfo.ThreadId = GetCurrentThreadId();
    miniDumpExceptionInfo.ClientPointers = TRUE;
    DWORD idProcess = GetCurrentProcessId();
    MiniDumpWriteDump(GetCurrentProcess(), idProcess, dump,
                      MiniDumpNormal/*(MINIDUMP_TYPE)flags*/, &miniDumpExceptionInfo, NULL, NULL);

    CloseHandle(dump);

    app->exit(E_UNEXPECTED);
    return EXCEPTION_EXECUTE_HANDLER;
}
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <QTranslator>
#include <QLibraryInfo>
#include "recordingwindow.h"
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QThread>
#include <usermessagebox.h>

class SingleInstanceApp : public QApplication {
public:
    SingleInstanceApp(int &argc, char **argv)
        : QApplication(argc, argv)
    {

        m_server.setMaxPendingConnections(50);
        if (!m_server.listen(QHostAddress::Any, 19990))
        {
            QTcpSocket socket;
            socket.connectToHost("localhost", 19990);
            socket.waitForConnected();
            socket.write("bringToFront");
            socket.waitForBytesWritten();
            socket.disconnectFromHost();
            exit(0);
        }

        // 等待客户端连接并处理请求
        connect(&m_server, &QTcpServer::newConnection, this, &SingleInstanceApp::onNewConnection);
    }
    void onNewConnection() {
        QTcpSocket *clientConnection = m_server.nextPendingConnection();
        connect(clientConnection, &QTcpSocket::disconnected, clientConnection, &QTcpSocket::deleteLater);

        // 通过信号唤醒已有实例的主窗口
        if (mainWindow) {
            mainWindow->raise();
            mainWindow->activateWindow();
            mainWindow->hide();
            mainWindow->show();
        }
    }

    void setWindow(QMainWindow* window)
    {
        mainWindow = window;
    }
private:
    QTcpServer m_server;
    QMainWindow* mainWindow = nullptr;

};

int main(int argc, char* argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
#endif
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));

    SingleInstanceApp app(argc, argv);

    app.setApplicationVersion(CURRENT_PROGRAM_VERSION);

    app.setApplicationName("Screen Recorder");

    app.setApplicationDisplayName(u8"屏幕录制");

    QThread::sleep(5);

    initializeLogger();

    auto paramStr = QUrl::fromPercentEncoding(argv[1]);
    auto params = paramStr.split(" ");
    qDebug() <<"App start with param:";
    for(int i = 0;i<params.count();i++)
    {
        qDebug() << "argv[" << i << "]:" << params[i];
    }


    //2024-11-19：避免从浏览器唤起时拉起程序路径不正确obs找不到文件的问题
    qDebug() << "current working path" << QCoreApplication::applicationDirPath();
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    QString portStr;
    QString server;
    QString token;
    if(params.count() >= 1)
    {
        portStr = params[0].split(":").last();
    }
    if(params.count() >=2)
    {
        server = QString(params[1]);
    }
    if(params.count() >=3)
    {
        token = QString(params[2]);
    }
    int port = portStr.isEmpty() ? -1 : portStr.toInt();


#ifdef _WIN32
    SetUnhandledExceptionFilter(exceptionCallback);
#endif

    qDebug() << "invoking with param: port:[" << portStr << "]" << " server:[" << server << "] token:[" << token << "]";

    RecordingWindow window(server, token, port, nullptr);
    app.setWindow(&window);
    window.show();

    qApp->setStyleSheet("QMenu,"
                        "QToolTip "
                        "{ padding:5px; color: #ffffff; background-color: #2F2F34; border: 1px solid 454549; }");


    int code =  QApplication::exec();

#ifdef WIN32
    _CrtDumpMemoryLeaks();
#endif
    return code;
}
