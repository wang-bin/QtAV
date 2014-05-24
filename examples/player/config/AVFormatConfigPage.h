#ifndef AVFORMATCONFIGPAGE_H
#define AVFORMATCONFIGPAGE_H

#include "ConfigPageBase.h"
#include <QtCore/QVariant>

class QCheckBox;
class QSpinBox;
class QLineEdit;
class AVFormatConfigPage : public ConfigPageBase
{
    Q_OBJECT
public:
    explicit AVFormatConfigPage(QWidget *parent = 0);
    virtual QString name() const;
public slots:
    virtual void apply(); //store the values on ui. call Config::xxx
    virtual void cancel(); //cancel the values on ui. values are from Config
    virtual void reset(); //reset to default

private:
    QCheckBox *m_direct;
    QSpinBox *m_probeSize;
    QSpinBox *m_analyzeDuration;
    QLineEdit *m_extra;
};

#endif // AVFORMATCONFIGPAGE_H
