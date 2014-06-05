#ifndef AVFILTERCONFIGPAGE_H
#define AVFILTERCONFIGPAGE_H

#include "ConfigPageBase.h"

class QCheckBox;
class QTextEdit;
class AVFilterConfigPage : public ConfigPageBase
{
public:
    AVFilterConfigPage();
    virtual QString name() const;
public slots:
    virtual void apply(); //store the values on ui. call Config::xxx
    virtual void cancel(); //cancel the values on ui. values are from Config
    virtual void reset(); //reset to default

private:
    QCheckBox *m_enable;
    QTextEdit *m_options;
};

#endif // AVFILTERCONFIGPAGE_H
