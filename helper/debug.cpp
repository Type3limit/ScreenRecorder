//
// Created by poocom on 2023/2/9.
//

#include "debug.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QScopedPointer>
#include <QMutexLocker>

const static QString LOG_NAME = "ScrrenRecorder";

const static int MAX_LOG_FILE_SIZE = 1024 * 1024 * 100; // log size in bytes
const static int MAX_LOG_FILE_COUNT = 9; // MAX_LOG_FILE_COUNT + one main file

static QScopedPointer<QFile> s_logFile;
static QMutex s_logFileMutex;

QString logDir()
{
    return QApplication::applicationDirPath() + "/log/";
}

void deleteOldLogs()
{
    QString appLogDir = logDir();

    QDir dir;
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Time | QDir::Reversed);
    dir.setPath(appLogDir);

    QFileInfoList list = dir.entryInfoList();
    if (list.size() <= MAX_LOG_FILE_COUNT) {
        return; //no files to delete
    }

    int deleteCount = list.size() - MAX_LOG_FILE_COUNT;
    for (int i = 0; i < deleteCount; i++) {
        QString path = list.at(i).absoluteFilePath();
        QFile file(path);
        file.remove();
    }
}

void rollLogFile()
{
    deleteOldLogs();

    QString appLogDir = logDir();

    QDir dir;
    if (!dir.exists(appLogDir)) {
        dir.mkpath(appLogDir);
    }

    QString rollingLogFileName = QString("%1/%2_%3_%4.log")
        .arg(appLogDir)
        .arg(LOG_NAME)
        .arg(QDate::currentDate().toString("yyyy_MM_dd"))
        .arg(QTime::currentTime().toString("hh_mm_ss_zzz"));

    if (s_logFile) {
        s_logFile->close();
        s_logFile->rename(rollingLogFileName);
    }

    s_logFile.reset(new QFile(rollingLogFileName));
    s_logFile->open(QFile::Append | QFile::Text);
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString dateTime = QDateTime::currentDateTime().toString(QString("[ yyyy-MM-dd HH:mm:ss.zzz ]"));

    QString message;
    QTextStream stream(&message);
    stream << dateTime;
    switch (type) {
        case QtDebugMsg:
            fprintf(stderr, "%s DEBUG: %s\n", dateTime.toStdString().c_str(), msg.toStdString().c_str());
            stream << " DEBUG " << msg << "\n";
            break;
        case QtInfoMsg:
            fprintf(stderr, "%s INFO: %s\n", dateTime.toStdString().c_str(), msg.toStdString().c_str());
            stream << " INFO " << msg << "\n";
            break;
        case QtWarningMsg:
            fprintf(stderr, "%s WARN: %s\n", dateTime.toStdString().c_str(), msg.toStdString().c_str());
            stream << " WARN " << msg << "\n";
            break;
        case QtCriticalMsg:
            fprintf(stderr, "%s CRITICAL: %s\n", dateTime.toStdString().c_str(), msg.toStdString().c_str());
            stream << " CRITICAL " << msg << "\n";
            break;
        case QtFatalMsg:
            fprintf(stderr, "%s Fatal: %s\n", dateTime.toStdString().c_str(), msg.toStdString().c_str());
            stream << " FATAL " << msg << "\n";
            break;
    }

    stream.flush();

    QMutexLocker locker(&s_logFileMutex);
    QTextStream out(s_logFile.data());
    out << message;
    out.flush();

    if (s_logFile->size() > MAX_LOG_FILE_SIZE) {
        rollLogFile();
    }
}

void initializeLogger()
{
    rollLogFile();

    qInstallMessageHandler(messageHandler);
}
