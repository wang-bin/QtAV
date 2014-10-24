#include <QtAV/VideoFrameExtractor.h>
#include <QApplication>
#include <QWidget>
#include <QtAV/VideoRenderer.h>
#include <QtAV/VideoRendererTypes.h>
#include <QtDebug>

using namespace QtAV;
class VideoFrameObserver : public QObject
{
    Q_OBJECT
public:
    VideoFrameObserver(QObject *parent = 0) : QObject(parent)
    {
        view = VideoRendererFactory::create(VideoRendererId_GLWidget2);
        view->widget()->resize(400, 300);
        view->widget()->show();
    }

public Q_SLOTS:
    void onVideoFrameExtracted() {
        VideoFrameExtractor *e = qobject_cast<VideoFrameExtractor*>(sender());
        VideoFrame frame(e->frame());
        view->receive(frame);
        qDebug() << frame.format();
        qDebug("frame %dx%d", frame.width(), frame.height());
    }
private:
    VideoRenderer *view;
};

int main(int argc, char** argv)
{
    QApplication a(argc, argv);
    int idx = a.arguments().indexOf("-f");
    if (idx < 0)
        return -1;
    QString file = a.arguments().at(idx+1);
    idx = a.arguments().indexOf("-t");
    int t = 0;
    if (idx > 0)
        t = a.arguments().at(idx+1).toInt();

    VideoFrameExtractor extractor;
    VideoFrameObserver obs;
    QObject::connect(&extractor, SIGNAL(frameExtracted()), &obs, SLOT(onVideoFrameExtracted()));
    extractor.setSource(file);
    extractor.setPosition(t);
    return a.exec();
}

#include "main.moc"
