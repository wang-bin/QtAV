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
    m_preview_w = new QSpinBox();
    m_preview_w->setRange(1, 1920);
    m_preview_h = new QSpinBox();
    m_preview_h->setRange(1, 1080);
    gl->addWidget(new QLabel(tr("Preview") + " " + tr("size") + ": "), r, 0);
    QHBoxLayout *hb = new QHBoxLayout();
    hb->addWidget(m_preview_w);
    hb->addWidget(new QLabel("x"));
    hb->addWidget(m_preview_h);
    gl->addLayout(hb, r, 1);
    r++;
    gl->addWidget(new QLabel(tr("Force fps")), r, 0);
    m_fps = new QDoubleSpinBox();
    m_fps->setMinimum(-m_fps->maximum());
    m_fps->setToolTip("<= 0: " + tr("Ignore"));
    gl->addWidget(m_fps, r++, 1);

    gl->addWidget(new QLabel(tr("Progress update interval") + "(ms)"), r, 0);
    m_notify_interval = new QSpinBox();
    m_notify_interval->setEnabled(false);
    gl->addWidget(m_notify_interval, r++, 1);

    gl->addWidget(new QLabel(tr("Buffer frames")), r, 0);
    m_buffer_value = new QSpinBox();
    m_buffer_value->setRange(-1, 32767);
    m_buffer_value->setToolTip("-1: auto");
    gl->addWidget(m_buffer_value, r++, 1);

    gl->addWidget(new QLabel(tr("Timeout") + "(" + tr("s") +")"), r, 0);
    m_timeout = new QDoubleSpinBox();
    m_timeout->setDecimals(3);
    m_timeout->setSingleStep(1.0);
    m_timeout->setMinimum(-0.5);
    m_timeout->setToolTip("<=0: never");    
    m_timeout_abort = new QCheckBox(tr("Abort"));
    hb = new QHBoxLayout();
    hb->addWidget(m_timeout);
    hb->addWidget(m_timeout_abort);
    gl->addLayout(hb, r++, 1);

    gl->addWidget(new QLabel(tr("OpenGL type")), r, 0);
    m_opengl = new QComboBox();
    m_opengl->addItem("Auto", Config::Auto);
    m_opengl->addItem("Desktop", Config::Desktop);
    m_opengl->addItem("OpenGLES", Config::OpenGLES);
    m_opengl->addItem("Software", Config::Software);
    m_opengl->setToolTip(tr("Windows only") + "\n" + tr("OpenGLES is Used by DXVA Zero Copy"));
    gl->addWidget(m_opengl, r, 1);
    m_angle_platform = new QComboBox();
    m_angle_platform->setToolTip(tr("D3D9 has performance if ZeroCopy is disabled or for software decoders") + "\n" + tr("RESTART REQUIRED"));
    m_angle_platform->addItems(QStringList() << "D3D9" << "D3D11" << "AUTO" << "WARP");
    gl->addWidget(m_angle_platform, r++, 2);

    applyToUi();
}

QString MiscPage::name() const
{
    return tr("Misc");
}


void MiscPage::applyFromUi()
{
    Config::instance().setPreviewEnabled(m_preview_on->isChecked())
            .setPreviewWidth(m_preview_w->value())
            .setPreviewHeight(m_preview_h->value())
            .setOpenGLType((Config::OpenGLType)m_opengl->itemData(m_opengl->currentIndex()).toInt())
            .setANGLEPlatform(m_angle_platform->currentText().toLower())
            .setForceFrameRate(m_fps->value())
            .setBufferValue(m_buffer_value->value())
            .setTimeout(m_timeout->value())
            .setAbortOnTimeout(m_timeout_abort->isChecked())
            ;
}

void MiscPage::applyToUi()
{
    m_preview_on->setChecked(Config::instance().previewEnabled());
    m_preview_w->setValue(Config::instance().previewWidth());
    m_preview_h->setValue(Config::instance().previewHeight());
    m_opengl->setCurrentIndex(m_opengl->findData(Config::instance().openGLType()));
    m_angle_platform->setCurrentIndex(m_angle_platform->findText(Config::instance().getANGLEPlatform().toUpper()));
    m_fps->setValue(Config::instance().forceFrameRate());
    //m_notify_interval->setValue(Config::instance().avfilterOptions());
    m_buffer_value->setValue(Config::instance().bufferValue());
    m_timeout->setValue(Config::instance().timeout());
    m_timeout_abort->setChecked(Config::instance().abortOnTimeout());
}
