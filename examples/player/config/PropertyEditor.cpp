/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#include "PropertyEditor.h"
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QLayout>
#include <QScrollArea>
#include <QtDebug>

PropertyEditor::PropertyEditor(QObject *parent) :
    QObject(parent)
{
}

void PropertyEditor::getProperties(QObject *obj)
{
    mMetaProperties.clear();
    mProperties.clear();
    if (!obj)
        return;
    const QMetaObject *mo = obj->metaObject();
    for (int i = 0; i < mo->propertyCount(); ++i) {
        QMetaProperty mp = mo->property(i);
        mMetaProperties.append(mp);
        QVariant v(mp.read(obj));
        if (mp.isEnumType()) {
            mProperties.insert(mp.name(), v.toInt());//mp.enumerator().valueToKey(v.toInt())); //always use string
        } else {
            mProperties.insert(mp.name(), v);
        }
    }
    mProperties.remove("objectName");
}

void PropertyEditor::set(const QVariantHash &hash)
{
    QVariantHash::const_iterator it = mProperties.constBegin();
    for (;it != mProperties.constEnd(); ++it) {
        QVariantHash::const_iterator ti = hash.find(it.key());
        if (ti == hash.constEnd())
            continue;
        mProperties[it.key()] = ti.value();
    }
}

void PropertyEditor::set(const QString &conf)
{

}

QString PropertyEditor::buildOptions()
{
    QString result;
    foreach (QMetaProperty mp, mMetaProperties) {
        if (qstrcmp(mp.name(), "objectName") == 0)
            continue;
        result += mp.name();
        result += ": ";
        if (mp.isEnumType()) {
            QMetaEnum me(mp.enumerator());
            for (int i = 0; i < me.keyCount(); ++i) {
                result += me.key(i);
                if (i < me.keyCount() - 1)
                    result += ",";
            }
        } else if (mp.type() == QVariant::Int){
            result += "int";
        } else if (mp.type() == QVariant::Double) {
            result += "real";
        } else if (mp.type() == QVariant::String) {
            result += "text";
        } else if (mp.type() == QVariant::Bool) {
            result += "bool";
        }
        result += "\n";
    }
    return result;
}

QWidget* PropertyEditor::buildUi()
{
    if (mMetaProperties.isEmpty())
        return 0;
    QWidget *w = new QWidget();
    QGridLayout *gl = new QGridLayout();
    w->setLayout(gl);
    int row = 0;
    QVariant value;
    foreach (QMetaProperty mp, mMetaProperties) {
        if (qstrcmp(mp.name(), "objectName") == 0)
            continue;
        value = mProperties[mp.name()];
        if (mp.isEnumType()) {
            gl->addWidget(new QLabel(tr(mp.name())), row, 0, Qt::AlignRight | Qt::AlignVCenter);
            gl->addWidget(createWidgetForEnum(mp.name(), value, mp.enumerator()), row, 1, Qt::AlignLeft | Qt::AlignVCenter);
        } else if (mp.type() == QVariant::Int || mp.type() == QVariant::UInt || mp.type() == QVariant::LongLong || mp.type() == QVariant::ULongLong){
            gl->addWidget(new QLabel(tr(mp.name())), row, 0, Qt::AlignRight | Qt::AlignVCenter);
            gl->addWidget(createWidgetForInt(mp.name(), value.toInt()), row, 1, Qt::AlignLeft | Qt::AlignVCenter);
        } else if (mp.type() == QVariant::Double) {
            gl->addWidget(new QLabel(tr(mp.name())), row, 0, Qt::AlignRight | Qt::AlignVCenter);
            gl->addWidget(createWidgetForReal(mp.name(), value.toReal()), row, 1, Qt::AlignLeft | Qt::AlignVCenter);
        } else if (mp.type() == QVariant::Bool) {
            gl->addWidget(createWidgetForBool(mp.name(), value.toBool()), row, 0, 1, 2, Qt::AlignLeft);
        } else {
            gl->addWidget(new QLabel(tr(mp.name())), row, 0, Qt::AlignRight | Qt::AlignVCenter);
            gl->addWidget(createWidgetForText(mp.name(), value.toString()), row, 1, Qt::AlignLeft | Qt::AlignVCenter);
        }
        ++row;
    }
    return w;
}

QVariantHash PropertyEditor::exportAsHash()
{
    return mProperties;
}

QString PropertyEditor::exportAsConfig()
{
    return "";
}

QWidget* PropertyEditor::createWidgetForEnum(const QString& name, const QVariant& value, QMetaEnum me, QWidget* parent)
{
    mProperties[name] = value;
    QComboBox *box = new QComboBox(parent);
    box->setObjectName(name);
    box->setEditable(false);
    for (int i = 0; i < me.keyCount(); ++i) {
        box->addItem(me.key(i), me.value(i));
    }
    if (value.type() == QVariant::Int) {
        box->setCurrentIndex(box->findData(value));
    } else {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        box->setCurrentText(value.toString());
#else
        box->setCurrentIndex(box->findText(value.toString()));
#endif
    }
    connect(box, SIGNAL(currentIndexChanged(int)), SLOT(onEnumChange(int)));
    return box;
}

QWidget* PropertyEditor::createWidgetForInt(const QString& name, int value, QWidget* parent)
{
    mProperties[name] = value;
    QSpinBox *box = new QSpinBox(parent);
    box->setObjectName(name);
    box->setValue(value);
    connect(box, SIGNAL(valueChanged(int)), SLOT(onIntChange(int)));
    return box;
}

QWidget* PropertyEditor::createWidgetForReal(const QString& name, qreal value, QWidget* parent)
{
    mProperties[name] = value;
    QDoubleSpinBox *box = new QDoubleSpinBox(parent);
    box->setObjectName(name);
    box->setValue(value);
    connect(box, SIGNAL(valueChanged(double)), SLOT(onRealChange(qreal)));
    return box;
}

QWidget* PropertyEditor::createWidgetForText(const QString& name, const QString& value, QWidget* parent)
{
    mProperties[name] = value;
    QLineEdit *box = new QLineEdit(parent);
    box->setObjectName(name);
    box->setText(value);
    connect(box, SIGNAL(textChanged(QString)), SLOT(onTextChange(QString)));
    return box;
}

QWidget* PropertyEditor::createWidgetForBool(const QString& name, bool value, QWidget* parent)
{
    mProperties[name] = value;
    QCheckBox *box = new QCheckBox(tr(qPrintable(name)), parent);
    box->setObjectName(name);
    box->setChecked(value);
    connect(box, SIGNAL(clicked(bool)), SLOT(onBoolChange(bool)));
    return box;
}

void PropertyEditor::updatePropertyValue(const QString &name, const QVariant &value)
{
    if (name.isEmpty())
        return;
    if (!mProperties.contains(name))
        return;
    qDebug() << name << " >>> " << value;
    mProperties[name] = value;
}

void PropertyEditor::onEnumChange(int value)
{
    QComboBox *box =  qobject_cast<QComboBox*>(sender());
    updatePropertyValue(sender()->objectName(), box->itemData(value));
}

void PropertyEditor::onIntChange(int value)
{
    updatePropertyValue(sender()->objectName(), value);
}

void PropertyEditor::onRealChange(qreal value)
{
    updatePropertyValue(sender()->objectName(), value);
}

void PropertyEditor::onTextChange(const QString& value)
{
    updatePropertyValue(sender()->objectName(), value);
}

void PropertyEditor::onBoolChange(bool value)
{
    updatePropertyValue(sender()->objectName(), value);
}
