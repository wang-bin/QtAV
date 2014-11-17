#include "QtAV/AVInput.h"
#include "QtAV/private/AVInput_p.h"
#include <QtCore/QIODevice>
#include "utils/Logger.h"

namespace QtAV {

class QIODeviceInput : public AVInput
{
public:
    void setIODevice(QIODevice *dev);
    QIODevice* device() const;

    virtual bool isSeekable() Q_DECL_OVERRIDE;
    virtual qint64 read(char *data, qint64 maxSize) Q_DECL_OVERRIDE;
    virtual bool seek(qint64 offset, int from) Q_DECL_OVERRIDE;
    virtual qint64 position() const Q_DECL_OVERRIDE;
    /*!
     * \brief size
     * \return <=0 if not support
     */
    virtual qint64 size() const Q_DECL_OVERRIDE;
};

// qrc support
class QFileInput : public QIODeviceInput
{
public:
    void setFileName(const QString& name);
    QString fileName() const;
};

} //namespace QtAV
