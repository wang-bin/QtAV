#ifndef SCREENSAVER_H
#define SCREENSAVER_H

#include "common.h"
#include <QtCore/QObject>

// TODO: read QtSystemInfo.ScreenSaver

class COMMON_EXPORT ScreenSaver : QObject
{
    Q_OBJECT
public:
    static ScreenSaver& instance();
    ScreenSaver();
    ~ScreenSaver();
    bool enable(bool yes);
public slots:
    void enable();
    void disable();
private:
    bool retrieveState();
    bool restoreState();
#ifdef Q_OS_WIN
    int lowpower, poweroff, screensaver;
    bool state_saved, modified;
#endif
};

#endif // SCREENSAVER_H
