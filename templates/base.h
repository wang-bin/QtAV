
#ifndef QTAV_%CLASS:u%_H
#define QTAV_%CLASS:u%_H

#include <QtAV/QtAV_Global.h>

namespace QtAV {

class %CLASS%Private;
class Q_AV_EXPORT %CLASS%
{
    DPTR_DECLARE_PRIVATE(%CLASS%)
public:
    virtual ~%CLASS%() = 0;
protected:
    %CLASS%(%CLASS%Private &d);
    DPTR_DECLARE(%CLASS%)
};

} //namespace QtAV

#endif // QTAV_%CLASS:u%_H
