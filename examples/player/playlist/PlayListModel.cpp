/******************************************************************************
    PlayListModel.cpp: description
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


#include "PlayListModel.h"
#include "PlayListItem.h"

PlayListModel::PlayListModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

QList<PlayListItem> PlayListModel::items() const
{
    return mItems;
}

Qt::ItemFlags PlayListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return QAbstractListModel::flags(index) | Qt::ItemIsDropEnabled;;
    return  QAbstractListModel::flags(index) | Qt::ItemIsSelectable;
}

int PlayListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return mItems.size();
}

QVariant PlayListModel::data(const QModelIndex &index, int role) const
{
    QVariant v;
    if (index.row() < 0 || index.row() >= mItems.size())
        return v;

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        v.setValue(mItems.at(index.row()));

    return v;
}

bool PlayListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.row() >= 0 && index.row() < mItems.size()
            && (role == Qt::EditRole || role == Qt::DisplayRole)) {
        // TODO: compare value?
        mItems.replace(index.row(), value.value<PlayListItem>());
        emit dataChanged(index, index, QVector<int>() << role);
        return true;
    }
    return false;
}


bool PlayListModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (count < 1 || row < 0 || row > rowCount(parent))
        return false;
    beginInsertRows(QModelIndex(), row, row + count - 1);
    for (int r = 0; r < count; ++r)
        mItems.insert(row, PlayListItem());
    endInsertRows();
    return true;
}


bool PlayListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (count <= 0 || row < 0 || (row + count) > rowCount(parent))
        return false;
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for (int r = 0; r < count; ++r)
        mItems.removeAt(row);
    endRemoveRows();
    return true;
}

void PlayListModel::updateLayout()
{
    emit layoutChanged();
}
