/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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

#ifndef QTAV_GLWIDGETRENDERER2_H
#define QTAV_GLWIDGETRENDERER2_H

#ifndef QT_NO_OPENGL
#include <QtAVWidgets/global.h>
#include <QtOpenGL/QGLWidget>
#include <QtAV/OpenGLRendererBase.h>

namespace QtAV {

class GLWidgetRenderer2Private;
/*!
 * \brief The GLWidgetRenderer2 class
 * Renderering video frames using GLSL. A more generic high level class OpenGLVideo is used internally.
 * TODO: for Qt5, no QtOpenGL, use QWindow instead.
 */
class Q_AVWIDGETS_EXPORT GLWidgetRenderer2 : public QGLWidget, public OpenGLRendererBase
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(GLWidgetRenderer2)
    Q_PROPERTY(qreal brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(qreal contrast READ contrast WRITE setContrast NOTIFY contrastChanged)
    Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY hueChanged)
    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
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
    GLWidgetRenderer2(QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0);
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
    void orientationChanged() Q_DECL_OVERRIDE;
    void videoRectChanged() Q_DECL_OVERRIDE;
    void videoFrameSizeChanged() Q_DECL_OVERRIDE;
    void backgroundColorChanged() Q_DECL_OVERRIDE;
protected:
    virtual void initializeGL() Q_DECL_OVERRIDE;
    virtual void paintGL() Q_DECL_OVERRIDE;
    virtual void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;  // not virtual in QGLWidget (Qt<5.5)
    virtual void showEvent(QShowEvent *) Q_DECL_OVERRIDE;
};
typedef GLWidgetRenderer2 VideoRendererGLWidget2;

} //namespace QtAV
#endif //QT_NO_OPENGL
#endif // QTAV_GLWIDGETRENDERER2_H
