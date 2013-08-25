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
    void setStatistics(const Statistics& s);

signals:
    
public slots:
    
private:
    void initBaseItems(QList<QTreeWidgetItem*>* items);
    QTreeWidgetItem* initVideoItems(QList<QTreeWidgetItem*>* items);
    QTreeWidgetItem* initAudioItems(QList<QTreeWidgetItem*>* items);

    QTreeWidget *mpView;
    QList<QTreeWidgetItem*> mBaseItems;
    QList<QTreeWidgetItem*> mVideoItems;
    QList<QTreeWidgetItem*> mAudioItems;
    Statistics mStatistics;

};

#endif // STATISTICSVIEW_H
