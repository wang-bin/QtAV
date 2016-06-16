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
#ifndef AVFORMATCONFIGPAGE_H
#define AVFORMATCONFIGPAGE_H

#include "ConfigPageBase.h"
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QSpinBox;
class QLineEdit;
QT_END_NAMESPACE
class AVFormatConfigPage : public ConfigPageBase
{
    Q_OBJECT
public:
    explicit AVFormatConfigPage(QWidget *parent = 0);
    virtual QString name() const;
protected:
    virtual void applyToUi();
    virtual void applyFromUi();
private:
    QCheckBox* m_on;
    QCheckBox *m_direct;
    QSpinBox *m_probeSize;
    QSpinBox *m_analyzeDuration;
    QLineEdit *m_extra;
};

#endif // AVFORMATCONFIGPAGE_H
