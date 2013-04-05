/******************************************************************************
    VideoWall:  this file is part of QtAV examples
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
#include <QFile>
#include <QMessageBox>
#include "VideoWall.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    int r = 3, c = 3;
    int idx = 0;
    if ((idx = a.arguments().indexOf("-r")) > 0)
        r = a.arguments().at(idx + 1).toInt();
    if ((idx = a.arguments().indexOf("-c")) > 0)
        c = a.arguments().at(idx + 1).toInt();
    VideoWall wall;
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
