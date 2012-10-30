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
    virtual int write(const QByteArray& data);// = 0; //TODO: why pure may case "pure virtual method called"
    virtual bool open() = 0;
    virtual bool close() = 0;
    void bindDecoder(AVDecoder *dec); //for resizing video (or resample audio?)

protected:
    AVOutput(AVOutputPrivate& d);

    Q_DECLARE_PRIVATE(AVOutput)
    AVOutputPrivate *d_ptr; //TODO: ambiguous with Qt class d_ptr when it is a base class together with Qt classes
};

} //namespace QtAV
#endif //QAVWRITER_H
