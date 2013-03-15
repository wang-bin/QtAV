#ifndef QTAV_OSDFILTER_H
#define QTAV_OSDFILTER_H

#include <QtAV/Filter.h>

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

} //namespace QtAV

#endif // QTAV_OSDFILTER_H
