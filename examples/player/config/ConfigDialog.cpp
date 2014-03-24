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

#include "ConfigDialog.h"
#include <QtCore/QFile>
#include <QLayout>
#include <QPushButton>
#include "CaptureConfigPage.h"
#include "DecoderConfigPage.h"
#include "Config.h"
void ConfigDialog::display()
{
    static ConfigDialog *dialog = new ConfigDialog();
    dialog->show();
}

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *vbl = new QVBoxLayout();
    setLayout(vbl);

    mpContent = new QTabWidget();
    mpContent->setTabPosition(QTabWidget::West);

    mpButtonBox = new QDialogButtonBox(Qt::Horizontal);
    mpButtonBox->addButton(tr("Reset"), QDialogButtonBox::ResetRole);// (QDialogButtonBox::Reset);
    mpButtonBox->addButton(tr("Ok"), QDialogButtonBox::AcceptRole); //QDialogButtonBox::Ok
    mpButtonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
    mpButtonBox->addButton(tr("Apply"), QDialogButtonBox::ApplyRole);

    connect(mpButtonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(mpButtonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(mpButtonBox, SIGNAL(clicked(QAbstractButton*)), SLOT(onButtonClicked(QAbstractButton*)));

    vbl->addWidget(mpContent);
    vbl->addWidget(mpButtonBox);

    mPages << new CaptureConfigPage()
           << new DecoderConfigPage();

    foreach (ConfigPageBase* page, mPages) {
        page->applyOnUiChange(false);
        mpContent->addTab(page, page->name());
    }
}

void ConfigDialog::onButtonClicked(QAbstractButton *btn)
{
    qDebug("QDialogButtonBox clicked role=%d", mpButtonBox->buttonRole(btn));
    switch (mpButtonBox->buttonRole(btn)) {
    case QDialogButtonBox::ResetRole:
        qDebug("QDialogButtonBox ResetRole clicked");
        onReset();
        break;
    case QDialogButtonBox::AcceptRole:
    case QDialogButtonBox::ApplyRole:
        qDebug("QDialogButtonBox ApplyRole clicked");
        onApply();
        break;
    case QDialogButtonBox::DestructiveRole:
    case QDialogButtonBox::RejectRole:
        onCancel();
        break;
    default:
        break;
    }
}

void ConfigDialog::onReset()
{
    // TODO: check change
    foreach (ConfigPageBase* page, mPages) {
        page->reset();
    }
    QFile cf(Config::instance().defaultDir() + "/config.ini");
    if (!cf.remove()) {

    }
    Config::instance().reload();
}

void ConfigDialog::onApply()
{
    // TODO: check change
    foreach (ConfigPageBase* page, mPages) {
        page->apply();
    }
}

void ConfigDialog::onCancel()
{
    // TODO: check change
    foreach (ConfigPageBase* page, mPages) {
        page->cancel();
    }
}

