/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TRACE_H
#define TRACE_H

#include <QDebug>

#define ENABLE_TRACE
//#define VERBOSE_TRACE

namespace Trace {

class NullDebug
{
public:
    template <typename T>
    NullDebug &operator<<(const T &) { return *this; }
};

inline NullDebug nullDebug() { return NullDebug(); }

template <typename T>
struct PtrWrapper
{
    PtrWrapper(const T *ptr) : m_ptr(ptr) { }
    const T *const m_ptr;
};

} // namespace Trace

template <typename T>
inline QDebug &operator<<(QDebug &debug, const Trace::PtrWrapper<T> &wrapper)
{
    debug.nospace() << "[" << (void*)wrapper.m_ptr << "]";
    return debug.space();
}

#ifdef ENABLE_TRACE
        inline QDebug qtTrace() { return qDebug() << "[qmlvideofx]"; }
#    ifdef VERBOSE_TRACE
        inline QDebug qtVerboseTrace() { return qtTrace(); }
#    else
        inline Trace::NullDebug qtVerboseTrace() { return Trace::nullDebug(); }
#    endif
#else
    inline Trace::NullDebug qtTrace() { return Trace::nullDebug(); }
    inline Trace::NullDebug qtVerboseTrace() { return Trace::nullDebug(); }
#endif

#endif // TRACE_H

