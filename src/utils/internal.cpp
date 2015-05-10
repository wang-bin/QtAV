/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

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
******************************************************************************/

#include "internal.h"
#include "QtAV/private/AVCompat.h"

namespace QtAV {
namespace Internal {

QString options2StringHelper(void* obj, const char* unit)
{
    QString s;
    const AVOption* opt = NULL;
    while ((opt = av_opt_next(obj, opt))) {
        if (opt->type == AV_OPT_TYPE_CONST) {
            if (!unit)
                continue;
            if (!qstrcmp(unit, opt->unit))
                s.append(QString(" %1=%2").arg(opt->name).arg(opt->default_val.i64));
            continue;
        } else {
            if (unit)
                continue;
        }
        s.append(QString("\n%1: ").arg(opt->name));
        switch (opt->type) {
        case AV_OPT_TYPE_FLAGS:
        case AV_OPT_TYPE_INT:
        case AV_OPT_TYPE_INT64:
            s.append(QString("(%1)").arg(opt->default_val.i64));
            break;
        case AV_OPT_TYPE_DOUBLE:
        case AV_OPT_TYPE_FLOAT:
            s.append(QString("(%1)").arg(opt->default_val.dbl, 0, 'f'));
            break;
        case AV_OPT_TYPE_STRING:
            if (opt->default_val.str)
                s.append(QString("(%1)").arg(opt->default_val.str));
            break;
        case AV_OPT_TYPE_RATIONAL:
            s.append(QString("(%1/%2)").arg(opt->default_val.q.num).arg(opt->default_val.q.den));
            break;
        default:
            break;
        }
        if (opt->help)
            s.append(" ").append(opt->help);
        if (opt->unit && opt->type != AV_OPT_TYPE_CONST)
            s.append("\n ").append(options2StringHelper(obj, opt->unit));
    }
    return s;
}

QString optionsToString(void* obj) {
    return options2StringHelper(obj, NULL);
}

} //namespace Internal
} //namespace QtAV
