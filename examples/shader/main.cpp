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
#include <QApplication>
#include <QtCore/QScopedPointer>
#include <QWidget>
using namespace QtAV;

#define GLSL(x) #x "\n"
class MediumBlurShader : public VideoShader {
public:
    const char* userFragmentShaderHeader() const {
        return GLSL(uniform float u_kernel[9];);
    }
    const char* userSample() const {
        return GLSL(
                    vec4 sample2d(sampler2D tex, vec2 pos, int p)
                    {
                        vec4 c = vec4(0.0);
                        c += texture(tex, pos -u_texelSize[p])*u_kernel[0];
                        c += texture(tex, pos + u_texelSize[p]*vec2(0.0, -1.0))*u_kernel[1];
                        c += texture(tex, pos + u_texelSize[p]*vec2(1.0, -1.0))*u_kernel[2];
                        c += texture(tex, pos + u_texelSize[p]*vec2(-1.0, 0.0))*u_kernel[3];
                        c += texture(tex, pos)*u_kernel[4];
                        c += texture(tex, pos + u_texelSize[p]*vec2(1.0, 0.0))*u_kernel[5];
                        c += texture(tex, pos + u_texelSize[p]*vec2(-1.0, 1.0))*u_kernel[6];
                        c += texture(tex, pos + u_texelSize[p]*vec2(0.0, 1.0))*u_kernel[7];
                        c += texture(tex, pos + u_texelSize[p])*u_kernel[8];
                        c.a = texture(tex, pos).a;
                        return c;
                    });
    }
    void setUserUniformValues() {
        const float v=1.0/9.0;
        static const float kernel[9] =  {
            v, v, v,
            v, v, v,
            v, v, v
        };
        program()->setUniformValueArray("u_kernel", kernel, 9, 1);
    }
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
    vo.widget()->setWindowTitle("Medium blur shader");
    vo0.widget()->show();
    vo.widget()->show();
    vo0.widget()->move(0, 0);
    vo.widget()->move(vo0.widget()->width(), vo0.widget()->y());

    player.play(a.arguments().at(1));

    QScopedPointer<VideoShader> shader;
    if (!vo.opengl())
        qFatal("No opengl in the renderer");
    shader.reset(new MediumBlurShader());
    vo.opengl()->setUserShader(shader.data());
    return a.exec();
}
