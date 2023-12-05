//
// Created by Type3Limit on 2023/11/28.
//

#ifndef QOPERATIONHELPER_H
#define QOPERATIONHELPER_H
#include <QWindow>
#include "graphics/graphics.h"
#ifdef Q_OS_UNIX
#include "obs-nix-platform.h"
#endif
#include <QColor>

static inline QSize GetPixelSize(const QWidget *widget)
{
    return widget->size() * widget->devicePixelRatioF();
}

inline bool QTToGSWindow(const QWindow *window, gs_window &gswindow)
{
    bool success = true;

#ifdef _WIN32
    gswindow.hwnd = reinterpret_cast<HWND>(window->winId());
#elif __APPLE__
    gswindow.view = (id)window->winId();
#else
    switch (obs_get_nix_platform()) {
    case OBS_NIX_PLATFORM_X11_GLX:
    case OBS_NIX_PLATFORM_X11_EGL:
        gswindow.id = window->winId();
        gswindow.display = obs_get_nix_platform_display();
        break;
#ifdef ENABLE_WAYLAND
    case OBS_NIX_PLATFORM_WAYLAND: {
            QPlatformNativeInterface *native =
                QGuiApplication::platformNativeInterface();
            gswindow.display =
                native->nativeResourceForWindow("surface", window);
            success = gswindow.display != nullptr;
            break;
    }
#endif
    default:
        success = false;
        break;
    }
#endif
    return success;
}

static inline void GetScaleAndCenterPos(int baseCX, int baseCY, int windowCX,
                    int windowCY, int &x, int &y,
                    float &scale)
{
    double windowAspect, baseAspect;
    int newCX, newCY;

    windowAspect = double(windowCX) / double(windowCY);
    baseAspect = double(baseCX) / double(baseCY);

    if (windowAspect > baseAspect) {
        scale = float(windowCY) / float(baseCY);
        newCX = int(double(windowCY) * baseAspect);
        newCY = windowCY;
    } else {
        scale = float(windowCX) / float(baseCX);
        newCX = windowCX;
        newCY = int(float(windowCX) / baseAspect);
    }

    x = windowCX / 2 - newCX / 2;
    y = windowCY / 2 - newCY / 2;
}

static inline long long color_to_int(const QColor &color)
{
    auto shift = [&](unsigned val, int shift) {
        return ((val & 0xff) << shift);
    };

    return shift(color.red(), 0) | shift(color.green(), 8) |
           shift(color.blue(), 16) | shift(color.alpha(), 24);
}

static inline QColor rgba_to_color(uint32_t rgba)
{
    return QColor::fromRgb(rgba & 0xFF, (rgba >> 8) & 0xFF,
                   (rgba >> 16) & 0xFF, (rgba >> 24) & 0xFF);
}
#endif //QOPERATIONHELPER_H
