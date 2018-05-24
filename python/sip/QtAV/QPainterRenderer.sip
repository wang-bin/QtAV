namespace QtAV
{

class QPainterRenderer : public QtAV::VideoRenderer /Abstract/
{
%TypeHeaderCode
#include <QtAV/QPainterRenderer.h>
%End

public:
    QPainterRenderer();
    bool isSupported(VideoFormat::PixelFormat pixfmt) const;
protected:
    bool preparePixmap(const VideoFrame& frame);
    void drawBackground();
    void drawFrame();
};

};
