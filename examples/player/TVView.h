/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

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
