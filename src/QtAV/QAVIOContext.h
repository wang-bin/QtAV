#ifndef AVIOCONTEXT_H
#define AVIOCONTEXT_H

#include <sys/types.h>
#include <QtAV_Global.h>

class QIODevice;
struct AVIOContext;

#define IODATA_BUFFER_SIZE 32768

namespace QtAV {

class QAVIOContext
{
public:
    QAVIOContext(QIODevice* io);
    ~QAVIOContext();
    static int read(void *opaque, unsigned char *buf, int buf_size);
    static int write(void *opaque, unsigned char *buf, int buf_size);
    static int64_t seek(void *opaque, int64_t offset, int whence);

    AVIOContext* context();

    QIODevice* device() const;
    void setDevice(QIODevice* device);

private:
    unsigned char* m_ucDataBuffer;
    QIODevice* m_pIO;
};

}
#endif // AVIOCONTEXT_H
