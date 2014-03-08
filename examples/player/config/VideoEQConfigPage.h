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

#ifndef VIDEOEQCONFIGPAGE_H
#define VIDEOEQCONFIGPAGE_H

#include <QWidget>

class QCheckBox;
class QComboBox;
class QPushButton;
class QSlider;
class VideoEQConfigPage : public QWidget
{
    Q_OBJECT
public:
    enum Engine {
        SWScale,
        GLSL,
        XV,
    };
    explicit VideoEQConfigPage(QWidget *parent = 0);
    void setEngines(const QVector<Engine>& engines);
    void setEngine(Engine engine);
    Engine engine() const;

    qreal brightness() const;
    qreal contrast() const;
    qreal hue() const;
    qreal saturation() const;

signals:
    void engineChanged();
    void brightnessChanged(int);
    void contrastChanged(int);
    void hueChanegd(int);
    void saturationChanged(int);

private slots:
    void onGlobalSet(bool);
    void onReset();
    void onEngineChangedByUI();

private:
    QCheckBox *mpGlobal;
    QComboBox *mpEngine;
    QSlider *mpBSlider, *mpCSlider, *mpSSlider;
    QSlider *mpHSlider;
    QPushButton *mpResetButton;
    Engine mEngine;
    QVector<Engine> mEngines;
};

#endif // VIDEOEQCONFIGPAGE_H
