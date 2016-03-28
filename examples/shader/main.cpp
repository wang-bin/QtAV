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
#include <QtAV/VideoShaderObject.h>
#include <QtCore/qmath.h>
#include <QApplication>
#include <QtCore/QScopedPointer>
#include <QWidget>
using namespace QtAV;

#define GLSL(x) #x ""
class MyShader : public VideoShaderObject
{
public:
    MyShader() { startTimer(50);}
protected:
    void timerEvent(QTimerEvent*) {
        static int t = 0;
        const float b = float(50 - t%100)/50.0;
        const float c = float(50 - 4*t%100)/50.0;
        ++t;
        setProperty("bs", QVariant::fromValue(QVector<float>() << b << c));
    }
private:
    const char* userShaderHeader(QOpenGLShader::ShaderType type) const {
        if (type == QOpenGLShader::Vertex)
            return 0;
        return GLSL(uniform vec2 bs;);
    }
    const char* userPostProcess() const {
        return GLSL(
                    float lr = 0.3086;
                    float lg = 0.6094;
                    float lb = 0.0820;
                    float s = bs.g + 1.0;
                    float is = 1.0-s;
                    float ilr = is * lr;
                    float ilg = is * lg;
                    float ilb = is * lb;
                    mat4 m = mat4(
                        ilr+s, ilg  , ilb  , 0.0,
                        ilr  , ilg+s, ilb  , 0.0,
                        ilr  , ilg  , ilb+s, 0.0,
                        0.0  , 0.0  , 0.0  , 1.0);
                    gl_FragColor = m*gl_FragColor+bs.r;);
    }
};

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
    VideoOutput vo[4];
    QScopedPointer<VideoShader> shaders[4];
    AVPlayer player;
    struct {
        QByteArray title;
        VideoShader *shader;
    } shader_list[] = {
    {"No shader", NULL},
    {"Wave effect shader", new WaveShader()},
    {"Blur shader", new MediumBlurShader()},
    {"Brightness+Saturation. (VideoShaderObject dynamic properties)", new MyShader()}
    };
    vo[0].widget()->move(0, 0);
    for (int i = 0; i < 4; ++i) {
        if (!vo[i].opengl())
            qFatal("No opengl in the renderer");
        player.addVideoRenderer(&vo[i]);
        vo[i].widget()->setWindowTitle(shader_list[i].title);
        vo[i].widget()->show();
        vo[i].widget()->resize(500, 300);
        vo[i].widget()->move(vo[0].widget()->x() + (i%2)*vo[0].widget()->width(), vo[0].widget()->y() + (i/2)*vo[0].widget()->height());
        vo[i].opengl()->setUserShader(shader_list[i].shader);
        shaders[i].reset(shader_list[i].shader);
    }
    player.play(a.arguments().at(1));
    return a.exec();
}
