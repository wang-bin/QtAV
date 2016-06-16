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
#include "AVFormatConfigPage.h"
#include <limits>
#include <QLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>

#include "common/Config.h"

AVFormatConfigPage::AVFormatConfigPage(QWidget *parent) :
    ConfigPageBase(parent)
{
    setObjectName(QString::fromLatin1("avformat"));
    QGridLayout *gl = new QGridLayout();
    setLayout(gl);
    gl->setSizeConstraint(QLayout::SetFixedSize);
    int r = 0;
    m_on = new QCheckBox(QString::fromLatin1("%1 avformat %2").arg(tr("Enable")).arg(tr("options")));
    gl->addWidget(m_on, r++, 0);
    m_direct = new QCheckBox(tr("Reduce buffering"));
    gl->addWidget(m_direct, r++, 0);
    gl->addWidget(new QLabel(tr("Probe size")), r, 0, Qt::AlignRight);
    m_probeSize = new QSpinBox();
    m_probeSize->setMaximum(std::numeric_limits<int>::max());
    m_probeSize->setMinimum(0);
    m_probeSize->setToolTip(tr("0: auto"));
    gl->addWidget(m_probeSize, r++, 1, Qt::AlignLeft);
    gl->addWidget(new QLabel(tr("Max analyze duration")), r, 0, Qt::AlignRight);
    m_analyzeDuration = new QSpinBox();
    m_analyzeDuration->setMaximum(std::numeric_limits<int>::max());
    m_analyzeDuration->setToolTip(tr("0: auto. how many microseconds are analyzed to probe the input"));
    gl->addWidget(m_analyzeDuration, r++, 1, Qt::AlignLeft);

    gl->addWidget(new QLabel(tr("Extra")), r, 0, Qt::AlignRight);
    m_extra = new QLineEdit();
    m_extra->setToolTip(QString::fromLatin1("key1=value1 key2=value2 ..."));
    gl->addWidget(m_extra, r++, 1, Qt::AlignLeft);
}

QString AVFormatConfigPage::name() const
{
    return tr("AVFormat");
}

void AVFormatConfigPage::applyFromUi()
{
    Config::instance()
            .setAvformatOptionsEnabled(m_on->isChecked())
            .probeSize(m_probeSize->value())
            .analyzeDuration(m_analyzeDuration->value())
            .reduceBuffering(m_direct->isChecked())
            .avformatExtra(m_extra->text());
}

void AVFormatConfigPage::applyToUi()
{
    m_on->setChecked(Config::instance().avformatOptionsEnabled());
    m_direct->setChecked(Config::instance().reduceBuffering());
    m_probeSize->setValue(Config::instance().probeSize());
    m_analyzeDuration->setValue(Config::instance().analyzeDuration());
    m_extra->setText(Config::instance().avformatExtra());
}
