#ifndef QAVVIDEORENDERER_P_H
#define QAVVIDEORENDERER_P_H

#include <qbytearray.h>
#include <private/AVOutput_p.h>
namespace QtAV {

class VideoRendererPrivate : public AVOutputPrivate
{
public:
    VideoRendererPrivate():width(0),height(0)
      {
    }

    int width, height;
};

} //namespace QtAV
#endif // QAVVIDEORENDERER_P_H
