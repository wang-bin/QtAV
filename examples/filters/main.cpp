/******************************************************************************
    Simple Player:  this file is part of QtAV examples
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

#include <QtAV/QtAV>
#include <QtAVWidgets>
#include "SimpleFilter.h"

using namespace QtAV;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    if (a.arguments().contains(QString::fromLatin1("-h")) || a.arguments().contains(QString::fromLatin1("--help"))) {
        qDebug("Usage: %s [-vo qt/gl/d2d/gdi] [url/path]filename", a.applicationFilePath().section(QDir::separator(), -1).toUtf8().constData());
        qDebug("\n%s", aboutQtAV_PlainText().toUtf8().constData());
        return 0;
    }

    QString vo;
    int idx = a.arguments().indexOf(QString::fromLatin1("-vo"));
    if (idx > 0) {
        vo = a.arguments().at(idx+1);
    } else {
        QString exe(a.arguments().at(0));
        qDebug("exe: %s", exe.toUtf8().constData());
        int i = exe.lastIndexOf(QLatin1Char('-'));
        if (i > 0) {
            vo = exe.mid(i+1, exe.indexOf(QLatin1Char('.')) - i - 1);
        }
    }
    qDebug("vo: %s", vo.toUtf8().constData());
    bool opt_has_file = argc > idx + 2;
    vo = vo.toLower();
    if (vo != QLatin1String("gl") && vo != QLatin1String("d2d") && vo != QLatin1String("gdi") && vo != QLatin1String("xv") && vo != QLatin1String("x11"))
        vo = QString::fromLatin1("qpainter");
    QString title = QString::fromLatin1("QtAV %1 wbsecg1@gmail.com").arg(QtAV_Version_String_Long());
    VideoRenderer *renderer = 0;
    bool textfilter = false;
    if (vo == QLatin1String("gl")) {
        renderer = VideoRenderer::create(VideoRendererId_GLWidget2);
        textfilter = true;
    } else if (vo == QLatin1String("d2d")) {
        renderer = VideoRenderer::create(VideoRendererId_Direct2D);
    } else if (vo == QLatin1String("gdi")) {
        renderer = VideoRenderer::create(VideoRendererId_GDI);
    } else if (vo == QLatin1String("xv")) {
        renderer = VideoRenderer::create(VideoRendererId_XV);
        textfilter = true;
    } else if (vo == QLatin1String("x11")) {
        renderer = VideoRenderer::create(VideoRendererId_X11);
        textfilter = true;
    } else {
        renderer = VideoRenderer::create(VideoRendererId_Widget);
        textfilter = true;
    }
    if (!renderer) {
        QMessageBox::critical(0, QString::fromLatin1("QtAV"), QString::fromLatin1("vo '%1' not supported").arg(vo));
        return 1;
    }
    renderer->widget()->setAttribute(Qt::WA_DeleteOnClose);
    renderer->widget()->setWindowTitle(title);

    renderer->setOutAspectRatioMode(VideoRenderer::VideoAspectRatio);
    renderer->widget()->resize(renderer->widget()->width(), renderer->widget()->width()*9/16);
    renderer->widget()->show();
    AVPlayer player;
    player.addVideoRenderer(renderer);
    if (textfilter) {
        SimpleFilter *filter = new SimpleFilter(renderer->widget());
        filter->setText(QString::fromLatin1("Filter on Renderer"));
        VideoFilterContext *ctx = static_cast<VideoFilterContext*>(filter->context());
        ctx->rect = QRect(200, 150, 400, 60);
        ctx->opacity = 0.7;
        filter->enableRotate(false);
        filter->prepare();
        renderer->installFilter(filter);

        filter = new SimpleFilter(renderer->widget());
        filter->setText(QString());
        filter->setImage(QImage(QString::fromLatin1(":/images/qt-logo.png")));
        ctx = static_cast<VideoFilterContext*>(filter->context());
        ctx->rect = QRect(400, 80, 200, 200);
        ctx->opacity = 0.618;
        filter->enableRotate(true);
        filter->enableWaveEffect(false);
        filter->prepare();
        renderer->installFilter(filter);

        filter = new SimpleFilter(&player);
        filter->setText(QString::fromLatin1("<h1 style='color:#ffff00'>HTML Filter on<span style='color:#ff0000'>Video Frame</span></h2>"));
        filter->enableWaveEffect(false);
        filter->enableRotate(true);
        ctx = static_cast<VideoFilterContext*>(filter->context());
        ctx->rect = QRect(200, 100, 400, 60);
        filter->prepare();
        player.installFilter(filter);
    }
    SubtitleFilter *sub = new SubtitleFilter(&player);
    sub->setPlayer(&player);
    player.installFilter(sub);
    opt_has_file &= argc > idx + 2;
    if (opt_has_file) {
        player.play(a.arguments().last());
    }
    int ret = a.exec();
    return ret;
}
