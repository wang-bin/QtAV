/******************************************************************************
    PlayList.h: description
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Alternatively, this file may be used under the terms of the GNU
    General Public License version 3.0 as published by the Free Software
    Foundation and appearing in the file LICENSE.GPL included in the
    packaging of this file.  Please review the following information to
    ensure the GNU General Public License version 3.0 requirements will be
    met: http://www.gnu.org/copyleft/gpl.html.
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
