
#ifndef QTAV_%CLASS:u%_H
#define QTAV_%CLASS:u%_H

#include <QtAV/%BASE%.h>

namespace QtAV {

class %CLASS%Private;
class Q_AV_EXPORT %CLASS% : public %BASE%
{
    DPTR_DECLARE_PRIVATE(%CLASS%)
public:
    %CLASS%();
    virtual ~%CLASS%();
};

} //namespace QtAV

#endif // QTAV_%CLASS:u%_H
