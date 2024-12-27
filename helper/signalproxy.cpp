//
// Created by 58226 on 2024/12/26.
//
#include "signalproxy.h"


SignalProxy SignalProxy::s_signalProxy;
SignalProxy *SignalProxy::instance()
{
    return &s_signalProxy;
}