#ifndef QAVAUDIODECODER_H
#define QAVAUDIODECODER_H

#include <QtAV/AVDecoder.h>

namespace QtAV {

class Q_EXPORT AudioDecoder : public AVDecoder
{
public:
    virtual bool decode(const QByteArray &encoded);

};
}

#endif // QAVAUDIODECODER_H
