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

#include "DecoderConfigPage.h"
#include "Config.h"

#include <QListView>
#include <QSpinBox>
#include <QToolButton>
#include <QLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSpacerItem>

#include <QtAV/VideoDecoderTypes.h>
#include <QPainter>
#include <QtDebug>

QString GetDecoderDescription(const QString& name) {
    struct {
        const char *name;
        QString desc;
    } dec_desc[] = {
        { "FFmpeg", "FFmpeg (" + QObject::tr("Software") + ")" },
        { "CUDA", "NVIDIA CUVID (" + QObject::tr("Hardware") + ")" },
        { "DXVA", "DirectX Video Acceleration 2.0 (" + QObject::tr("Hardware") + ")" },
        { "VAAPI", "Video Acceleration API (" + QObject::tr("Hardware") + ")" },
        { 0, 0 }
    };
    for (int i = 0; dec_desc[i].name; ++i) {
        if (name == dec_desc[i].name)
            return dec_desc[i].desc;
    }
    return "";
}

// shared
static QVector<QtAV::VideoDecoderId> sDecodersUi;
static QVector<QtAV::VideoDecoderId> sPriorityUi;

using namespace QtAV;
class DecoderConfigPage::DecoderItemWidget : public QWidget
{
    Q_OBJECT
public:
    DecoderItemWidget(QWidget* parent = 0)
        : QWidget(parent) {
        mSelected = false;
        QVBoxLayout *vb = new QVBoxLayout;
        setLayout(vb);

        mpCheck = new QCheckBox();
        mpDesc = new QLabel();
        vb->addWidget(mpCheck);
        vb->addWidget(mpDesc);
        connect(mpCheck, SIGNAL(pressed()), SLOT(checkPressed())); // no this->mousePressEvent
        connect(mpCheck, SIGNAL(toggled(bool)), this, SIGNAL(enableChanged()));
    }
    void select(bool s) {
        mSelected = s;
        update();
    }

    void setChecked(bool c) { mpCheck->setChecked(c); }
    bool isChecked() const { return mpCheck->isChecked(); }
    void setName(const QString& name) { mpCheck->setText(name);}
    QString name() const { return mpCheck->text();}
    void setDescription(const QString& desc) { mpDesc->setText(desc); }
    QString description() const { return mpDesc->text();}

signals:
    void enableChanged();
    void selected(DecoderItemWidget*);
private slots:
    void checkPressed() {
        select(true);
        emit selected(this);
    }

protected:
    virtual void mousePressEvent(QMouseEvent *) {
        select(true);
        emit selected(this);
    }
    virtual void paintEvent(QPaintEvent *e) {
        if (mSelected) {
            QPainter p(this);
            p.fillRect(rect(), QColor(0, 100, 200, 100));
        }
        QWidget::paintEvent(e);
    }

private:
    bool mSelected;
    QCheckBox *mpCheck;
    QLabel *mpDesc;
};

DecoderConfigPage::DecoderConfigPage(QWidget *parent) :
    ConfigPageBase(parent)
{
    mpSelectedDec = 0;
    setWindowTitle("Video decoder config page");
    QVBoxLayout *vb = new QVBoxLayout;
    setLayout(vb);

    QLabel *label = new QLabel(tr("Threads"));
    QSpinBox *sb = new QSpinBox;
    sb->setEnabled(false);
    sb->setMinimum(0);
    sb->setMaximum(16);
    sb->setValue(Config::instance().decodingThreads());
    QHBoxLayout *hb = new QHBoxLayout;
    hb->addWidget(label);
    hb->addWidget(sb);
    vb->addLayout(hb);

    QFrame *frame = new QFrame();
    frame->setFrameShape(QFrame::HLine);
    vb->addWidget(frame);
    vb->addWidget(new QLabel(tr("Decoder") + " " + tr("Priorities")));

    sPriorityUi = Config::instance().decoderPriority();
    sDecodersUi = Config::instance().registeredDecoders();
    QStringList vds = Config::instance().decoderPriorityNames();
    QStringList vds_all = Config::instance().registeredDecoderNames();
    mpDecLayout = new QVBoxLayout;
    for (int i = 0; i < vds_all.size(); ++i) {
        QString name = vds_all.at(i);
        DecoderItemWidget *iw = new DecoderItemWidget();
        mDecItems.append(iw);
        iw->setName(name);
        iw->setDescription(GetDecoderDescription(name));
        iw->setChecked(vds.contains(name));
        connect(iw, SIGNAL(enableChanged()), SLOT(videoDecoderEnableChanged()));
        connect(iw, SIGNAL(selected(DecoderItemWidget*)), SLOT(onDecSelected(DecoderItemWidget*)));
        mpDecLayout->addWidget(iw);
    }
    vb->addLayout(mpDecLayout);
    vb->addSpacerItem(new QSpacerItem(width(), 10, QSizePolicy::Ignored, QSizePolicy::Expanding));

    mpUp = new QToolButton;
    mpUp->setText("Up");
    connect(mpUp, SIGNAL(clicked()), SLOT(priorityUp()));
    mpDown = new QToolButton;
    mpDown->setText("Down");
    connect(mpDown, SIGNAL(clicked()), SLOT(priorityDown()));

    hb = new QHBoxLayout;
    hb->addWidget(mpUp);
    hb->addWidget(mpDown);
    vb->addLayout(hb);
    connect(&Config::instance(), SIGNAL(decoderPriorityChanged(QVector<QtAV::VideoDecoderId>)), SLOT(onConfigChanged()));
    connect(&Config::instance(), SIGNAL(registeredDecodersChanged(QVector<QtAV::VideoDecoderId>)), SLOT(onConfigChanged()));
    updateDecodersUi();
}

