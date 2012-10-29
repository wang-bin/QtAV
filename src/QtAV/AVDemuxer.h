#ifndef QAVDEMUXER_H
#define QAVDEMUXER_H

#include <QtAV/QtAV_Global.h>

struct AVFormatContext;

namespace QtAV {
class QAVPacket;
class Q_EXPORT AVDemuxer
{
public:
    AVDemuxer();

    bool readPacket(QAVPacket* packet, int stream);

private:
    AVFormatContext *formatCtx;
};
}

#endif // QAVDEMUXER_H
