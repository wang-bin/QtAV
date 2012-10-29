#ifndef QAVPACKET_H
#define QAVPACKET_H

#include <qbytearray.h>
#include <QtAV/QtAV_Global.h>

namespace QtAV {
class Q_EXPORT QAVPacket
{
public:
    QAVPacket();

    QByteArray data;
    qreal pts, duration;
};
}

#endif // QAVPACKET_H
