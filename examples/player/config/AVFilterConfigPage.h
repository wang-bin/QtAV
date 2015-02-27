/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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
#ifndef AVFILTERCONFIGPAGE_H
#define AVFILTERCONFIGPAGE_H

#include "ConfigPageBase.h"

class QCheckBox;
class QTextEdit;
class AVFilterConfigPage : public ConfigPageBase
{
public:
    AVFilterConfigPage(QWidget* parent = 0);
    virtual QString name() const;
protected:
    virtual void applyToUi();
    virtual void applyFromUi();
private:
    QCheckBox *m_enable;
    QTextEdit *m_options;
};

#endif // AVFILTERCONFIGPAGE_H
