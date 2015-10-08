
#ifndef QTAV_%CLASS:u%_P_H
#define QTAV_%CLASS:u%_P_H

#include <QtAV/QtAV_Global.h>

namespace QtAV {

class %CLASS%;
class Q_AV_PRIVATE_EXPORT %CLASS%Private : public DPtrPrivate<%CLASS%>
{
public:
    virtual ~%CLASS%Private() {}
};

} //namespace QtAV

#endif // QTAV_%CLASS%_P_H
