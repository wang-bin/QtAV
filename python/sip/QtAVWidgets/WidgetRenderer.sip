namespace QtAV
{

class WidgetRenderer : public QWidget, public QtAV::QPainterRenderer
{
%TypeHeaderCode
#include <QtAVWidgets/WidgetRenderer.h>
%End

public:
    QColor backgroundColor();
    void setBackgroundColor(QColor color);
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
    WidgetRenderer(QWidget* parent /TransferThis/ = 0, Qt::WindowFlags f = 0);
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
    void backgroundColorChanged();
    void orientationChanged();
    void videoRectChanged();
    void videoFrameSizeChanged();
    void imageReady();

protected:
    bool receiveFrame(const VideoFrame& frame);
    void resizeEvent(QResizeEvent *);
    void paintEvent(QPaintEvent *);
    bool onSetOrientation(int value);
};
typedef WidgetRenderer VideoRenderWidget;

};
