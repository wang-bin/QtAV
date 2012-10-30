#include <QtAV/AVOutput.h>
#include <private/AVOutput_p.h>

namespace QtAV {

AVOutput::AVOutput()
{
}

AVOutput::AVOutput(AVOutputPrivate &d)
    :d_ptr(&d)
{

}

//TODO: why need this?
AVOutput::~AVOutput()
{

}

int AVOutput::write(const QByteArray &data)
{
    Q_UNUSED(data);
    return 0;
}

void AVOutput::bindDecoder(AVDecoder *dec)
{
    d_ptr->dec = dec;
}

}
