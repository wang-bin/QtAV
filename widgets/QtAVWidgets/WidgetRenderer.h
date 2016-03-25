/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

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

#ifndef QTAV_WIDGETRENDERER_H
#define QTAV_WIDGETRENDERER_H

#include <QtAVWidgets/global.h>
#include <QtAV/QPainterRenderer.h>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtWidgets/QWidget>
#else
#include <QtGui/QWidget>
#endif

namespace QtAV {
class WidgetRendererPrivate;
class Q_AVWIDGETS_EXPORT WidgetRenderer : public QWidget, public QPainterRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(WidgetRenderer)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QRectF regionOfInterest READ regionOfInterest WRITE setRegionOfInterest NOTIFY regionOfInterestChanged)
    Q_PROPERTY(qreal sourceAspectRatio READ sourceAspectRatio NOTIFY sourceAspectRatioChanged)
    Q_PROPERTY(qreal outAspectRatio READ outAspectRatio WRITE setOutAspectRatio NOTIFY outAspectRatioChanged)
    //fillMode
    // TODO: how to use enums in base class as property or Q_ENUM
    Q_PROPERTY(OutAspectRatioMode outAspectRatioMode READ outAspectRatioMode WRITE setOutAspectRatioMode NOTIFY outAspectRatioModeChanged)
    Q_ENUMS(OutAspectRatioMode)
    Q_PROPERTY(int orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(QRect videoRect READ videoRect NOTIFY videoRectChanged)
    Q_PROPERTY(QSize videoFrameSize READ videoFrameSize NOTIFY videoFrameSizeChanged)
    Q_ENUMS(Quality)
public:
    explicit WidgetRenderer(QWidget *parent = 0, Qt::WindowFlags f = 0);
    virtual VideoRendererId id() const Q_DECL_OVERRIDE;
    virtual QWidget* widget() Q_DECL_OVERRIDE { return this; }
Q_SIGNALS:
    void sourceAspectRatioChanged(qreal value) Q_DECL_OVERRIDE Q_DECL_FINAL;
    void regionOfInterestChanged() Q_DECL_OVERRIDE;
    void outAspectRatioChanged() Q_DECL_OVERRIDE;
    void outAspectRatioModeChanged() Q_DECL_OVERRIDE;
    void brightnessChanged(qreal value) Q_DECL_OVERRIDE;
    void contrastChanged(qreal) Q_DECL_OVERRIDE;
    void hueChanged(qreal) Q_DECL_OVERRIDE;
    void saturationChanged(qreal) Q_DECL_OVERRIDE;
    void backgroundColorChanged() Q_DECL_OVERRIDE;
    void orientationChanged() Q_DECL_OVERRIDE;
    void videoRectChanged() Q_DECL_OVERRIDE;
    void videoFrameSizeChanged() Q_DECL_OVERRIDE;
    QTAVWIDGETS_DEPRECATED void imageReady(); // add frameReady() in the future?
protected:
    bool receiveFrame(const VideoFrame& frame) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
    /*usually you don't need to reimplement paintEvent, just drawXXX() is ok. unless you want do all
     *things yourself totally*/
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

    bool onSetOrientation(int value) Q_DECL_OVERRIDE;
protected:
    WidgetRenderer(WidgetRendererPrivate& d, QWidget *parent, Qt::WindowFlags f);
};

typedef WidgetRenderer VideoRendererWidget;
} //namespace QtAV
#endif // QTAV_WIDGETRENDERER_H
