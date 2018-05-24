namespace QtAV
{

class AVOutput
{
%TypeHeaderCode
#include <QtAV/AVOutput.h>
%End

public:
    explicit AVOutput();
    virtual ~AVOutput();

private:
    AVOutput(const AVOutput& o);
};

};
