// #include <iostream>
// #include <windows.h>
//
// void setWorkingDirectoryToAppPath() {
//     wchar_t appPath[MAX_PATH];
//     if (GetModuleFileNameW(nullptr, appPath, MAX_PATH)) {
//         // 移除文件名，保留路径
//         wchar_t *lastSlash = wcsrchr(appPath, L'\\');
//         if (lastSlash) {
//             *lastSlash = L'\0';
//             if (!SetCurrentDirectoryW(appPath)) {
//                 std::wcerr << L"Failed to set working directory. Error: " << GetLastError() << std::endl;
//             }
//         }
//     } else {
//         std::wcerr << L"Failed to get module file name. Error: " << GetLastError() << std::endl;
//     }
// }


#include <QApplication>

#include <QTranslator>
#include <QLibraryInfo>
#include "recordingwindow.h"
#include "debug.h"
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QThread>

#include "commandlinedefinations.h"


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

    QApplication app(argc, argv);

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
    //setWorkingDirectoryToAppPath();
    qDebug() << "current working path" << QCoreApplication::applicationDirPath();
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    // // 创建解析器
    // QCommandLineParser parser;
    // parser.setApplicationDescription("screen recorder helper");
    // parser.addHelpOption(); // 添加 --help 选项
    //
    // QCommandLineOption portOption(
    //     QStringList() << TCP_PORT_SHORT << TCP_PORT, // 参数名称 (支持短选项 -p 和长选项 --port)
    //     u8"tcp notice when recoding finished", // 参数描述
    //     u8"port", // 参数值的占位符
    //     u8"29989" // 默认值
    // );
    //
    // QCommandLineOption serverOption(
    //     QStringList() << SERVER_SHORT << SERVER,
    //     u8"ip Address(must has prefix [http(s)://]) for no-secret login",
    //     u8"address",
    //     u8""
    // );
    //
    // QCommandLineOption tokenOption(
    //     QStringList() << TOKEN_SHORT << TOKEN,
    //     u8"token for no-secret login",
    //     u8"token",
    //     u8""
    // );
    //
    // parser.addOption(portOption);
    // parser.addOption(serverOption);
    // parser.addOption(tokenOption);
    // parser.process(app);
    //
    // auto portStr = parser.value(portOption);
    // auto server = parser.value(serverOption);
    // auto token = parser.value(tokenOption);
    // int port = portStr.isEmpty() ? -1 : portStr.toInt();

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

    qDebug() << "invoking with param: port:[" << portStr << "]" << " server:[" << server << "] token:[" << token << "]";

    RecordingWindow window(server, token, port, nullptr);
    window.show();

    return QApplication::exec();
}
