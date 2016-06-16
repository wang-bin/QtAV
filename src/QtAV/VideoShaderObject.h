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
#ifndef QTAV_VIDEOSHADEROBJECT_H
#define QTAV_VIDEOSHADEROBJECT_H
#include <QtAV/VideoShader.h>
#include <QtCore/QObject>
#include <QtCore/QVector>

namespace QtAV {
// check and auto update properties in shader
class VideoShaderObjectPrivate;
/*!
 * \brief The VideoShaderObject class
 * User defined uniform names are bound to class meta properties (property signals are required)
 * and object dynamic properties.
 * Property value type T is limited to float, int, unsigned(ES3.0) and QVector<T>
 */
class Q_AV_EXPORT VideoShaderObject : public QObject, public VideoShader
{
    DPTR_DECLARE_PRIVATE(VideoShaderObject)
    Q_OBJECT
public:
    VideoShaderObject(QObject* parent = 0);
protected:
    VideoShaderObject(VideoShaderObjectPrivate &d, QObject* parent = 0);
    bool event(QEvent *event) Q_DECL_OVERRIDE;
private Q_SLOTS:
    void propertyChanged(int id);
private:
    void programReady() Q_DECL_OVERRIDE Q_DECL_FINAL;
};

class DynamicShaderObjectPrivate;
/*!
 * \brief The DynamicShaderObject class
 * Able to set custom shader code
 */
class Q_AV_EXPORT DynamicShaderObject : public VideoShaderObject
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(DynamicShaderObject)
    Q_PROPERTY(QString header READ header WRITE setHeader NOTIFY headerChanged)
    Q_PROPERTY(QString sample READ sample WRITE setSample NOTIFY sampleChanged)
    Q_PROPERTY(QString postProcess READ postProcess WRITE setPostProcess NOTIFY postProcessChanged)
public:
    DynamicShaderObject(QObject* parent = 0);
    QString header() const;
    void setHeader(const QString& text);
    QString sample() const;
    void setSample(const QString& text);
    QString postProcess() const;
    void setPostProcess(const QString& text);
Q_SIGNALS:
    void headerChanged();
    void sampleChanged();
    void postProcessChanged();
protected:
    DynamicShaderObject(DynamicShaderObjectPrivate &d, QObject* parent = 0);
private:
    const char* userShaderHeader(QOpenGLShader::ShaderType st) const Q_DECL_OVERRIDE;
    const char* userSample() const Q_DECL_OVERRIDE;
    const char* userPostProcess() const Q_DECL_OVERRIDE;
};
} //namespace QtAV
#endif //QTAV_VIDEOSHADEROBJECT_H
