/******************************************************************************
    QOptions: make command line options easy. https://github.com/wang-bin/qoptions
    Copyright (C) 2011-2015 Wang Bin <wbsecg1@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
******************************************************************************/

#include "qoptions.h"
#include <QtCore/QStringList>
#include <QtCore/QtDebug>

QOption::QOption()
{}

QOption::QOption(const char *name, const QVariant &defaultValue, Type type, const QString &description)
:mType(type),mDescription(description),mDefaultValue(defaultValue)
{
    setName(QLatin1String(name));
}

QOption::QOption(const char *name, Type type, const QString &description)
:mType(type),mDescription(description),mDefaultValue(QVariant())
{
    //if (mType==QOption::NoToken)
      //  mValue = false;
    setName(QLatin1String(name));
}
/*
QOption::QOption(const char *name, const QVariant& value, Type type, const QString &description)
:mType(type),mDescription(description),mDefaultValue(*value),mValue(value)
{qDebug("%s %s %d", __FILE__, __FUNCTION__, __LINE__);
	setName(name);
}
*/

QString QOption::shortName() const
{
	return mShortName;
}

QString QOption::longName() const
{
	return mLongName;
}

QString QOption::formatName() const
{
    if (mLongName.isEmpty())
        return QLatin1String("-") + mShortName;
    if (mShortName.isEmpty())
        return QLatin1String("--") + mLongName;
    return QString::fromLatin1("-%1 [--%2]").arg(mShortName).arg(mLongName);
}

QString QOption::description() const
{
	return mDescription;
}

QVariant QOption::value() const
{
    if (mValue.isValid())
        return mValue;
    return mDefaultValue;
}

void QOption::setValue(const QVariant &value)
{
    mValue = value;
}

bool QOption::isSet() const
{
    return mValue.isValid();
}

bool QOption::isValid() const
{
    return !shortName().isEmpty() || !longName().isEmpty();
}

void QOption::setType(QOption::Type type)
{
	mType = type;
	if (mType==NoToken)
        mValue = false;
}

QOption::Type QOption::type() const
{
	return mType;
}

QString QOption::help() const
{
	QString message = formatName();
	if (type()==QOption::SingleToken)
        message.append(QLatin1String(" value"));
	else if (type()==QOption::MultiToken)
        message.append(QLatin1String(" value1 ... valueN"));

    message = QString::fromLatin1("%1").arg(message, -33);
	message.append(mDescription);
    if (mDefaultValue.isValid() && !mDefaultValue.toString().isEmpty())
        message.append(QLatin1String(" (default: ") + mDefaultValue.toString() + QLatin1String(")"));
    return message;
}

bool QOption::operator <(const QOption& o) const
{
    return mType < o.type() || mShortName < o.shortName() || mLongName < o.longName() || mDescription < o.description();
}

static QString get_short(const QString& name)
{
    if (name.startsWith(QLatin1String("--")))
        return QString();
    if (name.startsWith(QLatin1String("-")))
        return name.mid(1);
    return name;
}

static QString get_long(const QString& name)
{
    if (name.startsWith(QLatin1String("--")))
        return name.mid(2);
    if (name.startsWith(QLatin1String("-")))
        return QString();
    return name;
}

void QOption::setName(const QString &name)
{
    int comma = name.indexOf(QLatin1Char(','));
	if (comma>0) {
        QString name1 = name.left(comma);
        QString name2 = name.mid(comma+1);
        if (name1.startsWith(QLatin1String("--"))) {
            mLongName = name1.mid(2);
            mShortName = get_short(name2);
        } else if (name1.startsWith(QLatin1String("-"))) {
            mShortName = name1.mid(1);
            mLongName = get_long(name2);
        } else {
            if (name2.startsWith(QLatin1String("--"))) {
                mLongName = name2.mid(2);
                mShortName = name1;
            } else if (name2.startsWith(QLatin1String("-"))) {
                mShortName = name2.mid(1);
                mLongName = name1;
            } else {
                mShortName = name2;
                mLongName = name1;
            }
        }
	} else {
        if (name.startsWith(QLatin1String("--")))
            mLongName = name.mid(2);
        else if (name.startsWith(QLatin1String("-")))
            mShortName = name.mid(1);
        else
            mShortName = name;
	}
}



QOptions::QOptions()
{
}

QOptions::~QOptions()
{
    mOptionGroupMap.clear();
    mOptions.clear();
}

