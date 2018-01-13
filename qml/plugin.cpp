/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2013)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#include <QtQml/QQmlExtensionPlugin>
#include <QtQml/qqml.h>
#include "QmlAV/QQuickItemRenderer.h"
#include "QmlAV/QmlAVPlayer.h"
#include "QmlAV/QuickFilter.h"
#include "QmlAV/QuickSubtitle.h"
#include "QmlAV/QuickSubtitleItem.h"
#include "QmlAV/MediaMetaData.h"
#include "QmlAV/QuickVideoPreview.h"
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
#include "QmlAV/QuickFBORenderer.h"
#endif

inline void initResources() {
#ifdef BUILD_QMLAV_STATIC
    Q_INIT_RESOURCE(libQmlAV);
#endif
}

namespace QtAV {

class QtAVQmlPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")
public:
    QtAVQmlPlugin() {
        initResources();
    }
    void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtAV"));
        qmlRegisterType<QQuickItemRenderer>(uri, 1, 3, "VideoOutput");
        qmlRegisterType<QmlAVPlayer>(uri, 1, 3, "AVPlayer");
        qmlRegisterType<QmlAVPlayer>(uri, 1, 3, "MediaPlayer");
        qmlRegisterType<QuickSubtitle>(uri, 1, 4, "Subtitle");
        qmlRegisterType<QuickSubtitleItem>(uri, 1, 4, "SubtitleItem");
        qmlRegisterType<QuickVideoPreview>(uri, 1, 4, "VideoPreview");
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
        qmlRegisterType<QuickFBORenderer>(uri, 1, 5, "VideoOutput2");
#endif
        qmlRegisterUncreatableType<VideoCapture>(uri, 1, 6, "VideoCapture", trUtf8("VideoCapture is provided by MediaPlayer"));
        qmlRegisterType<MediaMetaData>();

        // FIXME: if version is 2.x, some qtav types will be undefined, why?
        // 1.7
        qmlRegisterType<QuickAudioFilter>(uri, 1, 7, "AudioFilter");
        qmlRegisterType<QuickVideoFilter>(uri, 1, 7, "VideoFilter");
        qmlRegisterType<QtAV::DynamicShaderObject>(uri, 1, 7, "Shader");
    }
};
} //namespace QtAV

#include "plugin.moc"
