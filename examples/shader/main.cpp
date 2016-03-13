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
#include <QtCore/qmath.h>
#include <QApplication>
#include <QtCore/QScopedPointer>
#include <QWidget>
using namespace QtAV;

#define GLSL(x) #x "\n"
class WaveShader : public QObject, public VideoShader {
public:
    WaveShader(QObject *parent = 0) : QObject(parent)
      , t(0)
      , A(0.06)
      , omega(5)
    {
        startTimer(20);
    }
    const char* userShaderHeader(QOpenGLShader::ShaderType type) const {
        if (type == QOpenGLShader::Vertex)
            return 0;
        return GLSL(
        uniform float u_kernel[9];
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
    void setUserUniformValues() {
        program()->setUniformValue("u_A", A);
        program()->setUniformValue("u_omega", omega);
        program()->setUniformValue("u_t", t);
    }
protected:
    void timerEvent(QTimerEvent*) {
        t+=2.0*M_PI/50.0;
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
    VideoOutput vo0, vo;
    AVPlayer player;
    player.addVideoRenderer(&vo);
    player.addVideoRenderer(&vo0);
    vo0.widget()->setWindowTitle("No shader");
    vo.widget()->setWindowTitle("Wave effect shader");
    vo0.widget()->show();
    vo.widget()->show();
    vo0.widget()->move(0, 0);
    vo.widget()->move(vo0.widget()->width(), vo0.widget()->y());

    player.play(a.arguments().at(1));

    QScopedPointer<VideoShader> shader;
    if (!vo.opengl())
        qFatal("No opengl in the renderer");
    shader.reset(new WaveShader());
    vo.opengl()->setUserShader(shader.data());
    return a.exec();
}
