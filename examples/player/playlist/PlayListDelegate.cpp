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

#include "PlayListDelegate.h"
#include "PlayListItem.h"

#include <QtGui/QPainter>
#include <QApplication>
#include "PlayListModel.h"

static const int kMarginLeft = 4;
static const int kMarginTop = 2;
static const int kWidth = 320;
static const int kHeightMax = 30;
static const int kHeightMin = 20;

PlayListDelegate::PlayListDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
    mHighlightRow = -1;
}

// dynamic height:http://www.qtcentre.org/threads/18879-solved-QListView-dynamic-item-height
void PlayListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.data().canConvert<PlayListItem>()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();
    painter->translate(option.rect.topLeft()); //!!!
    painter->setRenderHint(QPainter::Antialiasing, true);
    PlayListItem pli = qvariant_cast<PlayListItem>(index.data(Qt::DisplayRole));
    //selected state has different background
    bool detail = false;
    //TODO: draw border. wrong rect. http://stackoverflow.com/questions/18568594/correct-highlighting-with-qt-custom-delegates
    //const QWidget *widget = option.widget;
    //QStyle *style = widget ? widget->style() : QApplication::style();
    if (option.state & QStyle::State_Selected) {
        detail = true;
        mSelectedRows.append(index.row());
        painter->fillRect(QRect(0, 0, kWidth, kHeightMax), QColor(0, 100, 200, 100));
        //style->drawControl(QStyle::CE_ItemViewItem, &option, painter, widget);
    } else {
        mSelectedRows.removeAll(index.row());
    }
    // if check QStyle::State_HasFocus, updateLayout() will be called infinitly, why?
    if (option.state & (QStyle::State_MouseOver)) {
        detail = true;
        PlayListModel *m = (PlayListModel*)index.model();
        if (m && mHighlightRow != index.row()) {
            mHighlightRow = index.row();
            m->updateLayout();
        }
        painter->fillRect(QRect(0, 0, kWidth, kHeightMax), QColor(0, 100, 200, 30));
    }
    QFont ft;
    ft.setBold(detail);
    ft.setPixelSize(12);//kHeightMin - 2*kMarginTop);
    painter->setFont(ft);
    painter->translate(kMarginLeft, kMarginTop);
    painter->drawText(QRect(0, 0, kWidth - 2*kMarginLeft, kHeightMin - 2*kMarginTop), pli.title());
    painter->translate(0, kHeightMin + kMarginTop);
    if (detail) {
        painter->save();
        ft.setBold(false);
        ft.setPixelSize(8);//(kHeightMax - kHeightMin - 2*kMarginTop));
        painter->setFont(ft);
        painter->drawText(0, 0, pli.lastTimeString() + "/" + pli.durationString());
        painter->restore();
    }
    painter->restore();
}

// http://qt-project.org/forums/viewthread/5741
QSize PlayListDelegate::sizeHint(const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    if (!index.data().canConvert<PlayListItem>()) {
        return QStyledItemDelegate::sizeHint(option, index);
    }
    bool detail = option.state & (QStyle::State_Selected|QStyle::State_MouseOver);
    // TODO: why detail always false?
    if (detail || mHighlightRow == index.row() || mSelectedRows.contains(index.row())) {
        return QSize(kWidth, kHeightMax);
    }
    return QSize(kWidth, kHeightMin);
}
