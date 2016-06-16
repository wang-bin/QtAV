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

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLabel;
class QTextEdit;
QT_END_NAMESPACE
class AVFilterConfigPage : public ConfigPageBase
{
    Q_OBJECT
public:
    AVFilterConfigPage(QWidget* parent = 0);
    virtual QString name() const;
protected:
    virtual void applyToUi();
    virtual void applyFromUi();
private Q_SLOTS:
    void audioFilterChanged(const QString& name);
    void videoFilterChanged(const QString& name);
private:
    struct {
        QString type;
        QCheckBox *enable;
        QComboBox *name;
        QLabel *description;
        QTextEdit *options;
    } m_ui[2]; //0: video, 1: audio
};

#endif // AVFILTERCONFIGPAGE_H
