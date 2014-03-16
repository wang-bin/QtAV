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

#ifndef CAPTURECONFIGPAGE_H
#define CAPTURECONFIGPAGE_H

#include "ConfigPageBase.h"
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>

/*
 * TODO: ConfigPageBase: auto save(true for menu ui, false for dialog ui)
 * virtual public slot: apply()
 */

class Slider;
class CaptureConfigPage : public ConfigPageBase
{
    Q_OBJECT
public:
    explicit CaptureConfigPage(QWidget *parent = 0);
    virtual QString name() const;

public slots:
    virtual void apply(); //store the values on ui. call Config::xxx
    virtual void cancel(); //cancel the values on ui. values are from Config
    virtual void reset(); //reset to default

private slots:
    // only emit signals. no value stores.
    void changeDirByUi(const QString& dir);
    void changeFormatByUi(const QString& fmt);
    void changeQualityByUi(int q);
    void formatChanged(const QByteArray& fmt);
    void selectSaveDir();
    void browseCaptureDir();

private:
    QLineEdit *mpDir;
    QComboBox *mpFormat;
    Slider *mpQuality;
};

#endif // CAPTURECONFIGPAGE_H
