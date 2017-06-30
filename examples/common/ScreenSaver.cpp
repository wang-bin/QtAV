#include "ScreenSaver.h"
#include <QtCore/QTimerEvent>
#include <QtCore/QLibrary>
#ifdef Q_OS_LINUX
//#include <X11/Xlib.h>
#ifndef Success
#define Success 0
#endif
struct _XDisplay;
typedef struct _XDisplay Display;
typedef Display* (*fXOpenDisplay)(const char*/* display_name */);
typedef int (*fXCloseDisplay)(Display*/* display */);
typedef int (*fXSetScreenSaver)(Display*, int /* timeout */, int /* interval */,int /* prefer_blanking */, int /* allow_exposures */);
typedef int (*fXGetScreenSaver)(Display*, int* /* timeout_return */, int* /* interval_return */, int* /* prefer_blanking_return */, int* /* allow_exposures_return */);
typedef int (*fXResetScreenSaver)(Display*/* display */);
static fXOpenDisplay XOpenDisplay = 0;
static fXCloseDisplay XCloseDisplay = 0;
static fXSetScreenSaver XSetScreenSaver = 0;
static fXGetScreenSaver XGetScreenSaver = 0;
static fXResetScreenSaver XResetScreenSaver = 0;
static QLibrary xlib;
#endif //Q_OS_LINUX
#if defined(Q_OS_MAC) && !defined(Q_OS_IOS)
#include <Availability.h>
#  if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_8
//http://www.cocoachina.com/macdev/cocoa/2010/0201/453.html
#include <CoreServices/CoreServices.h>
#  else
/* MAC OSX 10.8+ has deprecated the UpdateSystemActivity stuff in favor of a new API.. */
#    include <IOKit/pwr_mgt/IOPMLib.h>
#  endif
#endif //Q_OS_MAC
#ifdef Q_OS_WIN
#include <QAbstractEventDispatcher>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QAbstractNativeEventFilter>
#endif
//mingw gcc4.4 EXECUTION_STATE
#ifdef __MINGW32__
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410
#endif //_WIN32_WINDOWS
#endif //__MINGW32__
#include <windows.h>
#define USE_NATIVE_EVENT 0

#if USE_NATIVE_EVENT
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
        if (!yes) {
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
            //qDebug("WM_SYSCOMMAND SC_SCREENSAVE SC_MONITORPOWER");
            if (result) {
                //*result = 0; //why crash?
            }
            return true;
        }
        return false;
    }
private:
    ScreenSaverEventFilter() {}
    ~ScreenSaverEventFilter() {}
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
#endif //USE_NATIVE_EVENT
#endif //Q_OS_WIN


ScreenSaver& ScreenSaver::instance()
{
    static ScreenSaver sSS;
    return sSS;
}

ScreenSaver::ScreenSaver()
{
    state_saved = false;
    modified = false;
#ifdef Q_OS_LINUX
    timeout = 0;
    interval = 0;
    preferBlanking = 0;
    allowExposures = 0;
    if (qgetenv("DISPLAY").isEmpty()) {
        isX11 = false;
    } else {
        xlib.setFileName(QString::fromLatin1("libX11.so"));
        isX11 = xlib.load();
        // meego only has libX11.so.6, libX11.so.6.x.x
        if (!isX11) {
            xlib.setFileName(QString::fromLatin1("libX11.so.6"));
            isX11 = xlib.load();
        }
        if (!isX11) {
            qDebug("open X11 so failed: %s", xlib.errorString().toUtf8().constData());
        } else {
            XOpenDisplay = (fXOpenDisplay)xlib.resolve("XOpenDisplay");
            XCloseDisplay = (fXCloseDisplay)xlib.resolve("XCloseDisplay");
            XSetScreenSaver = (fXSetScreenSaver)xlib.resolve("XSetScreenSaver");
            XGetScreenSaver = (fXGetScreenSaver)xlib.resolve("XGetScreenSaver");
            XResetScreenSaver = (fXResetScreenSaver)xlib.resolve("XResetScreenSaver");
        }
        isX11 = XOpenDisplay && XCloseDisplay && XSetScreenSaver && XGetScreenSaver && XResetScreenSaver;
    }
#endif //Q_OS_LINUX
    ssTimerId = 0;
    osxIOPMAssertionId = 0U; // mac >=10.8 only
    retrieveState();
}

