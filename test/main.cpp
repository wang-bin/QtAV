#if CONFIG_EZX
#include <ZApplication.h>
#else
#include <qapplication.h>
typedef QApplication ZApplication;
#endif //CONFIG_EZX

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMessageBox>

#include <QtAV/AVPlayer.h>
#include <QtAV/WidgetRenderer.h>
#include <QtAV/GraphicsItemRenderer.h>
using namespace QtAV;

int main(int argc, char *argv[])
{
	ZApplication a(argc, argv);
    
    AVPlayer player;
    qDebug("%s %d", __FUNCTION__, __LINE__);
    if (argc>1)
        player.setFile(argv[1]);
    else
        QMessageBox::warning(0, "Usage", QString("%1 path/of/video").arg(qApp->arguments().at(0)));

    QGraphicsScene s;
    s.setSceneRect(0, 0, 800, 600);
    QGraphicsView v(&s);
    qDebug("%s %d", __FUNCTION__, __LINE__);

    GraphicsItemRenderer g;
    g.resizeVideo(800, 600);
    s.addItem(&g);
    qDebug("%s %d", __FUNCTION__, __LINE__);

    WidgetRenderer w;
    w.setWindowTitle(argv[1]);
    qDebug("%s %d", __FUNCTION__, __LINE__);
    player.setRenderer(&w);
    w.resize(400, 300);
    w.resizeVideo(w.size());
    w.show();
    player.play();
    return a.exec();
}
