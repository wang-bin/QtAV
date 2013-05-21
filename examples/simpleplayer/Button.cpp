#include "Button.h"

Button::Button(QWidget *parent) :
    QPushButton(parent)
  , mState(NotFocused)
{
}

Button::Button(const QString& text, QWidget *parent) :
    QPushButton(text, parent)
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
    QPushButton::enterEvent(e);
    if (mIcons.isEmpty())
        return;
    setIconState(Focused);
    emit entered();
}

void Button::leaveEvent(QEvent *e)
{
    QPushButton::leaveEvent(e);
    if (mIcons.isEmpty())
        return;
    setIconState(NotFocused);
    emit leaved();
}

void Button::mousePressEvent(QMouseEvent *e)
{
    QPushButton::mousePressEvent(e);
    if (mIcons.isEmpty())
        return;
    setIconState(Pressed);
}

void Button::mouseReleaseEvent(QMouseEvent *e)
{
    QPushButton::mouseReleaseEvent(e);
    if (mIcons.isEmpty())
        return;
    setIconState(Focused);
}
