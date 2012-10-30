#ifndef QAVWRITER_H
#define QAVWRITER_H

#include <qbytearray.h>
#include <QtAV/QtAV_Global.h>

namespace QtAV {

class AVDecoder;
class AVOutputPrivate;
class Q_EXPORT AVOutput
{
public:
    AVOutput();
    virtual ~AVOutput() = 0;
    virtual int write(const QByteArray& data) = 0;
    virtual bool open() = 0;
    virtual bool close() = 0;
    void bindDecoder(AVDecoder *dec); //for resizing video (or resample audio?)

protected:
    AVOutput(AVOutputPrivate& d);

    Q_DECLARE_PRIVATE(AVOutput)
    AVOutputPrivate *d_ptr; //ambiguous with Qt class d_ptr
};

} //namespace QtAV
#endif //QAVWRITER_H
