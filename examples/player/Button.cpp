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

#include "Button.h"

Button::Button(QWidget *parent) :
    QToolButton(parent)
  , mState(NotFocused)
{
}

Button::Button(const QString& text, QWidget *parent) :
    QToolButton(parent)
  , mState(NotFocused)
{
}

Button::~Button()
{
}

Button::IconState Button::iconState() const
{
    return mState;
}

void Button::setIconState(IconState state, bool force)
{
    if (mState == state && !force)
        return;
    mState = state;
    setIcon(iconForState(mState));
}

QIcon Button::iconForState(IconState state) const
{
    if (!mIcons.contains(state))
        return mIcons.begin().value();
    return mIcons.value(state);;
}

void Button::setIconForState(IconState state, const QIcon &icon)
{
    mIcons.insert(state, icon);
}

void Button::setIconWithSates(const QPixmap &pixmap, IconState s1, IconState s2, IconState s3, IconState s4)
{
    int a = pixmap.width();
    int b = pixmap.height();
    int count = qMax(a, b)/qMin(a, b);
    bool hor = pixmap.height() < pixmap.width();
    QPoint dp;
    if (hor) {
        a /= count;
        dp.setX(a);
    } else {
        b /= count;
        dp.setY(b);
    }
    QRect r(0, 0, a, b);
    setIconForState(s1, pixmap.copy(r));
    if (count > 1) {
        r.translate(dp);
        setIconForState(s2, pixmap.copy(r));
        if (count > 2) {
            r.translate(dp);
            setIconForState(s3, pixmap.copy(r));
            if (count > 3) {
                r.translate(dp);
                setIconForState(s4, pixmap.copy(r));
            }
        }
    }
    setIconState(iconState(), true); //TODO: other states set to existing icon
}

void Button::enterEvent(QEvent *e)
{
    QToolButton::enterEvent(e);
    if (mIcons.isEmpty())
        return;
    setIconState(Focused);
    emit entered();
}

void Button::leaveEvent(QEvent *e)
{
    QToolButton::leaveEvent(e);
    if (mIcons.isEmpty())
        return;
    setIconState(NotFocused);
    emit leaved();
}

void Button::mousePressEvent(QMouseEvent *e)
{
    QToolButton::mousePressEvent(e);
    if (mIcons.isEmpty())
        return;
    setIconState(Pressed);
}

void Button::mouseReleaseEvent(QMouseEvent *e)
{
    QToolButton::mouseReleaseEvent(e);
    if (mIcons.isEmpty())
        return;
    setIconState(Focused);
}
