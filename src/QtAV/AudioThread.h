#ifndef QAVAUDIOTHREAD_H
#define QAVAUDIOTHREAD_H

#include <QtAV/AVThread.h>

namespace QtAV {

class AudioDecoder;
class AudioThreadPrivate;
class Q_EXPORT AudioThread : public AVThread
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(AudioThread)
public:
    explicit AudioThread(QObject *parent = 0);

protected:
    virtual void run();
};
}

#endif // QAVAUDIOTHREAD_H
