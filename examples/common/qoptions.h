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

#ifndef QOPTIONS_H
#define QOPTIONS_H

#include <QtCore/QVariant>
#include <QtCore/QMap>
#include <QtCore/QList>

#if defined(BUILD_QOPT_LIB)
#  define QOPT_EXPORT Q_DECL_EXPORT
#elif defined(BUILD_QOPT_IMPORT)
#  define QOPT_EXPORT Q_DECL_IMPORT //only for vc?
#else
#  define QOPT_EXPORT
#endif

/*
 command line: some_cmd -short1 value --long1 value --long2=value -short_novalue --long_novalue...
 C++:
 QOptions get_global_options() {
    static QOptions ops = QOptions().addDescription("...")
                    .add("common option group")
                    ("long1,short1", default1, "description1")
                    ("--long2", default2, "description2")
                    ("short_novalue", "description3")
                    ("--long_novalue", "description4")
                    ;
    return ops;
 }
 QOptions options = get_global_options();
 options.add("special option group")(....)...;
 options.parse(argc, argv);
 int v1 = options.value("short1").toInt();
 */

class QOPT_EXPORT QOption {
public:
    // TODO: MultiToken -name value1 -name value2 ...
	enum Type {
		NoToken, SingleToken, MultiToken
	};
    QOption();
	explicit QOption(const char* name, const QVariant& defaultValue, Type type, const QString& description);
	explicit QOption(const char* name, Type type, const QString& description);
    //explicit QOption(const char* name, const QVariant& value, Type type, const QString& description);

	QString shortName() const;
	QString longName() const;
	QString formatName() const;
	QString description() const;
	QVariant value() const;
	void setValue(const QVariant& value);
    bool isSet() const;
    bool isValid() const;

	void setType(QOption::Type type);
	QOption::Type type() const;

    QString help() const;
    void print() const;
    bool operator <(const QOption& o) const;
private:
    /*!
     * \brief setName
     * short/long name format:
     * "--long", "-short", "short"
     * "long,short", "--long,short", "--long,-short", "long,-short"
     * "short,--long", "-short,long", "-short,--long"
     * \param name
     */
	void setName(const QString& name);

	QOption::Type mType;
	QString mShortName, mLongName, mDescription;
	QVariant mDefaultValue;
    QVariant mValue;
};


class QOPT_EXPORT QOptions {
public:
	//e.g. application information, copyright etc.
	QOptions();
    //QOptions(const QOptions& o);
	~QOptions();
    //QOptions& operator=(const QOptions& o);

    /*!
     * \brief parse
     * \param argc
     * \param argv
     * \return false if invalid option found
     */
	bool parse(int argc, const char*const* argv);
	QOptions& add(const QString& group_description);
	QOptions& addDescription(const QString& description);

    QOptions& operator ()(const char* name, const QString& description = QString());
    QOptions& operator ()(const char* name, QOption::Type type, const QString& description = QString());
    QOptions& operator ()(const char* name, const QVariant& defaultValue, const QString& description);
    QOptions& operator ()(const char* name, const QVariant& defaultValue, QOption::Type type, const QString& description = QString());
    //QOptions& operator ()(const char* name, QVariant* value, QOption::Type type, const QString& description = QString());

    QOption option(const QString& name) const;
    QVariant value(const QString& name) const;
    QVariant operator [](const QString& name) const;

    QString help() const;
    void print() const;
private:
	QString mDescription, mCurrentDescription;
    QList<QOption> mOptions;
    QMap<QOption, QString/*group*/> mOptionGroupMap;
};

#endif // QOPTIONS_H
