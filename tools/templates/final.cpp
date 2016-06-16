
#include "QtAV/%CLASS%.h"
#include "private/%BASE%_p.h"

namespace QtAV {

class %CLASS%Private : public %BASE%Private
{
public:
    %CLASS%Private() {}
    virtual ~%CLASS%Private() {}
};

%CLASS%::%CLASS%():
    %BASE%(*new %CLASS%Private())
{
}

%CLASS%::~%CLASS%()
{
}

} //namespace QtAV
