#ifndef BUTTON_H
#define BUTTON_H

//TODO: on/off, like qicon

#include <QPushButton>
#include <QMap>

class Button : public QPushButton
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
