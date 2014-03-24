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


#ifndef DECODERCONFIGPAGE_H
#define DECODERCONFIGPAGE_H

#include <QWidget>
#include "ConfigPageBase.h"

class QListWidget;
class QToolButton;
class QSpinBox;
class QVBoxLayout;
class DecoderConfigPage : public ConfigPageBase
{
    Q_OBJECT
    class DecoderItemWidget;
public:
    explicit DecoderConfigPage(QWidget *parent = 0);
    virtual QString name() const;

public slots:
    virtual void apply();
    virtual void cancel();
    virtual void reset();

private slots:
    void videoDecoderEnableChanged();
    void priorityUp();
    void priorityDown();
    void onDecSelected(DecoderItemWidget* iw);
    void updateDecodersUi();
    void onConfigChanged();

private:
    QSpinBox *mpThreads;
    QToolButton *mpUp, *mpDown;
    QList<DecoderItemWidget*> mDecItems;
    DecoderItemWidget *mpSelectedDec;
    QVBoxLayout *mpDecLayout;
};

#endif // DECODERCONFIGPAGE_H
