/******************************************************************************
    Simple Player:  this file is part of QtAV examples
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
#include <QtAV>
#include <QtAV/GLSLFilter.h>
#include <QtAV/VideoShaderObject.h>
#include <QApplication>
#include <QtCore/QScopedPointer>
#include <QtMath>
#include <QWidget>
#include <QtDebug>

using namespace QtAV;
#define GLSL(x) #x ""

class WaveShader : public DynamicShaderObject {
    float t;
public:
    WaveShader(QObject *parent = 0) : DynamicShaderObject(parent)
      , t(0)
    {
        setHeader(QLatin1String(GLSL(
                      uniform float u_omega;
                      uniform float u_A;
                      uniform float u_t;
                      )));
        setSample(QLatin1String(GLSL(
                                    vec4 sample2d(sampler2D tex, vec2 pos, int p)
                                    {
                                        vec2 pulse = sin(u_t - u_omega * pos);
                                        vec2 coord = pos + u_A*vec2(pulse.x, -pulse.x);
                                        return texture(tex, coord);
                                    })));
        startTimer(40);
    }
protected:
    void timerEvent(QTimerEvent*) {
        t+=2.0*M_PI/25.0;
        setProperty("u_t", t);
    }
private:
    virtual void ready() {
        setProperty("u_t", 0);
        setProperty("u_A", 0.06);
        setProperty("u_omega", 5);
    }
};

int main(int argc, char** argv)
{
    QApplication a(argc, argv);
    if (a.arguments().size() < 2) {
        qDebug() << a.arguments().at(0) << " file";
        return 0;
    }
    VideoOutput vo;
    if (!vo.opengl()) {
        qFatal("opengl renderer is required!");
        return 1;
    }
    if (!vo.widget())
        return 1;
    GLSLFilter *f = new GLSLFilter();
    f->setOwnedByTarget();
    f->opengl()->setUserShader(new WaveShader(f));
    f->installTo(&vo);
    vo.widget()->show();
    vo.widget()->resize(800, 500);
    vo.widget()->setWindowTitle(QLatin1String("GLSLFilter example"));
    AVPlayer player;
    player.setRenderer(&vo);

    player.play(a.arguments().last());
    return a.exec();
}
