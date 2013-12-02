/******************************************************************************
    DecoderConfigPage.h: description
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


#ifndef DECODERCONFIGPAGE_H
#define DECODERCONFIGPAGE_H

#include <QWidget>

class Config;
class QListWidget;
class QToolButton;
class QSpinBox;
class QVBoxLayout;
class DecoderConfigPage : public QWidget
{
    Q_OBJECT
    class DecoderItemWidget;
public:
    explicit DecoderConfigPage(Config *config, QWidget *parent = 0);

signals:

public slots:

private slots:
    void videoDecoderEnableChanged();
    void priorityUp();
    void priorityDown();
    void onDecSelected(DecoderItemWidget* iw);

private:

    Config *mpConfig;

    QSpinBox *mpThreads;
    QToolButton *mpUp, *mpDown;
    QList<DecoderItemWidget*> mDecItems;
    DecoderItemWidget *mpSelectedDec;
    QVBoxLayout *mpDecLayout;
};

#endif // DECODERCONFIGPAGE_H
