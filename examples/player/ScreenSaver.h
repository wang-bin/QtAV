#ifndef SCREENSAVER_H
#define SCREENSAVER_H

#include <qglobal.h>

class ScreenSaver
{
public:
    static ScreenSaver& instance();
    ScreenSaver();
    ~ScreenSaver();
    bool enable(bool yes = true);
private:
    bool retrieveState();
    bool restoreState();
#ifdef Q_OS_WIN
    int lowpower, poweroff, screensaver;
    bool state_saved, modified;
#endif
};

#endif // SCREENSAVER_H
