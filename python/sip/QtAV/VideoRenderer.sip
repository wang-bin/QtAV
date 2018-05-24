namespace QtAV
{

typedef int VideoRendererId /NoTypeName/;

class VideoRenderer : public QtAV::AVOutput /Abstract/
{
%TypeHeaderCode
#include <QtAV/VideoRenderer.h>
%End

public:
    enum OutAspectRatioMode {
        RendererAspectRatio //Use renderer's aspect ratio, i.e. stretch to fit the renderer rect
      , VideoAspectRatio    //Use video's aspect ratio and align center in renderer.
      , CustomAspectRation  //Use the ratio set by setOutAspectRatio(qreal). Mode will be set to this if that function is called
      //, AspectRatio4_3, AspectRatio16_9
    };

    VideoRenderer();
    virtual ~VideoRenderer();
    virtual VideoRendererId id() const = 0;

private:
    VideoRenderer(const VideoRenderer& r);
};

};
