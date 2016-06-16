/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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
#include "QtAVWidgets/QOpenGLWidget.h"
#include <QResizeEvent>
#include <QWindow>
// for dynamicgl. qglfunctions before qt5.3 does not have portable gl functions
#ifdef QT_OPENGL_DYNAMIC
#define DYGL(glFunc) QOpenGLContext::currentContext()->functions()->glFunc
#else
#define DYGL(glFunc) glFunc
#endif

namespace QtAV {

// TODO: is QOpenGLWidgetPaintDevice required?
class QOpenGLWidgetPaintDevice : public QOpenGLPaintDevice
{
public:
    QOpenGLWidgetPaintDevice(QOpenGLWidget *widget)
            : QOpenGLPaintDevice()
            , w(widget)
    { }
    void ensureActiveTarget() Q_DECL_OVERRIDE {
        if (QOpenGLContext::currentContext() != w->context()) {
            w->makeCurrent();
        }
    }
private:
    QOpenGLWidget *w;
};

QOpenGLWidget::QOpenGLWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_initialized(false)
    , m_fakeHidden(false)
    , m_context(0)
    , m_paintDevice(0)
{
    setAttribute(Qt::WA_NativeWindow); // ensure windowHandle() is not null
    // WA_PaintOnScreen: QWidget::paintEngine: Should no longer be called.  This flag is only supported on X11 and it disables double buffering
    //setAttribute(Qt::WA_PaintOnScreen); // enforce native window, so windowHandle() is not null
    setAttribute(Qt::WA_NoSystemBackground);
    //setAutoFillBackground(true); // for compatibility
    // FIXME: why setSurfaceType crash?
    //windowHandle()->setSurfaceType(QWindow::OpenGLSurface);
}

QOpenGLWidget::~QOpenGLWidget()
{
    delete m_paintDevice;
}

void QOpenGLWidget::setFormat(const QSurfaceFormat &format)
{
    m_requestedFormat = format;
}

QSurfaceFormat QOpenGLWidget::format() const
{
    return m_requestedFormat;
}

bool QOpenGLWidget::isValid() const
{
    return m_initialized && m_context->isValid();
}

void QOpenGLWidget::makeCurrent()
{
    if (!m_initialized) {
        qWarning("QOpenGLWidget: Cannot make uninitialized widget current");
        return;
    }
    if (!windowHandle()) {
        qWarning("QOpenGLWidget: No window handle");
        return;
    }
    m_context->makeCurrent((QSurface*)windowHandle());
}

void QOpenGLWidget::doneCurrent()
{
    if (!m_initialized)
        return;
    m_context->doneCurrent();
}

QOpenGLContext *QOpenGLWidget::context() const
{
    return m_context;
}

QPaintDevice* QOpenGLWidget::redirected(QPoint *offset) const
{
    Q_UNUSED(offset);
    // TODO: check backing store like Qt does
    return m_paintDevice;
}

void QOpenGLWidget::initializeGL()
{
}
void QOpenGLWidget::paintGL()
{
}

void QOpenGLWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void QOpenGLWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    if (!m_initialized)
        return;
    if (updatesEnabled())
        render();
}

void QOpenGLWidget::resizeEvent(QResizeEvent *e)
{
    if (e->size().isEmpty()) {
        m_fakeHidden = true;
        return;
    }
    m_fakeHidden = false;
    initialize();
    if (!m_initialized)
        return;
    //recreateFbo();
    resizeGL(width(), height());
    invokeUserPaint();
    //resolveSamples();
}

void QOpenGLWidget::initialize()
{
    if (m_initialized)
        return;

    QWindow *win = windowHandle();
    if (!win) {
        qWarning("QOpenGLWidget: No window handle");
        return;
    }
    m_context = new QOpenGLContext(this);
    // TODO: shareContext()
    m_context->setFormat(m_requestedFormat);
    if (!m_context->create()) {
        qWarning("QOpenGLWidget: Failed to create context");
        return;
    }
    //m_context = QOpenGLContext::currentContext();
    if (!m_context) {
        qWarning("QOpenGLWidget: QOpenGLContext is null");
        return;
    }
    if (!m_context->makeCurrent(win)) {
        qWarning("QOpenGLWidget: Failed to make context current");
        return;
    }
    m_paintDevice = new QOpenGLWidgetPaintDevice(this);
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    m_paintDevice->setSize(size() * devicePixelRatio());
    m_paintDevice->setDevicePixelRatio(devicePixelRatio());
#else
    m_paintDevice->setSize(size());
#endif
    m_initialized = true;
    initializeGL();
}

void QOpenGLWidget::render()
{
    if (m_fakeHidden || !m_initialized)
        return;
    // QOpenGLContext::swapBuffers() called with non-exposed window, behavior is undefined
    QWindow *win = windowHandle();
    if (!win || !win->isExposed())
        return;
    makeCurrent();
    invokeUserPaint();
    m_context->swapBuffers(win);
}

void QOpenGLWidget::invokeUserPaint()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 1 , 0)
    DYGL(glViewport(0, 0, width()*devicePixelRatio(), height()*devicePixelRatio()));
#else
    DYGL(glViewport(0, 0, width(), height()));
#endif
    paintGL();
    DYGL(glFlush());
}
} //namespace QtAV
