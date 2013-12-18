#include "StatisticsView.h"
#include <QtCore/QTimerEvent>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QLayout>
#include <QHeaderView>
#include <QPushButton>

QStringList getBaseInfoKeys() {
    return QStringList()
            << QObject::tr("Url")
            << QObject::tr("Format")
            << QObject::tr("Bit rate")
            << QObject::tr("Start time")
            << QObject::tr("Duration")
               ;
}

QStringList getCommonInfoKeys() {
    return QStringList()
            << QObject::tr("Available")
            << QObject::tr("Codec")
            << QObject::tr("Total time")
            << QObject::tr("Start time")
            << QObject::tr("Bit rate")
            << QObject::tr("Frames")
               ;
}

QStringList getVideoInfoKeys() {
    return getCommonInfoKeys()
            << QObject::tr("FPS Now") //current display fps
            << QObject::tr("FPS") // avg_frame_rate. guessed by FFmpeg
            << QObject::tr("Pixel format")
            << QObject::tr("Size") //w x h
            << QObject::tr("Coded size") // w x h
            << QObject::tr("GOP size")
               ;
}
QStringList getAudioInfoKeys() {
    return getCommonInfoKeys()
            << QObject::tr("Sample format")
            << QObject::tr("Sample rate")
            << QObject::tr("Channels")
            << QObject::tr("Channel layout")
            << QObject::tr("Frame size")
               ;
}

QVariantList getBaseInfoValues(const Statistics& s) {
    return QVariantList()
            << s.url
            << s.format
            << QString::number(s.bit_rate/1000) + " Kb/s"
            << s.start_time.toString("HH:mm:ss")
            << s.duration.toString("HH:mm:ss")
               ;
}

QList<QVariant> getVideoInfoValues(const Statistics& s) {
    return QList<QVariant>()
            << s.video.available
            << s.video.codec + " (" + s.video.codec_long + ")"
            << s.video.total_time.toString("HH:mm:ss")
            << s.video.start_time.toString("HH:mm:ss")
            << QString::number(s.video.bit_rate/1000) + " Kb/s"
            << s.video.frames
            << s.video_only.avg_frame_rate //TODO: dynamic compute
            << s.video_only.avg_frame_rate
            << s.video_only.pix_fmt
            << QString::number(s.video_only.width) + "x" + QString::number(s.video_only.height)
            << QString::number(s.video_only.coded_width) + "x" + QString::number(s.video_only.coded_height)
            << s.video_only.gop_size
               ;
}
QList<QVariant> getAudioInfoValues(const Statistics& s) {
    return QList<QVariant>()
            << s.audio.available
            << s.audio.codec + " (" + s.audio.codec_long + ")"
            << s.audio.total_time.toString("HH:mm:ss")
            << s.audio.start_time.toString("HH:mm:ss")
            << QString::number(s.audio.bit_rate/1000) + " Kb/s"
            << s.audio.frames
            << s.audio_only.sample_fmt
            << QString::number(s.audio_only.sample_rate) + " Hz"
            << s.audio_only.channels
            << s.audio_only.channel_layout
            << s.audio_only.frame_size
               ;
}


StatisticsView::StatisticsView(QWidget *parent) :
    QDialog(parent)
  , mTimer(0)
  , mpFPS(0)
  , mpAudioBitRate(0)
  , mpVideoBitRate(0)
{
    setWindowTitle(tr("Media info"));
    setModal(false);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    mpView = new QTreeWidget();
    mpView->setHeaderHidden(true);
    mpView->setColumnCount(2);
    initBaseItems(&mBaseItems);
    mpView->addTopLevelItems(mBaseItems);
    QTreeWidgetItem *item = createNodeWithItems(mpView, QObject::tr("Video"), getVideoInfoKeys(), &mVideoItems);
    mpFPS = item->child(6);
    //mpVideoBitRate =
    mpView->addTopLevelItem(item);
    item = createNodeWithItems(mpView, QObject::tr("Audio"), getAudioInfoKeys(), &mAudioItems);
    //mpAudioBitRate =
    mpView->addTopLevelItem(item);
    mpView->resizeColumnToContents(0); //call this after content is done

    QPushButton *btn = new QPushButton(QObject::tr("Ok"));
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(btn);
    QObject::connect(btn, SIGNAL(clicked()), SLOT(accept()));

    QVBoxLayout *vl = new QVBoxLayout();
    vl->addWidget(mpView);
    vl->addLayout(btnLayout);
    setLayout(vl);
}

void StatisticsView::setStatistics(const Statistics& s)
{
    mStatistics = s;
    QVariantList v = getBaseInfoValues(s);
    int i = 0;
    foreach(QTreeWidgetItem* item, mBaseItems) {
        if (item->data(1, Qt::DisplayRole) != v.at(i)) {
            item->setData(1, Qt::DisplayRole, v.at(i));
        }
        ++i;
    }
    v = getVideoInfoValues(s);
    i = 0;
    foreach(QTreeWidgetItem* item, mVideoItems) {
        if (item->data(1, Qt::DisplayRole) != v.at(i)) {
            item->setData(1, Qt::DisplayRole, v.at(i));
        }
        ++i;
    }
    v = getAudioInfoValues(s);
    i = 0;
    foreach(QTreeWidgetItem* item, mAudioItems) {
        if (item->data(1, Qt::DisplayRole) != v.at(i)) {
            item->setData(1, Qt::DisplayRole, v.at(i));
        }
        ++i;
    }
}

void StatisticsView::hideEvent(QHideEvent *e)
{
    QDialog::hideEvent(e);
    killTimer(mTimer);
}

void StatisticsView::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);
    mTimer = startTimer(1000);
}

void StatisticsView::timerEvent(QTimerEvent *e)
{
    if (e->timerId() != mTimer)
        return;
    if (mpFPS) {
        mpFPS->setData(1, Qt::DisplayRole, QString::number(mStatistics.video_only.currentDisplayFPS(), 'f', 2));
    }
}

void StatisticsView::initBaseItems(QList<QTreeWidgetItem *> *items)
{
    QTreeWidgetItem *item = 0;
    foreach(QString key, getBaseInfoKeys()) {
        item = new QTreeWidgetItem(0);
        item->setData(0, Qt::DisplayRole, key);
        items->append(item);
    }
}

QTreeWidgetItem* StatisticsView::createNodeWithItems(QTreeWidget *view, const QString &name, const QStringList &itemNames, QList<QTreeWidgetItem *> *items)
{
    QTreeWidgetItem *nodeItem = new QTreeWidgetItem(view);
    nodeItem->setData(0, Qt::DisplayRole, name);
    QTreeWidgetItem *item = 0;
    foreach(QString key, itemNames) {
        item = new QTreeWidgetItem(nodeItem);
        item->setData(0, Qt::DisplayRole, key);
        nodeItem->addChild(item);
        if (items)
            items->append(item);
    }
    nodeItem->setExpanded(true);
    return nodeItem;
}
