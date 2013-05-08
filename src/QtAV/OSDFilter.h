#ifndef QTAV_OSDFILTER_H
#define QTAV_OSDFILTER_H

#include <QtAV/Filter.h>
#include <QtAV/OSD.h>
#include <QtAV/FilterContext.h>
namespace QtAV {

class OSDFilterPrivate;
class OSDFilter : public Filter
{
    DPTR_DECLARE_PRIVATE(OSDFilter)
public:
    enum ShowType {
        ShowCurrentTime = 1,
        ShowCurrentAndTotalTime = 1<<1,
        ShowRemainTime = 1<<2,
        ShowPercent = 1<<3,
        ShowNone
    };

    virtual ~OSDFilter();
    void setShowType(ShowType type);
    ShowType showType() const;
    void useNextShowType();
    bool hasShowType(ShowType t) const;
    void setCurrentTime(int currentSeconds);
    void setTotalTime(int totalSeconds);
    void setImageSize(int width, int height); //TODO: move to VideoFrameContext
protected:
    OSDFilter(OSDFilterPrivate& d);
};

template<class Context>
class OSDFilter2 : public Filter, public OSD
{
public:
    OSDFilter2();
    ~OSDFilter2() {}
    virtual void process(FilterContext* context, Statistics* statistics);
};

typedef OSDFilter2<QPainterFilterContext> OSFilterQPainter2;

} //namespace QtAV

#endif // QTAV_OSDFILTER_H
