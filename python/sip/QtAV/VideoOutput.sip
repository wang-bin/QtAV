namespace QtAV
{

class VideoOutput : public QObject, public QtAV::VideoRenderer
{
%TypeHeaderCode
#include <QtAV/VideoOutput.h>
%End

public:
    VideoOutput(QWidget* parent /TransferThis/ = 0);
    VideoOutput(VideoRendererId rendererId, QObject *parent /TransferThis/ = 0);
    ~VideoOutput();

    VideoRendererId id() const;

    virtual QWidget* widget() final;
};

};