ScreenSaver::~ScreenSaver()
{
    restoreState();
#ifdef Q_OS_LINUX
    if (xlib.isLoaded())
        xlib.unload();
#elif defined(Q_OS_MAC) && !defined(Q_OS_IOS)
#  if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_8
    if (osxIOPMAssertionId) {
        IOPMAssertionRelease((IOPMAssertionID)osxIOPMAssertionId);
        osxIOPMAssertionId = 0U;
    }
#  endif
#endif
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
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
#if USE_NATIVE_EVENT
    ScreenSaverEventFilter::instance().enable(yes);
    modified = true;
    rv = true;
    return true;
#else
    /*
        int val; //SPI_SETLOWPOWERTIMEOUT, SPI_SETPOWEROFFTIMEOUT. SPI_SETSCREENSAVETIMEOUT
        if ( SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &val, 0)) {
            SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, val, NULL, 0);
        }
     */
    //http://msdn.microsoft.com/en-us/library/aa373208%28VS.85%29.aspx
    static EXECUTION_STATE sLastState = 0;
    if (!yes) {
        //Calling SetThreadExecutionState without ES_CONTINUOUS simply resets the idle timer; to keep the display or system in the working state, the thread must call SetThreadExecutionState periodically
        //ES_CONTINUOUS: Informs the system that the state being set should remain in effect until the next call that uses ES_CONTINUOUS and one of the other state flags is cleared.
        sLastState = SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
    } else {
        if (sLastState)
            sLastState = SetThreadExecutionState(sLastState|ES_CONTINUOUS);
    }
    rv = sLastState != 0;
    modified = true;
#endif //USE_NATIVE_EVENT
#endif //defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
#ifdef Q_OS_LINUX
    if (isX11) {
        Display *display = XOpenDisplay(0);
        // -1: restore default. 0: disable
        int ret = 0;
        if (yes)
            ret = XSetScreenSaver(display, -1, interval, preferBlanking, allowExposures);
        else
            ret = XSetScreenSaver(display, 0, interval, preferBlanking/*DontPreferBlanking*/, allowExposures);
        //TODO: why XSetScreenSaver return 1? now use XResetScreenSaver to workaround
        ret = XResetScreenSaver(display);
        XCloseDisplay(display);
        rv = ret==Success;
        qDebug("ScreenSaver::enable %d, ret %d timeout origin: %d", yes, ret, timeout);
    }
    modified = true;
    if (!yes) {
        if (ssTimerId <= 0) {
            ssTimerId = startTimer(1000 * 60);
        }
    } else {
        if (ssTimerId)
            killTimer(ssTimerId);
    }
    rv = true;
    modified = true;
#endif //Q_OS_LINUX
#if defined(Q_OS_MAC) && !defined(Q_OS_IOS)
    if (!yes) {
        if (ssTimerId <= 0) {
            ssTimerId = startTimer(1000 * 60);
        }
    } else {
        if (ssTimerId)
            killTimer(ssTimerId);
    }
    rv = true;
    modified = true;
#endif //Q_OS_MAC
    if (!rv) {
        qWarning("Failed to enable screen saver (%d)", yes);
    } else {
        qDebug("Succeed to enable screen saver (%d)", yes);
    }
    return rv;
}

void ScreenSaver::enable()
{
    enable(true);
}

void ScreenSaver::disable()
{
    enable(false);
}

bool ScreenSaver::retrieveState() {
    bool rv = false;
    qDebug("ScreenSaver::retrieveState");
    if (!state_saved) {
#ifdef Q_OS_LINUX
        if (isX11) {
            Display *display = XOpenDisplay(0);
            XGetScreenSaver(display, &timeout, &interval, &preferBlanking, &allowExposures);
            XCloseDisplay(display);
            qDebug("ScreenSaver::retrieveState timeout: %d, interval: %d, preferBlanking:%d, allowExposures:%d", timeout, interval, preferBlanking, allowExposures);
            state_saved = true;
            rv = true;
        }
#endif //Q_OS_LINUX
    } else {
        qDebug("ScreenSaver::retrieveState: state already saved previously, doing nothing");
    }
    return rv;
}

bool ScreenSaver::restoreState() {
    bool rv = false;
    if (!modified) {
        qDebug("ScreenSaver::restoreState: state did not change, doing nothing");
        return true;
    }
    if (state_saved) {
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
#if USE_NATIVE_EVENT
        ScreenSaverEventFilter::instance().enable();
        rv = true;
#else
        SetThreadExecutionState(ES_CONTINUOUS);
#endif //USE_NATIVE_EVENT
#endif //defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
#ifdef Q_OS_LINUX
        if (isX11) {
            Display *display = XOpenDisplay(0);
            // -1: restore default. 0: disable
            XSetScreenSaver(display, timeout, interval, preferBlanking, allowExposures);
            XCloseDisplay(display);
            rv = true;
        }
#endif //Q_OS_LINUX
    } else {
        qWarning("ScreenSaver::restoreState: no data, doing nothing");
    }
    return rv;
}

void ScreenSaver::timerEvent(QTimerEvent *e)
{
    if (e->timerId() != ssTimerId)
        return;
#if defined(Q_OS_MAC) && !defined(Q_OS_IOS)
#  if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_8
    UpdateSystemActivity(OverallAct);
#  else // OSX >= 10.8, use new API
    IOPMAssertionID assertionId = osxIOPMAssertionId;
    IOReturn r = IOPMAssertionDeclareUserActivity(CFSTR("QtAVScreenSaver"),kIOPMUserActiveLocal,&assertionId);
    if (r == kIOReturnSuccess) {
        osxIOPMAssertionId = assertionId;
    }
#  endif
    return;
#endif //Q_OS_MAC
#ifdef Q_OS_LINUX
    if (!isX11)
        return;
    Display *display = XOpenDisplay(0);
    XResetScreenSaver(display);
    XCloseDisplay(display);
#endif //Q_OS_LINUX
}
