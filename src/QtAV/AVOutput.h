#ifndef QAVWRITER_H
#define QAVWRITER_H

#include <qbytearray.h>
#include <QtAV/QtAV_Global.h>

namespace QtAV {
class Q_EXPORT AVOutput
{
public:
    AVOutput();
    virtual ~AVOutput() = 0;
    virtual int write(const QByteArray& data) = 0;
    virtual bool open() = 0;
    virtual bool close() = 0;
};
}

#endif //QAVWRITER_H
