#ifndef STATISTICSVIEW_H
#define STATISTICSVIEW_H

#include <QDialog>
#include <QtAV/Statistics.h>

using namespace QtAV;

class QTreeWidget;
class QTreeWidgetItem;
class StatisticsView : public QDialog
{
    Q_OBJECT
public:
    explicit StatisticsView(QWidget *parent = 0);
    void setStatistics(Statistics* s);

protected:
    virtual void hideEvent(QHideEvent* e);
    virtual void showEvent(QShowEvent* e);
    virtual void timerEvent(QTimerEvent *e);

signals:
    
public slots:
    
private:
    void initBaseItems(QList<QTreeWidgetItem*>* items);
    QTreeWidgetItem* createNodeWithItems(QTreeWidget* view, const QString& name, const QStringList& itemNames, QList<QTreeWidgetItem*>* items = 0);

    QTreeWidget *mpView;
    QList<QTreeWidgetItem*> mBaseItems;
    QList<QTreeWidgetItem*> mVideoItems;
    //TODO: multiple streams
    QList<QTreeWidgetItem*> mAudioItems;
    Statistics *mpStatistics;
    int mTimer;

    QTreeWidgetItem *mpFPS, *mpAudioBitRate, *mpVideoBitRate;
};

#endif // STATISTICSVIEW_H
