#include <QtAV/AVDemuxThread.h>

namespace QtAV {
AVDemuxThread::AVDemuxThread(QObject *parent) :
    QThread(parent)
{
}


void AVDemuxThread::run()
{
    //demuxer->read()
    //enqueue()
}
}
