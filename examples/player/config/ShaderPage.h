/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2016)

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

#ifndef SHADER_PAGE_H
#define SHADER_PAGE_H
#include "ConfigPageBase.h"
#include <QTextEdit>
#include <QCheckBox>

class ShaderPage : public ConfigPageBase
{
public:
    ShaderPage(QWidget* parent = 0);
    virtual QString name() const;
protected:
    virtual void applyToUi();
    virtual void applyFromUi();
private:
    QCheckBox *m_enable;
    QCheckBox *m_fbo;
    QTextEdit *m_header;
    QTextEdit *m_sample;
    QTextEdit *m_pp;
};

#endif //SHADER_PAGE_H
