#ifndef VUMeterFilter_H
#define VUMeterFilter_H
#include <QtAV/Filter.h>

using namespace QtAV;
class VUMeterFilter : public AudioFilter
{
    Q_OBJECT
    Q_PROPERTY(int leftLevel READ leftLevel WRITE setLeftLevel NOTIFY leftLevelChanged)
    Q_PROPERTY(int rightLevel READ rightLevel WRITE setRightLevel NOTIFY rightLevelChanged)
public:
    VUMeterFilter(QObject* parent = NULL);
    int leftLevel() const {return mLeft;}
    void setLeftLevel(int value) {mLeft = value;}
    int rightLevel() const {return mRight;}
    void setRightLevel(int value) {mRight = value;}
Q_SIGNALS:
    void leftLevelChanged(int value);
    void rightLevelChanged(int value);
protected:
    void process(Statistics* statistics, AudioFrame* frame = 0) Q_DECL_OVERRIDE;
private:
    int mLeft;
    int mRight;
};

#endif // VUMeterFilter_H
