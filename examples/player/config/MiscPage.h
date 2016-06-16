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
#include <QDoubleSpinBox>
#include <QComboBox>

class MiscPage : public ConfigPageBase
{
    Q_OBJECT
public:
    MiscPage();
    virtual QString name() const;
protected:
    virtual void applyToUi();
    virtual void applyFromUi();
private:
    QCheckBox *m_preview_on;
    QSpinBox *m_preview_w;
    QSpinBox *m_preview_h;
    QSpinBox *m_notify_interval;
    QDoubleSpinBox *m_fps;
    QSpinBox *m_buffer_value;
    QDoubleSpinBox *m_timeout;
    QCheckBox *m_timeout_abort;
    QComboBox *m_opengl;
    QComboBox *m_angle_platform;
    QCheckBox *m_egl;
    QComboBox *m_log;
};

#endif // MISCPAGE_H