QString DecoderConfigPage::name() const
{
    return tr("Decoder");
}

void DecoderConfigPage::apply()
{
    QStringList decs_all;
    QStringList decs;
    foreach (DecoderItemWidget *w, mDecItems) {
        decs_all.append(w->name());
        if (w->isChecked())
            decs.append(w->name());
    }
    sDecodersUi = idsFromNames(decs_all);
    sPriorityUi = idsFromNames(decs);
    Config::instance().registeredDecoders(sDecodersUi);
    Config::instance().decoderPriority(sPriorityUi);
}

void DecoderConfigPage::cancel()
{
    updateDecodersUi();
}

void DecoderConfigPage::reset()
{
}

void DecoderConfigPage::videoDecoderEnableChanged()
{
    QStringList names;
    foreach (DecoderItemWidget *iw, mDecItems) {
        if (iw->isChecked())
            names.append(iw->name());
    }
    sPriorityUi = idsFromNames(names);
    if (applyOnUiChange()) {
        Config::instance().decoderPriorityNames(names);
    } else {
//        emit Config::instance().decoderPriorityChanged(sPriorityUi);
    }
}

void DecoderConfigPage::priorityUp()
{
    if (!mpSelectedDec)
        return;
    int i = mDecItems.indexOf(mpSelectedDec);
    if (i == 0)
        return;
    DecoderItemWidget *iw = mDecItems.takeAt(i-1);
    mDecItems.insert(i, iw);
    mpDecLayout->removeWidget(iw);
    mpDecLayout->insertWidget(i, iw);
    QStringList decs_all;
    QStringList decs_p = Config::instance().decoderPriorityNames();
    QStringList decs;
    foreach (DecoderItemWidget *w, mDecItems) {
        decs_all.append(w->name());
        if (decs_p.contains(w->name()))
            decs.append(w->name());
    }
    sDecodersUi = idsFromNames(decs_all);
    sPriorityUi = idsFromNames(decs);
    if (applyOnUiChange()) {
        Config::instance().decoderPriorityNames(decs);
        Config::instance().registeredDecoderNames(decs);
    } else {
        //emit Config::instance().decoderPriorityChanged(idsFromNames(decs));
        //emit Config::instance().registeredDecodersChanged(idsFromNames(decs));
    }
}

void DecoderConfigPage::priorityDown()
{
    if (!mpSelectedDec)
        return;
    int i = mDecItems.indexOf(mpSelectedDec);
    if (i == mDecItems.size()-1)
        return;
    DecoderItemWidget *iw = mDecItems.takeAt(i+1);
    mDecItems.insert(i, iw);
    // why takeItemAt then insertItem does not work?
    mpDecLayout->removeWidget(iw);
    mpDecLayout->insertWidget(i, iw);

    QStringList decs_all;
    QStringList decs_p = Config::instance().decoderPriorityNames();
    QStringList decs;
    foreach (DecoderItemWidget *w, mDecItems) {
        decs_all.append(w->name());
        if (decs_p.contains(w->name()))
            decs.append(w->name());
    }
    sDecodersUi = idsFromNames(decs_all);
    sPriorityUi = idsFromNames(decs);
    if (applyOnUiChange()) {
        Config::instance().decoderPriorityNames(decs);
        Config::instance().registeredDecoderNames(decs_all);
    } else {
        //emit Config::instance().decoderPriorityChanged(idsFromNames(decs));
        //emit Config::instance().registeredDecodersChanged(idsFromNames(decs));
    }
}

void DecoderConfigPage::onDecSelected(DecoderItemWidget *iw)
{
    if (mpSelectedDec == iw)
        return;
    if (mpSelectedDec) {
        mpSelectedDec->select(false);
    }
    mpSelectedDec = iw;
}

void DecoderConfigPage::updateDecodersUi()
{
    QStringList names = idsToNames(sPriorityUi);
    QStringList all_names = idsToNames(sDecodersUi);
    //qDebug() << "updateDecodersUi " << this << " " << names << " all: " << all_names;
    int idx = 0;
    foreach (QString name, all_names) {
        DecoderItemWidget * iw = 0;
        for (int i = idx; i < mDecItems.size(); ++i) {
           if (mDecItems.at(i)->name() != name)
               continue;
           iw = mDecItems.at(i);
           break;
        }
        if (!iw)
            break;
        iw->setChecked(names.contains(iw->name()));
        int i = mDecItems.indexOf(iw);
        if (i != idx) {
            mDecItems.removeAll(iw);
            mDecItems.insert(idx, iw);
        }
        // why takeItemAt then insertItem does not work?
        if (mpDecLayout->indexOf(iw) != idx) {
            mpDecLayout->removeWidget(iw);
            mpDecLayout->insertWidget(idx, iw);
        }
        ++idx;
    }
}

void DecoderConfigPage::onConfigChanged()
{
    sPriorityUi = Config::instance().decoderPriority();
    sDecodersUi = Config::instance().registeredDecoders();
    updateDecodersUi();
}

#include "DecoderConfigPage.moc"
