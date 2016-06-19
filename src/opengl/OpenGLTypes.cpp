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
#include "QtAV/OpenGLTypes.h"
#include "opengl/OpenGLHelper.h"
#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include "utils/Logger.h"
namespace QtAV {
struct uniform_type_name {
    QByteArray name;
    Uniform::Type type;
} uniform_type_names[] ={
    {"sample2D", Uniform::Sampler},
    {"bool", Uniform::Bool},
    {"int", Uniform::Int},
    {"uint", Uniform::Int},
    {"float", Uniform::Float},
    {"vec2", Uniform::Vec2},
    {"vec3", Uniform::Vec3},
    {"vec4", Uniform::Vec4},
    {"mat2", Uniform::Mat2},
    {"mat3", Uniform::Mat3},
    {"mat4", Uniform::Mat4},
    {"bvec2", Uniform::BVec2},
    {"bvec3", Uniform::BVec3},
    {"bvec4", Uniform::BVec4},
    {"ivec2", Uniform::IVec2},
    {"ivec3", Uniform::IVec3},
    {"ivec4", Uniform::IVec4},
    {"uvec2", Uniform::UVec2},
    {"uvec3", Uniform::UVec3},
    {"uvec4", Uniform::UVec4},
    {"mat2x2", Uniform::Mat2},
    {"mat3x3", Uniform::Mat3},
    {"mat4x4", Uniform::Mat4},
    {"dmat2", Uniform::DMat2},
    {"dmat3", Uniform::DMat3},
    {"dmat4", Uniform::DMat4},
};

static Uniform::Type UniformTypeFromName(const QByteArray& name)
{
    for (const uniform_type_name* un = uniform_type_names; un < uniform_type_names + sizeof(uniform_type_names)/sizeof(uniform_type_names[0]); ++un) {
        if (un->name == name)
            return un->type;
    }
    return Uniform::Unknown;
}

static QByteArray UniformTypeToName(Uniform::Type ut)
{
    for (const uniform_type_name* un = uniform_type_names; un < uniform_type_names + sizeof(uniform_type_names)/sizeof(uniform_type_names[0]); ++un) {
        if (un->type == ut)
            return un->name;
    }
    return "unknown";
}

Uniform::Uniform(Type tp, int count)
    : dirty(true)
    , location(-1)
    , tuple_size(1)
    , array_size(1)
    , t(tp)
{
    setType(tp, count);
}

Uniform& Uniform::setType(Type tp, int count)
{
    t = tp;
    array_size = count;
    if (isVec()) {
        tuple_size = (t >> (V+1)) & ((1<<3) - 1);
    } else if (isMat()) {
        tuple_size = (t >> (M+1)) & ((1<<3) - 1);
        tuple_size *= tuple_size;
    }
    int element_size = sizeof(float);
    if (isInt() || isUInt() || isBool()) {
        element_size = sizeof(int);
    }
    data = QVector<int>(element_size/sizeof(int)*tupleSize()*arraySize());
    return *this;
}

template<typename T> bool set_uniform_value(QVector<int>& dst, const T* v, int count)
{
    Q_ASSERT(sizeof(T)*count <= sizeof(int)*dst.size() && "set_uniform_value: Bad type or array size");
    // why not dst.constData()?
    const QVector<int> old(dst);
    memcpy((char*)dst.data(), (const char*)v, count*sizeof(T));
    return old != dst;
}

template<> bool set_uniform_value<bool>(QVector<int>& dst, const bool* v, int count)
{
    const QVector<int> old(dst);
    for (int i = 0; i < count; ++i) {
        dst[i] = *(v + i);
    }
    return old != dst;
}

void Uniform::set(const float &v, int count)
{
    if (count <= 0)
        count = tupleSize()*arraySize();
    dirty = set_uniform_value(data, &v, count);
}

void Uniform::set(const int &v, int count)
{
    if (count <= 0)
        count = tupleSize()*arraySize();
    dirty = set_uniform_value(data, &v, count);

}

void Uniform::set(const unsigned &v, int count)
{
    if (count <= 0)
        count = tupleSize()*arraySize();
    dirty = set_uniform_value(data, &v, count);
}


void Uniform::set(const float *v, int count)
{
    if (count <= 0)
        count = tupleSize()*arraySize();
    dirty = set_uniform_value(data, v, count);
}

void Uniform::set(const int *v, int count)
{
    if (count <= 0)
        count = tupleSize()*arraySize();
    dirty = set_uniform_value(data, v, count);
}

void Uniform::set(const unsigned *v, int count)
{
    if (count <= 0)
        count = tupleSize()*arraySize();
    dirty = set_uniform_value(data, v, count);
}

void Uniform::set(const QVariant &v)
{
    if (tupleSize() > 1 || arraySize() > 1) {
        if (isFloat()) { //TODO: what if QVector<qreal> but uniform is float?
            set(v.value<QVector<float> >().data());
        } else if (isInt() || isBool()) {
            set(v.value<QVector<int> >().data());
        } else if (isUInt()) {
            set(v.value<QVector<unsigned> >().data());
        } else if (type() == Uniform::Sampler) {

        }
    } else {
        if (isFloat()) {
            set(v.toFloat());
        } else if (isInt() || isBool()) {
            set(v.toInt());
        } else if (isUInt()) {
            set(v.toUInt());
        } else if (type() == Uniform::Sampler) {

        }
    }
}

bool Uniform::setGL()
{
    if (location < 0) {
        return false;
    }
    switch (type()) {
    case Uniform::Bool:
    case Uniform::Int:
        gl().Uniform1iv(location, arraySize(), address<int>());
        break;
    case Uniform::Float:
        gl().Uniform1fv(location, arraySize(), address<float>());
        break;
    case Uniform::Vec2:
        gl().Uniform2fv(location, arraySize(), address<float>());
        break;
    case Uniform::Vec3:
        gl().Uniform3fv(location, arraySize(), address<float>());
        break;
    case Uniform::Vec4:
        gl().Uniform4fv(location, arraySize(), address<float>());
        break;
    case Uniform::Mat2:
        gl().UniformMatrix2fv(location, arraySize(), GL_FALSE, address<float>());
        break;
    case Uniform::Mat3:
        gl().UniformMatrix3fv(location, arraySize(), GL_FALSE, address<float>());
        break;
    case Uniform::Mat4:
        gl().UniformMatrix4fv(location, arraySize(), GL_FALSE, address<float>());
        break;
    case Uniform::IVec2:
        gl().Uniform2iv(location, arraySize(), address<int>());
        break;
    case Uniform::IVec3:
        gl().Uniform3iv(location, arraySize(), address<int>());
        break;
    case Uniform::IVec4:
        gl().Uniform4iv(location, arraySize(), address<int>());
        break;
    default:
        qDebug() << *this;
        qWarning("Unsupported uniform type in Qt. You should use 'VideoShader::setUserUniformValues()' to call glUniformXXX or directly call glUniformXXX instead");
        return false;
    }
    dirty = false;
    return true;
}

#ifndef QT_NO_DEBUG_STREAM
Q_AV_EXPORT QDebug operator<<(QDebug dbg, const Uniform &u)
{
    dbg.nospace() << "uniform " << UniformTypeToName(u.type()) << " " << u.name.constData();
    if (u.arraySize() > 1) {
        dbg.nospace() << "[" << u.arraySize() << "]";
    }
    dbg.nospace() << ", dirty: " << u.dirty;
    dbg.nospace() << ", location: " << u.location << ", " << "tupleSize: " << u.tupleSize() << ", ";
    if (u.isBool() || u.isInt()) {
        dbg.nospace() << "value: " << u.value<int>();
    } else if (u.isUInt()) {
        dbg.nospace() << "value: " << u.value<unsigned>();
    } else if (u.isDouble()) {
        dbg.nospace() << "value: " << u.value<double>();
    } else {
        dbg.nospace() << "value: " << u.value<float>();
    }
    return dbg.space();
}

Q_AV_EXPORT QDebug operator<<(QDebug dbg, Uniform::Type ut);
#endif

QVector<Uniform> ParseUniforms(const QByteArray &text, GLuint programId = 0)
{
    QVector<Uniform> uniforms;
    const QString code = OpenGLHelper::removeComments(QString(text));
    const QStringList lines = code.split(';');
    // TODO: highp lowp etc.
    const QString exp(QStringLiteral("\\s*uniform\\s+([\\w\\d]+)\\s+([\\w\\d]+)\\s*"));
    const QString exp_array = exp + QStringLiteral("\\[(\\d+)\\]\\s*");
    foreach (QString line, lines) {
        line = line.trimmed();
        if (!line.startsWith(QStringLiteral("uniform ")))
            continue;
        QRegExp rx(exp_array);
        if (rx.indexIn(line) < 0) {
            rx = QRegExp(exp);
            if (rx.indexIn(line) < 0)
                continue;
        }
        Uniform u;
        const QStringList x = rx.capturedTexts();
        //qDebug() << x;
        u.name = x.at(2).toUtf8();
        int array_size = 1;
        if (x.size() > 3)
            array_size = x[3].toInt();
        const QByteArray t(x[1].toLatin1());
        u.setType(UniformTypeFromName(t), array_size);
        if (programId > 0)
            u.location = gl().GetUniformLocation(programId, u.name.constData());
        uniforms.append(u);
    }
    qDebug() << uniforms;
    return uniforms;
}
} //namespace QtAV
