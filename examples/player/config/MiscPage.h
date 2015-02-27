/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef MISCPAGE_H
#define MISCPAGE_H

#include "ConfigPageBase.h"
#include <QCheckBox>
#include <QSpinBox>

class MiscPage : public ConfigPageBase
{
    Q_OBJECT
public:
    MiscPage();
    virtual QString name() const;
public Q_SLOTS:
    virtual void apply(); //store the values on ui. call Config::xxx
    virtual void cancel(); //cancel the values on ui. values are from Config
    virtual void reset(); //reset to default
private:
    QCheckBox *m_preview_on;
    QSpinBox *m_notify_interval;
};

#endif // MISCPAGE_H
