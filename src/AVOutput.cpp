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

void AVOutput::bindDecoder(AVDecoder *dec)
{
    d_ptr->dec = dec;
}

}
