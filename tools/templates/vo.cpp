
#include "QtAV/%CLASS%.h"
#include "private/VideoRenderer_p.h"
#include <QResizeEvent>

namespace QtAV {

class %CLASS%Private : public VideoRendererPrivate
{
public:
    DPTR_DECLARE_PUBLIC(%CLASS%)

    %CLASS%Private()
    {
    }
    ~%CLASS%Private() {
    }
    virtual void setupQuality() {}
};

%CLASS%::%CLASS%(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f)
  , VideoRenderer(*new %CLASS%Private())
{
    DPTR_INIT_PRIVATE(%CLASS%);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    //setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_PaintOnScreen, true);
}

%CLASS%::~%CLASS%()
{
}


QPaintEngine* %CLASS%::paintEngine() const
{
    return 0; //use native engine
}

void %CLASS%::convertData(const QByteArray &data)
{
    DPTR_D(%CLASS%);
    //TODO: if date is deep copied, mutex can be avoided
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
}

ool %CLASS%::needUpdateBackground() const
{
    return VideoRenderer::needUpdateBackground();
}

void %CLASS%::drawBackground()
{
}

bool %CLASS%::needDrawFrame() const
{
    return VideoRenderer::needDrawFrame();
}

void %CLASS%::drawFrame()
{
    //assume that the image data is already scaled to out_size(NOT renderer size!)
    if (d.src_width == d.out_rect.width() && d.src_height == d.out_rect.height()) {
        //you may copy data to video buffer directly
    } else {
        //paint with scale
    }
}

void %CLASS%::paintEvent(QPaintEvent *)
{
    //DPTR_D(%CLASS%);
    //d.painter->begin(this); //Widget painting can only begin as a result of a paintEvent
    handlePaintEvent();
    //end paint. how about QPainter::endNativePainting()?
    //d.painter->end();
}

void %CLASS%::resizeEvent(QResizeEvent *e)
{
    DPTR_D(%CLASS%);
    d.update_background = true;
    resizeRenderer(e->size());
    update();
}

void %CLASS%::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    DPTR_D(%CLASS%);
    d.update_background = true;
    /*
     * Do something that depends on widget below! e.g. recreate render target for direct2d.
     * When Qt::WindowStaysOnTopHint changed, window will hide first then show. If you
     * don't do anything here, the widget content will never be updated.
     */
}

bool %CLASS%::write()
{
    update();
    return true;
}

} //namespace QtAV
