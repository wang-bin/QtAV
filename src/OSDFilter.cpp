#include "QtAV/OSDFilter.h"
#include "private/OSDFilter_p.h"
#include "QtAV/Statistics.h"
#include <QtGui/QPainter>

namespace QtAV {

OSDFilter::OSDFilter(OSDFilterPrivate &d):
    Filter(d)
{
}

OSDFilter::~OSDFilter()
{
}

void OSDFilter::setShowType(ShowType type)
{
    DPTR_D(OSDFilter);
    d.show_type = type;
}

OSDFilter::ShowType OSDFilter::showType() const
{
    return d_func().show_type;
}

void OSDFilter::useNextShowType()
{
    DPTR_D(OSDFilter);
    if (d.show_type == ShowNone) {
        d.show_type = (ShowType)1;
        return;
    }
    if (d.show_type + 1 == ShowNone) {
        d.show_type = ShowNone;
        return;
    }
    d.show_type = (ShowType)(d.show_type << 1);
}

bool OSDFilter::hasShowType(ShowType t) const
{
    DPTR_D(const OSDFilter);
    return (t&d.show_type) == t;
}

void OSDFilter::setCurrentTime(int currentSeconds)
{
    DPTR_D(OSDFilter);
    d.sec_current = currentSeconds;
}

void OSDFilter::setTotalTime(int totalSeconds)
{
    DPTR_D(OSDFilter);
    d.sec_total = totalSeconds;
    d.computeTime(totalSeconds, &d.total_hour, &d.total_min, &d.total_sec);
}

void OSDFilter::setImageSize(int width, int height)
{
    DPTR_D(OSDFilter);
    d.width = width;
    d.height = height;
}

template<>
void OSFilterQPainter2::process(FilterContext *context, Statistics *statistics)
{
    QPainterFilterContext* ctx = static_cast<QPainterFilterContext*>(context);
    if (!ctx->painter)
        return;
    ctx->painter->drawText(mPosition, statistics->video.current_time.toString("HH:mm:ss"));
}

} //namespace QtAV
