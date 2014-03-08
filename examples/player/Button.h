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

#ifndef BUTTON_H
#define BUTTON_H

//TODO: on/off, like qicon

#include <QPushButton>
#include <QToolButton>
#include <QMap>

class Button : public QToolButton
{
    Q_OBJECT
public:
    //like QIcon::Mode
    //use icon state to avoid setIcon with the same icon
    enum IconState {
        Disabled, //inactive
        Pressed, //focused and pressed
        Focused, //mouse enter
        NotFocused //mouse leave
    };

    explicit Button(QWidget *parent = 0);
    explicit Button(const QString& text, QWidget *parent = 0);
    virtual ~Button();
    IconState iconState() const;
    void setIconState(IconState state, bool force = false);
    QIcon iconForState(IconState state) const;
    void setIconForState(IconState state, const QIcon& icon);

    void setIconWithSates(const QPixmap& pixmap, IconState s1 = NotFocused, IconState s2 = Disabled
            , IconState s3 = Pressed, IconState s4 = Focused);
signals:
    void entered();
    void leaved();

protected:
    //for change icon.
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);

private:
    IconState mState;
    QMap<IconState, QIcon> mIcons;
};

#endif // BUTTON_H
