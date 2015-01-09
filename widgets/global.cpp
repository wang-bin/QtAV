/******************************************************************************
    VideoRendererTypes: type id and manually id register function
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

#include "QtAVWidgets/global.h"

#include <QBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextBrowser>

#include "QtAV/VideoRendererTypes.h"
#include "QtAV/private/prepost.h"
#include "QtAV/WidgetRenderer.h"
#include "QtAV/GraphicsItemRenderer.h"
#if QTAV_HAVE(GL)
#include "QtAV/GLWidgetRenderer2.h"
#endif //QTAV_HAVE(GL)
#if QTAV_HAVE(GL1)
#include "QtAV/GLWidgetRenderer.h"
#endif //QTAV_HAVE(GL1)
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
#include "QtAV/OpenGLWidgetRenderer.h"
#endif
#include "QtAV/private/factory.h"
#include "QtAV/private/mkid.h"

namespace QtAV {

VideoRendererId VideoRendererId_Widget = mkid::id32base36_6<'W', 'i', 'd', 'g', 'e', 't'>::value;
VideoRendererId VideoRendererId_OpenGLWidget = mkid::id32base36_6<'Q', 'O', 'G', 'L', 'W', 't'>::value;
VideoRendererId VideoRendererId_GLWidget2 = mkid::id32base36_6<'Q', 'G', 'L', 'W', 't', '2'>::value;
VideoRendererId VideoRendererId_GLWidget = mkid::id32base36_6<'Q', 'G', 'L', 'W', 't', '1'>::value;
VideoRendererId VideoRendererId_GraphicsItem = mkid::id32base36_6<'Q', 'G', 'r', 'a', 'p', 'h'>::value;
VideoRendererId VideoRendererId_GDI = mkid::id32base36_3<'G', 'D', 'I'>::value;
VideoRendererId VideoRendererId_Direct2D = mkid::id32base36_3<'D', '2', 'D'>::value;
VideoRendererId VideoRendererId_XV = mkid::id32base36_6<'X', 'V', 'i', 'd', 'e', 'o'>::value;

//QPainterRenderer is abstract. So can not register(operator new will needed)
FACTORY_REGISTER_ID_AUTO(VideoRenderer, Widget, "QWidegt")

void RegisterVideoRendererWidget_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, Widget, "QWidegt")
}

FACTORY_REGISTER_ID_AUTO(VideoRenderer, GraphicsItem, "QGraphicsItem")

void RegisterVideoRendererGraphicsItem_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, GraphicsItem, "QGraphicsItem")
}

VideoRendererId WidgetRenderer::id() const
{
    return VideoRendererId_Widget;
}

VideoRendererId GraphicsItemRenderer::id() const
{
    return VideoRendererId_GraphicsItem;
}

#if QTAV_HAVE(GL)
#if QTAV_HAVE(GL1)
FACTORY_REGISTER_ID_AUTO(VideoRenderer, GLWidget, "QGLWidegt")

void RegisterVideoRendererGLWidget_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, GLWidget, "QGLWidegt")
}
VideoRendererId GLWidgetRenderer::id() const
{
    return VideoRendererId_GLWidget;
}
#endif //QTAV_HAVE(GL1)
FACTORY_REGISTER_ID_AUTO(VideoRenderer, GLWidget2, "QGLWidegt2")

void RegisterVideoRendererGLWidget2_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, GLWidget2, "QGLWidegt2")
}

VideoRendererId GLWidgetRenderer2::id() const
{
    return VideoRendererId_GLWidget2;
}
#endif //QTAV_HAVE(GL)
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
FACTORY_REGISTER_ID_AUTO(VideoRenderer, OpenGLWidget, "OpenGLWidget")

void RegisterVideoRendererOpenGLWidget_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, OpenGLWidget, "OpenGLWidget")
}

VideoRendererId OpenGLWidgetRenderer::id() const
{
    return VideoRendererId_OpenGLWidget;
}
#endif

extern void RegisterVideoRendererGDI_Man();
extern void RegisterVideoRendererDirect2D_Man();
extern void RegisterVideoRendererXV_Man();

namespace Widgets {
void registerRenderers()
{
    RegisterVideoRendererWidget_Man();
    RegisterVideoRendererGraphicsItem_Man();
#if QTAV_HAVE(GL)
    RegisterVideoRendererGLWidget2_Man();
#endif //QTAV_HAVE(GL)
#if QTAV_HAVE(GL1)
    RegisterVideoRendererGLWidget_Man();
#endif //QTAV_HAVE(GL1)
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    RegisterVideoRendererOpenGLWidget_Man();
#endif
#if QTAV_HAVE(GDIPLUS)
    RegisterVideoRendererGDI_Man();
#endif //QTAV_HAVE(GDIPLUS)
#if QTAV_HAVE(DIRECT2D)
    RegisterVideoRendererDirect2D_Man();
#endif //QTAV_HAVE(DIRECT2D)
#if QTAV_HAVE(XV)
    RegisterVideoRendererXV_Man();
#endif //QTAV_HAVE(XV)
}
} //namespace Widgets

void about() {
    //we should use new because a qobject will delete it's children
    QTextBrowser *viewQtAV = new QTextBrowser;
    QTextBrowser *viewFFmpeg = new QTextBrowser;
    viewQtAV->setOpenExternalLinks(true);
    viewFFmpeg->setOpenExternalLinks(true);
    viewQtAV->setHtml(aboutQtAV_HTML());
    viewFFmpeg->setHtml(aboutFFmpeg_HTML());
    QTabWidget *tab = new QTabWidget;
    tab->addTab(viewQtAV, "QtAV");
    tab->addTab(viewFFmpeg, "FFmpeg");
    QPushButton *btn = new QPushButton(QObject::tr("Ok"));
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(btn);
    QDialog dialog;
    dialog.setWindowTitle(QObject::tr("About") + "  QtAV");
    QVBoxLayout *layout = new QVBoxLayout;
    dialog.setLayout(layout);
    layout->addWidget(tab);
    layout->addLayout(btnLayout);
    QObject::connect(btn, SIGNAL(clicked()), &dialog, SLOT(accept()));
    dialog.exec();
}

void aboutFFmpeg()
{
    QMessageBox::about(0, QObject::tr("About FFmpeg"), aboutFFmpeg_HTML());
}

void aboutQtAV()
{
    QMessageBox::about(0, QObject::tr("About QtAV"), aboutQtAV_HTML());
}
}//namespace QtAV
