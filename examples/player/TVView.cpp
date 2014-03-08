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

#include "TVView.h"
#include <QtCore/QTimer>
#include <QtCore/QFile>
#include <QLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QString>
#include <QApplication>
#include <QtCore/QTextStream>

TVView::TVView(QWidget *parent) :
    QWidget(parent)
{
    setWindowTitle(tr("Online TV channels"));
    //setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    mpView = new QTreeWidget();
    mpView->setHeaderHidden(true);
    mpView->setColumnCount(1);
    connect(mpView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(onItemDoubleClick (QTreeWidgetItem*,int)));
    QVBoxLayout *vl = new QVBoxLayout();
    vl->addWidget(mpView);
    setLayout(vl);
    QTimer::singleShot(0, this, SLOT(load()));
}

void TVView::load()
{
    /*
        //codec problem
        QSettings tv(qApp->applicationDirPath() + "/tv.ini", QSettings::IniFormat);
        tv.setIniCodec("UTF-8");
        foreach (QString key, tv.allKeys()) {
            subMenu->addAction(key)->setData(tv.value(key).toString());
        }
    */
    QFile tv_file(qApp->applicationDirPath() + "/tv.ini");
    if (!tv_file.exists())
        tv_file.setFileName(":/tv.ini");
    if (!tv_file.open(QIODevice::ReadOnly))
        return;
    QTextStream ts(&tv_file);
    ts.setCodec("UTF-8");
    QTreeWidgetItem *nodeItem = new QTreeWidgetItem(mpView);
    nodeItem->setData(0, Qt::DisplayRole, "");
    mpView->addTopLevelItem(nodeItem);
    nodeItem->setExpanded(true);
    QString line;
    while (!ts.atEnd()) {
        line = ts.readLine();
        if (line.isEmpty() || line.startsWith("#"))
            continue;
        if (!line.contains("=")) {
            nodeItem = new QTreeWidgetItem(mpView);
            nodeItem->setData(0, Qt::DisplayRole, line);
            mpView->addTopLevelItem(nodeItem);
            continue;
        }
        QString key = line.section('=', 0, 0);
        QString value = line.section('=', 1);
        QTreeWidgetItem *item = new QTreeWidgetItem(nodeItem);
        item->setData(0, Qt::DisplayRole, key);
        item->setData(1, Qt::EditRole, value);
    }
    mpView->resizeColumnToContents(0); //call this after content is done
}

void TVView::onItemDoubleClick(QTreeWidgetItem *item, int column)
{
    emit clicked(item->data(0, Qt::DisplayRole).toString(), item->data(1, Qt::EditRole).toString());
}
