/******************************************************************************
    Simple Player:  this file is part of QtAV examples
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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
#include <cstdio>
#include <cstdlib>
#include <QApplication>

#include <QtDebug>
#include <QtCore/QDir>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QMessageBox>

#include <QtAV/AVPlayer.h>
#include <QtAV/VideoRendererTypes.h>
#include "SimpleFilter.h"

using namespace QtAV;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    if (a.arguments().contains("-h") || a.arguments().contains("--help")) {
        qDebug("Usage: %s [-vo qt/gl/d2d/gdi] [url/path]filename", a.applicationFilePath().section(QDir::separator(), -1).toUtf8().constData());
        qDebug("\n%s", aboutQtAV_PlainText().toUtf8().constData());
        return 0;
    }

    QString vo;
    int idx = a.arguments().indexOf("-vo");
    if (idx > 0) {
        vo = a.arguments().at(idx+1);
    } else {
        QString exe(a.arguments().at(0));
        qDebug("exe: %s", exe.toUtf8().constData());
        int i = exe.lastIndexOf('-');
        if (i > 0) {
            vo = exe.mid(i+1, exe.indexOf('.') - i - 1);
        }
    }
    qDebug("vo: %s", vo.toUtf8().constData());
    bool opt_has_file = argc > idx + 2;
    vo = vo.toLower();
    if (vo != "gl" && vo != "d2d" && vo != "gdi" && vo != "xv")
        vo = "qpainter";
    QString title = "QtAV " /*+ vo + " "*/ + QtAV_Version_String_Long() + " wbsecg1@gmail.com";
    VideoRenderer *renderer = 0;
    bool textfilter = false;
    if (vo == "gl") {
        renderer = VideoRendererFactory::create(VideoRendererId_GLWidget);
        textfilter = true;
    } else if (vo == "d2d") {
        renderer = VideoRendererFactory::create(VideoRendererId_Direct2D);
    } else if (vo == "gdi") {
        renderer = VideoRendererFactory::create(VideoRendererId_GDI);
    } else if (vo == "xv") {
        renderer = VideoRendererFactory::create(VideoRendererId_XV);
    } else {
        renderer = VideoRendererFactory::create(VideoRendererId_Widget);
        textfilter = true;
    }
    if (!renderer) {
        QMessageBox::critical(0, "QtAV", "vo '" + vo + "' not supported");
        return 1;
    }
    renderer->widget()->setWindowTitle(title);

    renderer->setOutAspectRatioMode(VideoRenderer::VideoAspectRatio);
    renderer->widget()->resize(renderer->widget()->width(), renderer->widget()->width()*9/16);
    renderer->widget()->show();
    AVPlayer player;
    player.addVideoRenderer(renderer);
    if (textfilter) {
        SimpleFilter *filter = new SimpleFilter(renderer->widget());
        filter->setText("Filter on Renderer");
        VideoFilterContext *ctx = static_cast<VideoFilterContext*>(filter->context());
        ctx->rect = QRect(200, 150, 400, 60);
        ctx->opacity = 0.7;
        filter->enableRotate(false);
        filter->prepare();
        renderer->installFilter(filter);

        filter = new SimpleFilter(renderer->widget());
        filter->setText(QString());
        filter->setImage(QImage(":/images/qt-logo.png"));
        ctx = static_cast<VideoFilterContext*>(filter->context());
        ctx->rect = QRect(400, 80, 200, 200);
        ctx->opacity = 0.618;
        filter->enableRotate(true);
        filter->enableWaveEffect(false);
        filter->prepare();
        renderer->installFilter(filter);

        filter = new SimpleFilter(renderer->widget());
        filter->setText("<h1 style='color:#ffff00'>HTML Filter on<span style='color:#ff0000'>Video Frame</span></h2>");
        filter->enableWaveEffect(false);
        filter->enableRotate(true);
        ctx = static_cast<VideoFilterContext*>(filter->context());
        ctx->rect = QRect(200, 100, 400, 60);
        filter->prepare();
        player.installVideoFilter(filter);
    }
    QString ao = "portaudio";
    idx = a.arguments().indexOf("-ao");
    if (idx > 0) {
        ao = a.arguments().at(idx+1);
    }
    ao = ao.toLower();
    player.enableAudio(ao != "null" && ao != "0");
    opt_has_file &= argc > idx + 2;
    if (opt_has_file) {
        player.play(a.arguments().last());
    }
    int ret = a.exec();
    return ret;
}
