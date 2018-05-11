%ModuleCode
#include <QtAV/QtAV_Global.h>
%End

namespace QtAV {

unsigned Version();
%MethodCode
::QtAV_Version();
%End

QString Version_String();
%MethodCode
::QtAV_Version_String();
%End

QString Version_String_Long();
%MethodCode
::QtAV_Version_String_Long();
%End

enum LogLevel {
    LogOff,
    LogDebug, // log all
    LogWarning, // log warning, critical, fatal
    LogCritical, // log critical, fatal
    LogFatal, // log fatal
    LogAll
};
QString aboutFFmpeg_PlainText();
QString aboutFFmpeg_HTML();
QString aboutQtAV_PlainText();
QString aboutQtAV_HTML();
void setLogLevel(LogLevel value);
LogLevel logLevel();
//void setFFmpegLogHandler(void(*)(void *, int, const char *, va_list));
void setFFmpegLogLevel(const QByteArray& level);

QString avformatOptions();
QString avcodecOptions();

enum MediaStatus
{
    UnknownMediaStatus,
    NoMedia,
    LoadingMedia, // when source is set
    LoadedMedia, // if auto load and source is set. player is stopped state
    StalledMedia, // insufficient buffering or other interruptions (timeout, user interrupt)
    BufferingMedia, // NOT IMPLEMENTED
    BufferedMedia, // when playing //NOT IMPLEMENTED
    EndOfMedia, // Playback has reached the end of the current media. The player is in the StoppedState.
    InvalidMedia // what if loop > 0 or stopPosition() is not mediaStopPosition()?
};

enum BufferMode {
    BufferTime,
    BufferBytes,
    BufferPackets
};

enum MediaEndActionFlag {
    MediaEndAction_Default,
    MediaEndAction_KeepDisplay,
    MediaEndAction_Pause
};

//Q_DECLARE_FLAGS(MediaEndAction, MediaEndActionFlag)

enum SeekUnit {
    SeekByTime, // only this is supported now
    SeekByByte,
    SeekByFrame
};
enum SeekType {
    AccurateSeek, // slow
    KeyFrameSeek, // fast
    AnyFrameSeek
};


enum ColorSpace {
    ColorSpace_Unknown,
    ColorSpace_RGB,
    ColorSpace_GBR, // for planar gbr format(e.g. video from x264) used in glsl
    ColorSpace_BT601,
    ColorSpace_BT709,
    ColorSpace_XYZ
};


enum ColorRange {
    ColorRange_Unknown,
    ColorRange_Limited, // TV, MPEG
    ColorRange_Full     // PC, JPEG
};


enum SurfaceType {
    HostMemorySurface,
    GLTextureSurface,
    SourceSurface,
    UserSurface = 0xffff
};

};
