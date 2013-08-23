#include "ScreenSaver.h"

#include <QAbstractEventDispatcher>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QAbstractNativeEventFilter>
#endif

#ifdef Q_OS_WIN
#include <windows.h>

class ScreenSaverEventFilter
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        : public QAbstractNativeEventFilter
#endif
{
public:
    //screensaver is global
    static ScreenSaverEventFilter& instance() {
        static ScreenSaverEventFilter sSSEF;
        return sSSEF;
    }
    void enable(bool yes = true) {
        if (yes) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
            mLastEventFilter = QAbstractEventDispatcher::instance()->setEventFilter(eventFilter);
#else
            QAbstractEventDispatcher::instance()->installNativeEventFilter(this);
#endif
        } else {
            if (!QAbstractEventDispatcher::instance())
                return;
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
            mLastEventFilter = QAbstractEventDispatcher::instance()->setEventFilter(mLastEventFilter);
#else
            QAbstractEventDispatcher::instance()->removeNativeEventFilter(this);
#endif
        }
    }
    void disable(bool yes = true) {
        enable(!yes);
    }

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) {
        Q_UNUSED(eventType);
        MSG* msg = static_cast<MSG*>(message);
        //qDebug("ScreenSaverEventFilter: %p", msg->message);
        if (WM_DEVICECHANGE == msg->message) {
            qDebug("~~~~~~~~~~device event");
            /*if (msg->wParam == DBT_DEVICEREMOVECOMPLETE) {
                qDebug("Remove device");
            }*/

        }
        if (msg->message == WM_SYSCOMMAND
                && ((msg->wParam & 0xFFF0) == SC_SCREENSAVE
                    || (msg->wParam & 0xFFF0) == SC_MONITORPOWER)
        ) {
            qDebug("WM_SYSCOMMAND SC_SCREENSAVE SC_MONITORPOWER");
            if (result) {
                //*result = 0; //why crash?
            }
            return true;
        }
        return false;
    }
private:
    ScreenSaverEventFilter() {
        enable();
    }
    ~ScreenSaverEventFilter() {
        enable(false);
    }
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    static QAbstractEventDispatcher::EventFilter mLastEventFilter;
    static bool eventFilter(void* message) {
        return ScreenSaverEventFilter::instance().nativeEventFilter("windows_MSG", message, 0);
    }
#endif
};
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
QAbstractEventDispatcher::EventFilter ScreenSaverEventFilter::mLastEventFilter = 0;
#endif
#endif //Q_OS_WIN


ScreenSaver& ScreenSaver::instance()
{
    static ScreenSaver sSS;
    return sSS;
}

ScreenSaver::ScreenSaver()
{
#ifdef Q_OS_WIN
    ScreenSaverEventFilter::instance().enable();
    lowpower = poweroff = screensaver = 0;
    state_saved = false;
    modified = false;
#endif
    retrieveState();
}

ScreenSaver::~ScreenSaver()
{
    restoreState();
}

//http://msdn.microsoft.com/en-us/library/windows/desktop/ms724947%28v=vs.85%29.aspx
//http://msdn.microsoft.com/en-us/library/windows/desktop/aa373208%28v=vs.85%29.aspx
/* TODO:
 * SystemParametersInfo will change system wild settings. An application level solution is better. Use native event
 * SPI_SETSCREENSAVETIMEOUT?
 * SPI_SETLOWPOWERTIMEOUT, SPI_SETPOWEROFFTIMEOUT for 32bit
 */
bool ScreenSaver::enable(bool yes)
{
    bool rv = false;
#ifdef Q_OS_WIN
    ScreenSaverEventFilter::instance().enable(yes);
    return true;
#if 0
    //TODO: ERROR_OPERATION_IN_PROGRESS
    if (yes)
        rv = SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, 1, NULL, 0);
    else
        rv = SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, 0, NULL, 0);
#else
    /*
    static EXECUTION_STATE sLastState = 0;
    if (yes) {
        sLastState = SetThreadExecutionState(ES_DISPLAY_REQUIRED);
    } else {
        if (sLastState)
            sLastState = SetThreadExecutionState(sLastState);
    }
    rv = sLastState != 0;
    */
    if (yes) {
        rv = restoreState();
    } else {
        //if (QSysInfo::WindowsVersion < QSysInfo::WV_VISTA) {
            // Not supported on Windows Vista
            SystemParametersInfo(SPI_SETLOWPOWERTIMEOUT, 0, NULL, 0);
            SystemParametersInfo(SPI_SETPOWEROFFTIMEOUT, 0, NULL, 0);
        //}
        rv = SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, 0, NULL, 0);
        modified = true;
    }
#endif
#else
#endif
    if (!rv) {
        qWarning("Failed to enable screen saver (%d)", yes);
    } else {
        qDebug("Succeed to enable screen saver (%d)", yes);
    }
    return rv;
}


bool ScreenSaver::retrieveState() {
    bool rv = false;
#ifdef Q_OS_WIN
    qDebug("WinScreenSaver::retrieveState");
    if (!state_saved) {
        //if (QSysInfo::WindowsVersion < QSysInfo::WV_VISTA) {
            // Not supported on Windows Vista
            SystemParametersInfo(SPI_GETLOWPOWERTIMEOUT, 0, &lowpower, 0);
            SystemParametersInfo(SPI_GETPOWEROFFTIMEOUT, 0, &poweroff, 0);
        //}
        rv = SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &screensaver, 0);
        state_saved = true;
        qDebug("WinScreenSaver::retrieveState: lowpower: %d, poweroff: %d, screensaver: %d", lowpower, poweroff, screensaver);
    } else {
        rv = true;
        qDebug("WinScreenSaver::retrieveState: state already saved previously, doing nothing");
    }
#endif
    return rv;
}

bool ScreenSaver::restoreState() {
    bool rv = false;
#ifdef Q_OS_WIN
    if (!modified) {
        qDebug("WinScreenSaver::restoreState: state did not change, doing nothing");
        return true;
    }
    if (state_saved) {
        //if (QSysInfo::WindowsVersion < QSysInfo::WV_VISTA) {
            // Not supported on Windows Vista
            SystemParametersInfo(SPI_SETLOWPOWERTIMEOUT, lowpower, NULL, 0);
            SystemParametersInfo(SPI_SETPOWEROFFTIMEOUT, poweroff, NULL, 0);
        //}
        rv = SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, screensaver, NULL, 0);
        qDebug("WinScreenSaver::restoreState: lowpower: %d, poweroff: %d, screensaver: %d", lowpower, poweroff, screensaver);
    } else {
        qWarning("WinScreenSaver::restoreState: no data, doing nothing");
    }
#endif
    return rv;
}
