//
// Created by 58226 on 2024/12/26.
//

#ifndef SIGNALPROXY_H
#define SIGNALPROXY_H

#include <QObject>

class SignalProxy:public QObject
{
  Q_OBJECT
  public:
    static SignalProxy *instance();

  public:
    signals:
       void recodingStarted();
       void recodingPaused();
       void recodingStoped();

       void requestStopRecording();
       void requestPauseRecording();
       void requestStartRecording();
private:
  explicit SignalProxy(QObject *parent = nullptr):QObject(parent){}
  static SignalProxy s_signalProxy;
};

#endif //SIGNALPROXY_H
