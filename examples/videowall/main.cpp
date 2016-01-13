/******************************************************************************
    VideoWall:  this file is part of QtAV examples
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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
#include <QFile>
#include <QMessageBox>
#include "VideoWall.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    int r = 3, c = 3;
    int idx = 0;
    if ((idx = a.arguments().indexOf(QLatin1String("-r"))) > 0)
        r = a.arguments().at(idx + 1).toInt();
    if ((idx = a.arguments().indexOf(QLatin1String("-c"))) > 0)
        c = a.arguments().at(idx + 1).toInt();
    QString vo;
    idx = a.arguments().indexOf(QLatin1String("-vo"));
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
    VideoWall wall;
    wall.setVideoRendererTypeString(vo.toLower());
    wall.setRows(r);
    wall.setCols(c);
    wall.show();
    QString file;
    if (a.arguments().size() > 1)
        file = a.arguments().last();
    if (QFile(file).exists()) {
        wall.play(file);
    } else {
        wall.help();
    }
    return a.exec();
}
