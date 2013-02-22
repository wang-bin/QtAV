
#ifndef QTAV_%CLASS:u%_H
#define QTAV_%CLASS:u%_H

#include <QtAV/ImageRenderer.h>
#include <QWidget>

namespace QtAV {

class %CLASS%Private;
class Q_EXPORT %CLASS% : public QWidget, public ImageRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(%CLASS%)
public:
    %CLASS%(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~%CLASS%();
    virtual bool write();

    /* WA_PaintOnScreen: To render outside of Qt's paint system, e.g. If you require
     * native painting primitives, you need to reimplement QWidget::paintEngine() to
     * return 0 and set this flag
     */
    virtual QPaintEngine* paintEngine() const;
    //TODO: move to base class
	bool useQPainter() const;
	void useQPainter(bool qp);
protected:
    virtual void changeEvent(QEvent *event); //stay on top will change parent, we need GetDC() again
    virtual void resizeEvent(QResizeEvent *);
    virtual void paintEvent(QPaintEvent *);
};

} //namespace QtAV

#endif // QTAV_%CLASS%_H
