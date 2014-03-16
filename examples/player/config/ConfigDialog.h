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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QtCore/QList>

class ConfigPageBase;
class ConfigDialog : public QDialog
{
    Q_OBJECT
public:
    static void display();

signals:

private slots:
    void onButtonClicked(QAbstractButton* btn);
    void onApply();
    void onCancel();
    void onReset();

private:
    explicit ConfigDialog(QWidget *parent = 0);
    QTabWidget *mpContent;
    QDialogButtonBox *mpButtonBox;
    QList<ConfigPageBase*> mPages;
};

#endif // CONFIGDIALOG_H
