/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
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

#include "PropertyEditor.h"
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QLayout>
#include <QScrollArea>
#include <QToolButton>
#include <QtDebug>

#include "../ClickableMenu.h"

PropertyEditor::PropertyEditor(QObject *parent) :
    QObject(parent)
{
}

void PropertyEditor::getProperties(QObject *obj)
{
    mMetaProperties.clear();
    mProperties.clear();
    mPropertyDetails.clear();
    if (!obj)
        return;
    const QMetaObject *mo = obj->metaObject();
    for (int i = 0; i < mo->propertyCount(); ++i) {
        QMetaProperty mp = mo->property(i);
        mMetaProperties.append(mp);
        QVariant v(mp.read(obj));
        if (mp.isEnumType()) {
            mProperties.insert(QString::fromLatin1(mp.name()), v.toInt());//mp.enumerator().valueToKey(v.toInt())); //always use string
        } else {
            mProperties.insert(QString::fromLatin1(mp.name()), v);
        }
        const QVariant detail = obj->property(QByteArray("detail_").append(mp.name()).constData());
        if (!detail.isNull())
            mPropertyDetails.insert(QString::fromLatin1(mp.name()), detail.toString());
    }
    mProperties.remove(QString::fromLatin1("objectName"));
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

void PropertyEditor::set(const QString &)
{

}

QString PropertyEditor::buildOptions()
{
    QString result;
    foreach (QMetaProperty mp, mMetaProperties) {
        if (qstrcmp(mp.name(), "objectName") == 0)
            continue;
        result += QString::fromLatin1("  * %1: ").arg(QString::fromLatin1(mp.name()));
        if (mp.isEnumType()) {
            if (mp.isFlagType())
                result += QString::fromLatin1("flag ");
            else
                result += QString::fromLatin1("enum ");
            QMetaEnum me(mp.enumerator());
            for (int i = 0; i < me.keyCount(); ++i) {
                result += QString::fromLatin1(me.key(i));
                result += QString::fromLatin1("=");
                result += QString::number(me.value(i));
                if (i < me.keyCount() - 1)
                    result += QString::fromLatin1(",");
            }
        } else if (mp.type() == QVariant::Int){
            result += QString::fromLatin1("int");
        } else if (mp.type() == QVariant::Double) {
            result += QString::fromLatin1("real");
        } else if (mp.type() == QVariant::String) {
            result += QString::fromLatin1("text");
        } else if (mp.type() == QVariant::Bool) {
            result += QString::fromLatin1("bool");
        }
        const QVariant detail =  mPropertyDetails.value(QString::fromLatin1(mp.name()));
        if (!detail.isNull())
            result += QString::fromLatin1("\n    > property detail: %1").arg(detail.toString());
        result += QString::fromLatin1("\n");
    }
    return result;
}

QWidget* PropertyEditor::buildUi(QObject *obj)
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
        value = mProperties[QString::fromLatin1(mp.name())];
        if (mp.isEnumType()) {
            if (mp.isFlagType()) {
                gl->addWidget(createWidgetForFlags(QString::fromLatin1(mp.name()), value, mp.enumerator(), obj ? obj->property(QByteArray("detail_").append(mp.name()).constData()).toString() : QString()), row, 0, Qt::AlignLeft | Qt::AlignVCenter);
            } else {
                gl->addWidget(new QLabel(QObject::tr(mp.name())), row, 0, Qt::AlignRight | Qt::AlignVCenter);
                gl->addWidget(createWidgetForEnum(QString::fromLatin1(mp.name()), value, mp.enumerator(), obj ? obj->property(QByteArray("detail_").append(mp.name()).constData()).toString() : QString()), row, 1, Qt::AlignLeft | Qt::AlignVCenter);
            }
        } else if (mp.type() == QVariant::Int || mp.type() == QVariant::UInt || mp.type() == QVariant::LongLong || mp.type() == QVariant::ULongLong){
            gl->addWidget(new QLabel(QObject::tr(mp.name())), row, 0, Qt::AlignRight | Qt::AlignVCenter);
            gl->addWidget(createWidgetForInt(QString::fromLatin1(mp.name()), value.toInt(), obj ? obj->property(QByteArray("detail_").append(mp.name()).constData()).toString() : QString()), row, 1, Qt::AlignLeft | Qt::AlignVCenter);
        } else if (mp.type() == QVariant::Double) {
            gl->addWidget(new QLabel(QObject::tr(mp.name())), row, 0, Qt::AlignRight | Qt::AlignVCenter);
            gl->addWidget(createWidgetForReal(QString::fromLatin1(mp.name()), value.toReal(), obj ? obj->property(QByteArray("detail_").append(mp.name()).constData()).toString() : QString()), row, 1, Qt::AlignLeft | Qt::AlignVCenter);
        } else if (mp.type() == QVariant::Bool) {
            gl->addWidget(createWidgetForBool(QString::fromLatin1(mp.name()), value.toBool(), obj ? obj->property(QByteArray("detail_").append(mp.name()).constData()).toString() : QString()), row, 0, 1, 2, Qt::AlignLeft);
        } else {
            gl->addWidget(new QLabel(QObject::tr(mp.name())), row, 0, Qt::AlignRight | Qt::AlignVCenter);
            gl->addWidget(createWidgetForText(QString::fromLatin1(mp.name()), value.toString(), !mp.isWritable(), obj ? obj->property(QByteArray("detail_").append(mp.name()).constData()).toString() : QString()), row, 1, Qt::AlignLeft | Qt::AlignVCenter);
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
    return QString();
}

QWidget* PropertyEditor::createWidgetForFlags(const QString& name, const QVariant& value, QMetaEnum me, const QString &detail, QWidget* parent)
{
    mProperties[name] = value;
    QToolButton *btn = new QToolButton(parent);
    if (!detail.isEmpty())
        btn->setToolTip(detail);
    btn->setObjectName(name);
    btn->setText(QObject::tr(name.toUtf8().constData()));
    btn->setPopupMode(QToolButton::InstantPopup);
    ClickableMenu *menu = new ClickableMenu(btn);
    menu->setObjectName(name);
    btn->setMenu(menu);
    for (int i = 0; i < me.keyCount(); ++i) {
        QAction * a = menu->addAction(QString::fromLatin1(me.key(i)));
        a->setCheckable(true);
        a->setData(me.value(i));
        a->setChecked(value.toInt() & me.value(i));
    }
    connect(menu, SIGNAL(triggered(QAction*)), SLOT(onFlagChange(QAction*)));
    return btn;
}

QWidget* PropertyEditor::createWidgetForEnum(const QString& name, const QVariant& value, QMetaEnum me, const QString &detail, QWidget* parent)
{
    mProperties[name] = value;
    QComboBox *box = new QComboBox(parent);
    if (!detail.isEmpty())
        box->setToolTip(detail);
    box->setObjectName(name);
    box->setEditable(false);
    for (int i = 0; i < me.keyCount(); ++i) {
        box->addItem(QString::fromLatin1(me.key(i)), me.value(i));
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

QWidget* PropertyEditor::createWidgetForInt(const QString& name, int value, const QString& detail, QWidget* parent)
{
    mProperties[name] = value;
    QSpinBox *box = new QSpinBox(parent);
    if (!detail.isEmpty())
        box->setToolTip(detail);
    box->setObjectName(name);
    box->setValue(value);
    connect(box, SIGNAL(valueChanged(int)), SLOT(onIntChange(int)));
    return box;
}

QWidget* PropertyEditor::createWidgetForReal(const QString& name, qreal value, const QString &detail, QWidget* parent)
{
    mProperties[name] = value;
    QDoubleSpinBox *box = new QDoubleSpinBox(parent);
    if (!detail.isEmpty())
        box->setToolTip(detail);
    box->setObjectName(name);
    box->setValue(value);
    connect(box, SIGNAL(valueChanged(double)), SLOT(onRealChange(qreal)));
    return box;
}

QWidget* PropertyEditor::createWidgetForText(const QString& name, const QString& value, bool readOnly, const QString& detail, QWidget* parent)
{
    mProperties[name] = value;
    QWidget *w = 0;
    if (readOnly) {
        QLabel *label = new QLabel(parent);
        w = label;
        label->setText(value);
    } else {
        QLineEdit *box = new QLineEdit(parent);
        w = box;
        box->setText(value);
        connect(box, SIGNAL(textChanged(QString)), SLOT(onTextChange(QString)));
    }
    if (!detail.isEmpty())
        w->setToolTip(detail);
    w->setObjectName(name);
    return w;
}

QWidget* PropertyEditor::createWidgetForBool(const QString& name, bool value, const QString &detail, QWidget* parent)
{
    mProperties[name] = value;
    QCheckBox *box = new QCheckBox(QObject::tr(name.toUtf8().constData()), parent);
    if (!detail.isEmpty())
        box->setToolTip(detail);
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

void PropertyEditor::onFlagChange(QAction *action)
{
    int value = mProperties[sender()->objectName()].toInt();
    int flag = action->data().toInt();
    if (action->isChecked()) {
        value |= flag;
    } else {
        value &= ~flag;
    }
    updatePropertyValue(sender()->objectName(), value);
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
