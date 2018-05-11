namespace QtAV
{

class OpenGLRendererBase : public QtAV::VideoRenderer
{
%TypeHeaderCode
#include <QtAV/OpenGLRendererBase.h>
%End

public:
    virtual ~OpenGLRendererBase();
    bool isSupported(VideoFormat::PixelFormat pixfmt) const;
    OpenGLVideo* opengl() const;

protected:
    virtual bool receiveFrame(const VideoFrame& frame);
    virtual void drawBackground();
    virtual void drawFrame();
    void onInitializeGL();
    void onPaintGL();
    void onResizeGL(int w, int h);
    void onResizeEvent(int w, int h);
    void onShowEvent();

private:
    OpenGLRendererBase();
};

};
