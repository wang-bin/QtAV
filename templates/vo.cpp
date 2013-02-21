
#include "QtAV/%CLASS%.h"
#include "private/ImageRenderer_p.h"
#include <QtGui/QPainter>
#include <QtGui/QPaintEngine>
#include <QResizeEvent>

namespace QtAV {

class %CLASS%Private : public ImageRendererPrivate
{
public:
    DPTR_DECLARE_PUBLIC(%CLASS%)

    %CLASS%Private():
		use_qpainter(true)
    {
    }
    ~%CLASS%Private() {
    }
    bool use_qpainter; //TODO: move to base class
};

%CLASS%::%CLASS%(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f),ImageRenderer(*new %CLASS%Private())
{
    DPTR_INIT_PRIVATE(%CLASS%);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    //setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
}

%CLASS%::~%CLASS%()
{
}

bool %CLASS%::write()
{
    update();
    return true;
}

QPaintEngine* %CLASS%::paintEngine() const
{
    if (d_func().use_qpainter) {
        return QWidget::paintEngine();
    } else {
        return 0; //use native engine
    }
}

void %CLASS%::useQPainter(bool qp)
{
    DPTR_D(%CLASS%);
    d.use_qpainter = qp;
    setAttribute(Qt::WA_PaintOnScreen, !d.use_qpainter);
}

bool %CLASS%::useQPainter() const
{
    DPTR_D(const %CLASS%);
	return d.use_qpainter;
}

void %CLASS%::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange) { //auto called when show
        useQPainter(d_func().use_qpainter);
        event->accept();
    }
}

void %CLASS%::resizeEvent(QResizeEvent *e)
{
    resizeVideo(e->size());
    update();
}

void %CLASS%::paintEvent(QPaintEvent *)
{
    DPTR_D(%CLASS%);
    if (!d.scale_in_qt) {
        d.img_mutex.lock();
    }
    QImage image = d.image; //TODO: other renderer use this style
    if (image.isNull()) {
        if (d.preview.isNull()) {
            d.preview = QImage(videoSize(), QImage::Format_RGB32);
            d.preview.fill(Qt::black); //maemo 4.7.0: QImage.fill(uint)
        }
        image = d.preview;
    }
    //begin paint
    
    //end paint
    if (!d.scale_in_qt) {
        d.img_mutex.unlock();
    }
}

} //namespace QtAV
