/******************************************************************************
    VideoEQConfigPage.h: description
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Alternatively, this file may be used under the terms of the GNU
    General Public License version 3.0 as published by the Free Software
    Foundation and appearing in the file LICENSE.GPL included in the
    packaging of this file.  Please review the following information to
    ensure the GNU General Public License version 3.0 requirements will be
    met: http://www.gnu.org/copyleft/gpl.html.
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
        GLSL
    };
    explicit VideoEQConfigPage(QWidget *parent = 0);
    void setEngines(const QVector<Engine>& engines);
    void setEngine(Engine engine);
    Engine engine() const;

signals:
    void engineChanged();
    void brightnessChanged(int);
    void contrastChanged(int);
    void saturationChanged(int);

private slots:
    void onGlobalSet(bool);
    void onReset();
    void onEngineChangedByUI();

private:
    QCheckBox *mpGlobal;
    QComboBox *mpEngine;
    QSlider *mpBSlider, *mpCSlider, *mpSSlider;
    QPushButton *mpResetButton;
    Engine mEngine;
};

#endif // VIDEOEQCONFIGPAGE_H
