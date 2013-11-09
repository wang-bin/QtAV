#ifndef TVVIEW_H
#define TVVIEW_H

#include <QWidget>


class QTreeWidget;
class QTreeWidgetItem;

class TVView : public QWidget
{
    Q_OBJECT
public:
    explicit TVView(QWidget *parent = 0);

signals:
    void clicked(const QString& key, const QString& value);

private slots:
    void load();
    void onItemDoubleClick( QTreeWidgetItem * item, int column);
private:
    QTreeWidgetItem* createNodeWithItems(QTreeWidget* view, const QString& name, const QStringList& itemNames, QList<QTreeWidgetItem*>* items = 0);

    QTreeWidget *mpView;

};

#endif // TVVIEW_H
