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
#include <QtAV/ConvolutionShader.h>
#include <QtCore/qmath.h>
#include <QApplication>
#include <QtCore/QScopedPointer>
#include <QWidget>
using namespace QtAV;

#define GLSL(x) #x "\n"
class MediumBlurShader : public ConvolutionShader
{
public:
    MediumBlurShader() {setKernelRadius(2);}
protected:
    virtual const float* kernel() const {
        const float v = 1.0/(float)kernelSize(); //radius 1
        static QVector<float> k;
        k.resize(kernelSize());
        k.fill(v);
        return k.constData();
    }
};

class WaveShader : public QObject, public VideoShader {
public:
    WaveShader(QObject *parent = 0) : QObject(parent)
      , t(0)
      , A(0.06)
      , omega(5)
    {
        startTimer(20);
    }
protected:
    void timerEvent(QTimerEvent*) {
        t+=2.0*M_PI/50.0;
    }
private:
    const char* userShaderHeader(QOpenGLShader::ShaderType type) const {
        if (type == QOpenGLShader::Vertex)
            return 0;
        return GLSL(
        uniform float u_omega;
        uniform float u_A;
        uniform float u_t;
        );
    }
    const char* userSample() const {
        return GLSL(
                    vec4 sample2d(sampler2D tex, vec2 pos, int p)
                    {
                        vec2 pulse = sin(u_t - u_omega * pos);
                        vec2 coord = pos + u_A*vec2(pulse.x, -pulse.x);
                        return texture(tex, coord);
                    });
    }
    bool setUserUniformValues() {
        // return false; // enable this line to call setUserUniformValue(Uniform&)
        program()->setUniformValue("u_A", A);
        program()->setUniformValue("u_omega", omega);
        program()->setUniformValue("u_t", t);
        return true;
    }
    // setUserUniformValues() must return false to call setUserUniformValue(Uniform&)
    void setUserUniformValue(Uniform &u) {
        if (u.name == "u_A") {
            u.set(A);
        } else if (u.name == "u_omega") {
            u.set(omega);
        } else if (u.name == "u_t") {
            u.set(t);
        }
    }
private:
    float t;
    float A;
    float omega;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    if (a.arguments().size() < 2) {
        qDebug("./shader file");
        return 0;
    }
    VideoOutput vo, vo0, vo1;
    AVPlayer player;
    player.addVideoRenderer(&vo);
    player.addVideoRenderer(&vo0);
    player.addVideoRenderer(&vo1);
    vo.widget()->setWindowTitle("No shader");
    vo0.widget()->setWindowTitle("Wave effect shader");
    vo1.widget()->setWindowTitle("Blur shader");
    vo.widget()->show();
    vo.widget()->resize(500, 300);
    vo0.widget()->show();
    vo0.widget()->resize(500, 300);
    vo1.widget()->show();
    vo1.widget()->resize(500, 300);
    vo.widget()->move(0, 0);
    vo0.widget()->move(vo.widget()->x() + vo.widget()->width(), vo.widget()->y());
    vo1.widget()->move(vo.widget()->x(), vo.widget()->y() + vo.widget()->height());

    player.play(a.arguments().at(1));
    //player.setStartPosition(1000);
    //player.setStopPosition(1000);
    //player.setRepeat(-1);
    QScopedPointer<VideoShader> shader0;
    if (!vo0.opengl())
        qFatal("No opengl in the renderer");
    shader0.reset(new WaveShader());
    vo0.opengl()->setUserShader(shader0.data());
    QScopedPointer<VideoShader> shader1;
    if (!vo1.opengl())
        qFatal("No opengl in the renderer");
    shader1.reset(new MediumBlurShader());
    vo1.opengl()->setUserShader(shader1.data());

    return a.exec();
}