bool QOptions::parse(int argc, const char *const*argv)
{
    if (mOptionGroupMap.isEmpty())
		return false;

    if (argc==1)
		return true;

	bool result = true;
    QStringList args;
    for (int i=1;i<argc;++i) {
        args.append(QString::fromLocal8Bit(argv[i]));
	}

	QStringList::Iterator it = args.begin();
    QList<QOption>::Iterator it_list;
    mOptions = mOptionGroupMap.keys();

    while (it != args.end()) {
        if (it->startsWith(QLatin1String("--"))) {
            int e = it->indexOf(QLatin1Char('='));
            for (it_list = mOptions.begin(); it_list != mOptions.end(); ++it_list) {
                if (it_list->longName() == it->mid(2,e-2)) {
                    if (it_list->type()==QOption::NoToken) {
                        it_list->setValue(true);
                        //qDebug("%d %s", __LINE__, qPrintable(it_list->value().toString()));
						it = args.erase(it);
						break;
					}
					if (e>0) { //
                        it_list->setValue(it->mid(e+1));
                        //qDebug("%d %s", __LINE__, qPrintable(it_list->value().toString()));
					} else {
						it = args.erase(it);
                        if (it == args.end())
                            break;
                        it_list->setValue(*it);
                        //qDebug("%d %s", __LINE__, qPrintable(it_list->value().toString()));
					}
					it = args.erase(it);
					break;
				}
			}
            if (it_list == mOptions.end()) {
                qWarning() << "unknown option: " << *it;
                result = false;
				++it;
            }
			//handle unknown option
        } else if (it->startsWith(QLatin1Char('-'))) {
            for (it_list = mOptions.begin(); it_list != mOptions.end(); ++it_list) {
                QString sname = it_list->shortName();
				int sname_len = sname.length(); //usally is 1
                //TODO: startsWith(-height,-h) Not endsWith, -oabco
                if (it->midRef(1).compare(sname) == 0) {
                    if (it_list->type() == QOption::NoToken) {
                        it_list->setValue(true);
						it = args.erase(it);
						break;
					}
                    if (it->length() == sname_len+1) {//-o abco
						it = args.erase(it);
                        if (it == args.end())
                            break;
                        it_list->setValue(*it);
                        //qDebug("%d %s", __LINE__, qPrintable(it_list->value().toString()));
                    } else {
                        it_list->setValue(it->mid(sname_len+1));
                        //qDebug("%d %s", __LINE__, qPrintable(it_list->value().toString()));
                    }
					it = args.erase(it);
					break;
				}
			}
            if (it_list==mOptions.end()) {
                qWarning() << "unknown option: " << *it;
                result = false;
				++it;
            }
			//handle unknown option
        } else {
            qWarning() << "unknown option: " << *it;
            ++it;
        }
	}
    if (!result) {
        print();
    }
	return result;
}

QOptions& QOptions::add(const QString &group_description)
{
	mCurrentDescription = group_description;
	return *this;
}

QOptions& QOptions::addDescription(const QString &description)
{
	mDescription = description;
	return *this;
}

QOptions& QOptions::operator ()(const char* name, const QString& description)
{
    QOption op(name, QOption::NoToken, description);
    mOptions.append(op);
    mOptionGroupMap.insert(op, mCurrentDescription);
	return *this;
}

QOptions& QOptions::operator ()(const char* name, QOption::Type type, const QString& description)
{
    QOption op(name, type, description);
    mOptions.append(op);
    mOptionGroupMap.insert(op, mCurrentDescription);
	return *this;
}

QOptions& QOptions::operator ()(const char* name, const QVariant& defaultValue, const QString& description)
{
    QOption op(name, defaultValue, QOption::SingleToken, description);
    mOptions.append(op);
    mOptionGroupMap.insert(op, mCurrentDescription);
	return *this;
}

QOptions& QOptions::operator ()(const char* name, const QVariant& defaultValue, QOption::Type type, const QString& description)
{
    QOption op(name, defaultValue, type, description);
    mOptions.append(op);
    mOptionGroupMap.insert(op, mCurrentDescription);
	return *this;
}
/*

QOptions& QOptions::operator ()(const char* name, const QVariant& value, QOption::Type type, const QString& description)
{
    QOption op(name, value, type, description);
    mOptions.append(op);
    mOptionGroupMap.insert(op, mCurrentDescription);
	return *this;
}
*/

QOption QOptions::option(const QString &name) const
{
    if (mOptions.isEmpty())
        return QOption();
    QList<QOption>::ConstIterator it_list;
    for (it_list=mOptions.constBegin(); it_list!=mOptions.constEnd(); ++it_list) {
        if (it_list->shortName()==name || it_list->longName()==name) {
            return *it_list;
        }
    }
    return QOption();
}

QVariant QOptions::value(const QString& name) const
{
    return option(name).value();
}

QVariant QOptions::operator [](const QString& name) const
{
    return value(name);
}

QString QOptions::help() const
{
    QString message = mDescription;
    QStringList groups = mOptionGroupMap.values();
    groups.removeDuplicates();
	QList<QString>::ConstIterator it;

    QList<QOption>::ConstIterator it_op;
    for (it=groups.constBegin(); it!=groups.constEnd(); ++it) {
        message.append(QLatin1String("\n")).append(*it);
        QList<QOption> options = mOptionGroupMap.keys(*it);
        for (it_op=options.constBegin();it_op!=options.constEnd();++it_op)
            message.append(QLatin1String("\n  ")).append(it_op->help());
	}
    return message;
}

void QOptions::print() const
{
    qDebug("%s", help().toUtf8().constData());
}
