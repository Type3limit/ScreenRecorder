#include <QApplication>

#include <QTranslator>
#include <QLibraryInfo>
#include "recordingwindow.h"
#include "debug.h"
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

    QApplication a(argc, argv);

    initializeLogger();

    RecordingWindow window(nullptr);
    window.show();

    return QApplication::exec();
}
