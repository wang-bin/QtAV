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

#include "AVFilterConfigPage.h"
#include <QLayout>
#include <QCheckBox>
#include <QLabel>
#include <QTextEdit>
#include "common/Config.h"

AVFilterConfigPage::AVFilterConfigPage(QWidget *parent)
    : ConfigPageBase(parent)
{
    setObjectName("avfilter");
    QGridLayout *gl = new QGridLayout();
    setLayout(gl);
    gl->setSizeConstraint(QLayout::SetFixedSize);
    int r = 0;
    m_enable = new QCheckBox(tr("Enable"));
    gl->addWidget(m_enable, r++, 0);
    gl->addWidget(new QLabel(tr("Parameters")), r++, 0);
    m_options = new QTextEdit();
    gl->addWidget(m_options, r++, 0);
    applyToUi();
}

QString AVFilterConfigPage::name() const
{
    return "AVFilter";
}


void AVFilterConfigPage::applyFromUi()
{
    Config::instance().avfilterOptions(m_options->toPlainText())
            .avfilterEnable(m_enable->isChecked())
            ;
}

void AVFilterConfigPage::applyToUi()
{
    m_enable->setChecked(Config::instance().avfilterEnable());
    m_options->setText(Config::instance().avfilterOptions());
}
