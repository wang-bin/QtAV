/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2016)

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
#include "QtAV/VideoShaderObject.h"
#include "QtAV/private/VideoShader_p.h"
#include <QtCore/QEvent>
#include <QtCore/QMetaProperty>
#include <QtCore/QSignalMapper>

namespace QtAV {

class VideoShaderObjectPrivate : public VideoShaderPrivate
{
public:
    ~VideoShaderObjectPrivate() {
        qDeleteAll(sigMap[VertexShader]);
        qDeleteAll(sigMap[FragmentShader]);
        sigMap[VertexShader].clear();
        sigMap[FragmentShader].clear();
    }

    QVector<QSignalMapper*> sigMap[ShaderTypeCount];
};

VideoShaderObject::VideoShaderObject(QObject *parent)
    : QObject(parent)
    , VideoShader(*new VideoShaderObjectPrivate())
{}

VideoShaderObject::VideoShaderObject(VideoShaderObjectPrivate &d, QObject *parent)
    : QObject(parent)
    , VideoShader(d)
{}

bool VideoShaderObject::event(QEvent *event)
{
    DPTR_D(VideoShaderObject);
    if (event->type() != QEvent::DynamicPropertyChange)
        return QObject::event(event);
    QDynamicPropertyChangeEvent *e = static_cast<QDynamicPropertyChangeEvent*>(event);
    for (int shaderType = VertexShader; shaderType < ShaderTypeCount; ++shaderType) {
        QVector<Uniform> &uniforms = d.user_uniforms[shaderType];
        for (int i = 0; i < uniforms.size(); ++i) {
            if (uniforms.at(i).name == e->propertyName()) {
                propertyChanged(i|(shaderType<<16));
            }
        }
    }
    return QObject::event(event);
}

void VideoShaderObject::propertyChanged(int id)
{
    DPTR_D(VideoShaderObject);
    const int st = id>>16;
    const int idx = id&0xffff;
    Uniform &u = d.user_uniforms[st][idx];
    const QVariant v = property(u.name.constData());
    u.set(v);
    //if (u.dirty) update();
}

void VideoShaderObject::programReady()
{
    DPTR_D(VideoShaderObject);
    // find property name. if has property, bind to property
    for (int st = VertexShader; st < ShaderTypeCount; ++st) {
        qDeleteAll(d.sigMap[st]);
        d.sigMap[st].clear();
        const QVector<Uniform> &uniforms = d.user_uniforms[st];
        for (int i = 0; i < uniforms.size(); ++i) {
            const Uniform& u = uniforms[i];
            const int idx = metaObject()->indexOfProperty(u.name.constData());
            if (idx < 0) {
                qDebug("VideoShaderObject has no meta property '%s'. Setting initial value from dynamic property", u.name.constData());
                const_cast<Uniform&>(u).set(property(u.name.constData()));
                continue;
            }
            QMetaProperty mp = metaObject()->property(idx);
            if (!mp.hasNotifySignal()) {
                qWarning("VideoShaderObject property '%s' has no signal", mp.name());
                continue;
            }
            QMetaMethod mm = mp.notifySignal();
            QSignalMapper *mapper = new QSignalMapper();
            mapper->setMapping(this, i|(st<<16));
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
            connect(this, mm, mapper, mapper->metaObject()->method(mapper->metaObject()->indexOfSlot("map()")));
#else

#endif
            connect(mapper, SIGNAL(mapped(int)), this, SLOT(propertyChanged(int)));
            d.sigMap[st].append(mapper);
            qDebug() << "set uniform property: " << u.name << property(u.name.constData());
            propertyChanged(i|(st<<16)); // set the initial value
        }
    }
    //ready();
}

class DynamicShaderObjectPrivate : public VideoShaderObjectPrivate {
public:
    QString header;
    QString sampleFunc;
    QString pp;
};

DynamicShaderObject::DynamicShaderObject(QObject *parent)
    : VideoShaderObject(*new DynamicShaderObjectPrivate(), parent)
{}

DynamicShaderObject::DynamicShaderObject(DynamicShaderObjectPrivate &d, QObject *parent)
    : VideoShaderObject(d, parent)
{}

QString DynamicShaderObject::header() const
{
    return d_func().header;
}

void DynamicShaderObject::setHeader(const QString &text)
{
    DPTR_D(DynamicShaderObject);
    if (d.header == text)
        return;
    d.header = text;
    Q_EMIT headerChanged();
    rebuildLater();
}


QString DynamicShaderObject::sample() const
{
    return d_func().sampleFunc;
}

void DynamicShaderObject::setSample(const QString &text)
{
    DPTR_D(DynamicShaderObject);
    if (d.sampleFunc == text)
        return;
    d.sampleFunc = text;
    Q_EMIT sampleChanged();
    rebuildLater();
}

QString DynamicShaderObject::postProcess() const
{
    return d_func().pp;
}

void DynamicShaderObject::setPostProcess(const QString &text)
{
    DPTR_D(DynamicShaderObject);
    if (d.pp == text)
        return;
    d.pp = text;
    Q_EMIT postProcessChanged();
    rebuildLater();
}

const char* DynamicShaderObject::userShaderHeader(QOpenGLShader::ShaderType st) const
{
    if (st == QOpenGLShader::Vertex)
        return 0;
    if (d_func().header.isEmpty())
        return 0;
    return d_func().header.toUtf8().constData();
}

const char* DynamicShaderObject::userSample() const
{
    if (d_func().sampleFunc.isEmpty())
        return 0;
    return d_func().sampleFunc.toUtf8().constData();
}

const char* DynamicShaderObject::userPostProcess() const
{
    if (d_func().pp.isEmpty())
        return 0;
    return d_func().pp.toUtf8().constData();
}

} //namespace QtAV
