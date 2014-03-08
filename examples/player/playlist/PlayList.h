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


#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QWidget>
#include <QtCore/QModelIndex>
#include "PlayListItem.h"

class QListView;
class QToolButton;
class PlayListDelegate;

class PlayListModel;
class PlayList : public QWidget
{
    Q_OBJECT
public:
    explicit PlayList(QWidget *parent = 0);
    ~PlayList();

    void setSaveFile(const QString& file);
    QString saveFile() const;
    void load();
    void save();

    PlayListItem itemAt(int row);
    void insertItemAt(const PlayListItem& item, int row = 0);
    void setItemAt(const PlayListItem& item, int row = 0);
    void remove(const QString& url);
    void insert(const QString& url, int row = 0);
    void setMaxRows(int r);
    int maxRows() const;

signals:
    void aboutToPlay(const QString& url);

private slots:
    void removeSelectedItems();
    void clearItems();
    //
    void addItems();

    void onAboutToPlay(const QModelIndex& index);
    //void highlight(const QModelIndex& index);
private:
    QListView *mpListView;
    QToolButton *mpClear, *mpRemove, *mpAdd;
    PlayListDelegate *mpDelegate;
    PlayListModel *mpModel;
    int mMaxRows;
    QString mFile;
    bool mFirstShow;
};

#endif // PLAYLIST_H
