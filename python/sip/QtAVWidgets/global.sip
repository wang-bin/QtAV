%ModuleCode
#include <QtAVWidgets/global.h>
%End


namespace QtAV {

// Use custom code to allow QtAVWidgets.registerRenderers() instead of
// QtAVWidgets.Widgets.registerRenderers()
void registerRenderers();
%MethodCode
QtAV::Widgets::registerRenderers();
%End

VideoRendererId VideoRendererId_Widget;
VideoRendererId VideoRendererId_GraphicsItem;
VideoRendererId VideoRendererId_GLWidget;
VideoRendererId VideoRendererId_GDI;
VideoRendererId VideoRendererId_Direct2D;
VideoRendererId VideoRendererId_XV;
VideoRendererId VideoRendererId_X11;
VideoRendererId VideoRendererId_GLWidget2;
VideoRendererId VideoRendererId_OpenGLWidget;

void about();
void aboutFFmpeg();
void aboutQtAV();

};
