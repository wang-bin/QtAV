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

#include "CaptureConfigPage.h"
#include "Config.h"
#include <QLabel>
#include <QFormLayout>
#include <QtGui/QImageWriter>
#include <QToolButton>
#include <QDesktopServices>
#include <QFileDialog>
#include <QtCore/QUrl>
#include "../Slider.h"

CaptureConfigPage::CaptureConfigPage(QWidget *parent) :
    ConfigPageBase(parent)
{
    QFormLayout *formLayout = new QFormLayout();
    setLayout(formLayout);
    QHBoxLayout *hb = new QHBoxLayout();
    mpDir = new QLineEdit();
    hb->addWidget(mpDir);
    QToolButton *bt = new QToolButton();
    bt->setText("...");
    hb->addWidget(bt);
    connect(bt, SIGNAL(clicked()), SLOT(selectSaveDir()));
    bt = new QToolButton();
    bt->setText(tr("Browse"));
    hb->addWidget(bt);
    connect(bt, SIGNAL(clicked()), SLOT(browseCaptureDir()));
    formLayout->addRow(tr("Save dir"), hb);
    mpDir->setEnabled(false);
    mpDir->setText(Config::instance().captureDir());
    mpFormat = new QComboBox();
    formLayout->addRow(tr("Save format"), mpFormat);
    QList<QByteArray> formats;
    formats << "YUV" << QImageWriter::supportedImageFormats();
    foreach (QByteArray fmt, formats) {
        mpFormat->addItem(fmt);
    }
    int idx = mpFormat->findText(Config::instance().captureFormat());
    mpFormat->setCurrentIndex(idx);
    mpQuality = new Slider();
    formLayout->addRow(tr("Quality"), mpQuality);
    mpQuality->setRange(0, 100);
    mpQuality->setOrientation(Qt::Horizontal);
    mpQuality->setValue(Config::instance().captureQuality());
    mpQuality->setSingleStep(1);
    mpQuality->setTickInterval(10);
    mpQuality->setTickPosition(QSlider::TicksBelow);

    connect(&Config::instance(), SIGNAL(captureDirChanged(QString)), mpDir, SLOT(setText(QString)));
    connect(&Config::instance(), SIGNAL(captureQualityChanged(int)), mpQuality, SLOT(setValue(int)));
    connect(&Config::instance(), SIGNAL(captureFormatChanged(QByteArray)), SLOT(formatChanged(QByteArray)));
    connect(mpDir, SIGNAL(textChanged(QString)), SLOT(changeDirByUi(QString)));
    connect(mpFormat, SIGNAL(currentIndexChanged(QString)), SLOT(changeFormatByUi(QString)));
    connect(mpQuality, SIGNAL(valueChanged(int)), SLOT(changeQualityByUi(int)));
}

void CaptureConfigPage::apply()
{
    Config::instance().captureDir(mpDir->text())
            .captureFormat(mpFormat->currentText().toUtf8())
            .captureQuality(mpQuality->value());
}

QString CaptureConfigPage::name() const
{
    return tr("Capture");
}

void CaptureConfigPage::cancel()
{
    mpDir->setText(Config::instance().captureDir());
    formatChanged(Config::instance().captureFormat());
    mpQuality->setValue(Config::instance().captureQuality());
}

void CaptureConfigPage::reset()
{

}

void CaptureConfigPage::changeDirByUi(const QString& dir)
{
    if (applyOnUiChange()) {
        Config::instance().captureDir(dir);
    } else {
        emit Config::instance().captureDirChanged(dir);
    }
}

void CaptureConfigPage::changeFormatByUi(const QString& fmt)
{
    if (applyOnUiChange()) {
        Config::instance().captureFormat(mpFormat->currentText().toUtf8());
    } else{
        emit Config::instance().captureFormatChanged(fmt.toUtf8());
    }
}

void CaptureConfigPage::changeQualityByUi(int q)
{
    if (applyOnUiChange()) {
        Config::instance().captureQuality(mpQuality->value());
    } else {
        emit Config::instance().captureQualityChanged(q);
    }
}

void CaptureConfigPage::formatChanged(const QByteArray& fmt)
{
    int idx = mpFormat->findText(fmt);
    if (idx >= 0)
        mpFormat->setCurrentIndex(idx);
}

void CaptureConfigPage::selectSaveDir()
{
    QString dir = QFileDialog::getExistingDirectory(0, tr("Save dir"), mpDir->text());
    if (dir.isEmpty())
        return;
    mpDir->setText(dir);
}

void CaptureConfigPage::browseCaptureDir()
{
    qDebug("browse capture dir");
    QDesktopServices::openUrl(QUrl("file:///" + mpDir->text()));
}
