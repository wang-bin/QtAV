namespace QtAV
{

class GraphicsItemRenderer : public QGraphicsObject, public QtAV::QPainterRenderer
{
%TypeHeaderCode
#include <QtAVWidgets/GraphicsItemRenderer.h>
%End

public:
    void setBrightness(qreal brightness);
    qreal brightness();
    void setContrast(qreal contrast);
    qreal contrast();
    void setHue(qreal hue);
    qreal hue();
    void setSaturation(qreal saturation);
    qreal saturation();
    void setBackgroundColor(QColor color);
    QColor backgroundColor();

    QRectF regionOfInterest();
    void setRegionOfInterest(QRectF region);
    qreal sourceAspectRatio();
    qreal outAspectRatio();
    void setOutAspectRatio(qreal ratio);
    OutAspectRatioMode outAspectRatioMode();
    void setOutAspectRatioMode(OutAspectRatioMode mode);

    int orientation();
    void setOrientation(int orientation);
    QRect videoRect();
    QSize videoFrameSize();

public:
    GraphicsItemRenderer(QGraphicsItem* parent /TransferThis/ = 0);
    virtual VideoRendererId id();

    bool isSupported(VideoFormat::PixelFormat pixfmt);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QGraphicsItem* graphicsItem();

    bool isOpenGL() const;
    void setOpenGL(bool o);

    OpenGLVideo* opengl() const;

signals:
    void sourceAspectRatioChanged(qreal value) final;
    void regionOfInterestChanged();
    void outAspectRatioChanged();
    void outAspectRatioModeChanged();
    void brightnessChanged(qreal value);
    void contrastChanged(qreal);
    void hueChanged(qreal);
    void saturationChanged(qreal);
    void backgroundColorChanged();
    void orientationChanged();
    void videoRectChanged();
    void videoFrameSizeChanged();
    void openGLChanged();

protected:
    bool receiveFrame(const VideoFrame& frame);
    void drawBackground();
    void drawFrame();
    bool event(QEvent *event);
};
typedef GraphicsItemRenderer VideoRendererGraphicsItem;

};
