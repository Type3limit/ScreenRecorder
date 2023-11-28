//
// Created by 58226 on 2023/11/22.
//

#ifndef CONFIG_H
#define CONFIG_H
#include <QJsonObject>
#include <QJsonDocument>
#include "extensionmethods.h"

using StrEx = ExtensionMethods::QStringExtension;

class Config
{
public:
    Config() { readJson(); }
    QString bitRateInUse;
    QString frameRateInUse;
    QString savePath;
    QString startRecordShortCut;
    QString pauseRecordShortCut;
    int tcpPort = 29989;
    bool countDownEnable = false;
    int countDownSeconds = 3;
    bool showPreviewWindow = false;

    bool isDefault() const
    {
        return (StrEx::isNullOrEmpty(bitRateInUse)
            && StrEx::isNullOrEmpty(frameRateInUse) && StrEx::isNullOrEmpty(savePath)
            && StrEx::isNullOrEmpty(startRecordShortCut) && StrEx::isNullOrEmpty(pauseRecordShortCut));
    }

    const QString ConfigPath = "./config.json";
    void writeJson()
    {
        QJsonObject obj = {
            {"bitRateInUse", bitRateInUse},
            {"frameRateInUse", frameRateInUse},
            {"savePath", savePath},
            {"startRecordShortCut", startRecordShortCut},
            {"pauseRecordShortCut", pauseRecordShortCut},
            {"countDownEnable", countDownEnable},
            {"countDownSeconds", countDownSeconds},
            {"tcpPort", tcpPort},
            {"showPreviewWindow", showPreviewWindow}
        };
        StrEx::writeAllText(ConfigPath, QJsonDocument(obj).toJson());
    }
    void readJson()
    {
        auto doc = QJsonDocument::fromJson(StrEx::readAllText(ConfigPath).toUtf8());
        if (doc.isEmpty())
            return;
        auto obj = doc.object();
        this->bitRateInUse = obj["bitRateInUse"].toString();
        this->frameRateInUse = obj["frameRateInUse"].toString();
        this->savePath = obj["savePath"].toString();
        this->startRecordShortCut = obj["startRecordShortCut"].toString();
        this->pauseRecordShortCut = obj["pauseRecordShortCut"].toString();
        this->countDownEnable = obj["countDownEnable"].toBool();
        this->countDownSeconds = obj["countDownSeconds"].toInt();
        this->tcpPort = obj["tcpPort"].toInt();
        this->showPreviewWindow = obj["showPreviewWindow"].toBool();
    }
};


#endif //CONFIG_H
