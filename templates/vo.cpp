
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
};

%CLASS%::%CLASS%(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f)
  , VideoRenderer(*new %CLASS%Private())
{
    DPTR_INIT_PRIVATE(%CLASS%);
    d_func().widget_holder = this;
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
    if (!d.scale_in_renderer) {
        /*if lock is required, do not use locker in if() scope, it will unlock outside the scope*/
        d.img_mutex.lock();
        /* convert data to your image below*/
        d.img_mutex.unlock();
    } else {
        //TODO: move to private class
        /* convert data to your image below. image size is (d.src_width, d.src_height)*/
    }
}

void %CLASS%::paintEvent(QPaintEvent *)
{
    DPTR_D(%CLASS%);
    if (!d.scale_in_renderer) {
        d.img_mutex.lock();
    }
    //begin paint. how about QPainter::beginNativePainting()?

    //fill background color when necessary, e.g. renderer is resized, image is null
    if ((d.update_background && d.out_rect != rect()) || d.data.isEmpty()) {
        d.update_background = false;
        //fill background color. DO NOT return, you must continue drawing
    }
    if (d.data.isEmpty()) {
        return;
    }
    //assume that the image data is already scaled to out_size(NOT renderer size!)
    if (!d.scale_in_renderer || (d.src_width == d.out_rect.width() && d.src_height == d.out_rect.height())) {
        //you may copy data to video buffer directly
    } else {
        //paint with scale
    }

    //end paint. how about QPainter::endNativePainting()?
    if (!d.scale_in_renderer) {
        d.img_mutex.unlock();
    }
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
