
#include "QtAV/%CLASS%.h"
#include "private/VideoRenderer_p.h"
#include <QtGui/QPainter>
#include <QtGui/QPaintEngine>
#include <QResizeEvent>

namespace QtAV {

class %CLASS%Private : public VideoRendererPrivate
{
public:
    DPTR_DECLARE_PUBLIC(%CLASS%)

    %CLASS%Private()
    {
        use_qpainter = false; //default is to use custome paint engine, e.g. dx. gl
    }
    ~%CLASS%Private() {
    }
    bool use_qpainter; //TODO: move to base class
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
}

%CLASS%::~%CLASS%()
{
}


QPaintEngine* %CLASS%::paintEngine() const
{
    if (d_func().use_qpainter) {
        return QWidget::paintEngine();
    } else {
        return 0; //use native engine
    }
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

    //end paint. how about QPainter::endNativePainting()?
    if (!d.scale_in_renderer) {
        d.img_mutex.unlock();
    }
}

void %CLASS%::resizeEvent(QResizeEvent *e)
{
    resizeRenderer(e->size());
    update();
}

void %CLASS%::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    DPTR_D(%CLASS%);
    useQPainter(d.use_qpainter);
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
