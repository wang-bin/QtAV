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

#include "MiscPage.h"
#include <QLabel>
#include <QLayout>
#include "common/Config.h"

MiscPage::MiscPage()
{
    QGridLayout *gl = new QGridLayout();
    setLayout(gl);
    gl->setSizeConstraint(QLayout::SetFixedSize);
    int r = 0;
    m_preview_on = new QCheckBox(tr("Preview"));
    gl->addWidget(m_preview_on, r++, 0);

    gl->addWidget(new QLabel(tr("Force fps")), r, 0);
    m_fps = new QDoubleSpinBox();
    m_fps->setMinimum(-m_fps->maximum());
    m_fps->setToolTip("<= 0: " + tr("Ignore"));
    gl->addWidget(m_fps, r++, 1);

    gl->addWidget(new QLabel(tr("Progress update interval") + "(ms)"), r, 0);
    m_notify_interval = new QSpinBox();
    m_notify_interval->setEnabled(false);
    gl->addWidget(m_notify_interval, r++, 1);
    applyToUi();
}

QString MiscPage::name() const
{
    return tr("Misc");
}


void MiscPage::applyFromUi()
{
    Config::instance().setPreviewEnabled(m_preview_on->isChecked())
            .setForceFrameRate(m_fps->value());
            ;
}

void MiscPage::applyToUi()
{
    m_preview_on->setChecked(Config::instance().previewEnabled());
    m_fps->setValue(Config::instance().forceFrameRate());
    //m_notify_interval->setValue(Config::instance().avfilterOptions());
}
