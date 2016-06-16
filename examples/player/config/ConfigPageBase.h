/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#ifndef CONFIGPAGEBASE_H
#define CONFIGPAGEBASE_H

#include <QWidget>

class ConfigPageBase : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigPageBase(QWidget *parent = 0);
    virtual QString name() const = 0;
    void applyOnUiChange(bool a);
    // default is true. in dialog is false, must call ConfigDialog::apply() to apply
    bool applyOnUiChange() const;

public slots:
    // deprecated. call applyFromUi()
    void apply();
    // deprecated. call applyToUi().
    void cancel();
    //call applyToUi() after Config::instance().reset();
    void reset();
    /*!
     * \brief applyToUi
     * Apply configurations to UI. Call this in your page ctor when ui layout is ready.
     */
    virtual void applyToUi() = 0;
protected:
    /*!
     * \brief applyFromUi
     * Save configuration values from UI. Call Config::xxx(value) in your implementation
     */
    virtual void applyFromUi() = 0;
private:
    bool mApplyOnUiChange;
};

#endif // CONFIGPAGEBASE_H
