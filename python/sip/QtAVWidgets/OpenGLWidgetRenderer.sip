namespace QtAV
{

class OpenGLWidgetRenderer : public QOpenGLWidget, public QtAV::OpenGLRendererBase
{
%TypeHeaderCode
#include <QtAVWidgets/OpenGLWidgetRenderer.h>
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
    explicit OpenGLWidgetRenderer(QWidget* parent /TransferThis/ = 0, Qt::WindowFlags f = 0);
    virtual VideoRendererId id();
    virtual QWidget* widget();

signals:
    void sourceAspectRatioChanged(qreal value) final;
    void regionOfInterestChanged();
    void outAspectRatioChanged();
    void outAspectRatioModeChanged();
    void brightnessChanged(qreal value);
    void contrastChanged(qreal);
    void hueChanged(qreal);
    void saturationChanged(qreal);
    void orientationChanged();
    void videoRectChanged();
    void videoFrameSizeChanged();
    void backgroundColorChanged();

protected:
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int w, int h);
    virtual void resizeEvent(QResizeEvent *);  // not virtual in QGLWidget (Qt<5.5)
    virtual void showEvent(QShowEvent *);

};
typedef OpenGLWidgetRenderer VideoRendererOpenGLWidget;

};
