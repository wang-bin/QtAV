namespace QtAV
{

class Frame
{
%TypeHeaderCode
#include <QtAV/Frame.h>
%End

public:
    Frame(const Frame& other);
    virtual ~Frame() = 0;

    // NOTE: We don't expose the assignment operators.
    //Frame& operator =(const Frame &other);
 
    int planeCount() const;
    virtual int channelCount() const;

    //int bytesPerLine(int plane = 0) const;
    //QByteArray frameData() const;
    //int dataAlignment() const;
    //uchar* frameDataPtr(int* size = NULL) const;
    //QByteArray data(int plane = 0) const;
    //uchar* bits(int plane = 0);
    //const uchar *bits(int plane = 0) const;
    //const uchar* constBits(int plane = 0) const;
    //void setBits(uchar *b, int plane = 0);
    //void setBits(const QVector<uchar*>& b);
    //void setBits(quint8 *slice[]);

    //void setBytesPerLine(int lineSize, int plane = 0);
    //void setBytesPerLine(const QVector<int>& lineSize);
    //void setBytesPerLine(int stride[]);

    QVariantMap availableMetaData() const;
    QVariant metaData(const QString& key) const;
    void setMetaData(const QString &key, const QVariant &value);
    void setTimestamp(qreal ts);
    qreal timestamp() const;
    void swap(Frame &other);
};

};
