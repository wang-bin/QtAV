#include "ConfigPageBase.h"

ConfigPageBase::ConfigPageBase(QWidget *parent) :
    QWidget(parent)
  , mApplyOnUiChange(true)
{
}

void ConfigPageBase::applyOnUiChange(bool a)
{
    mApplyOnUiChange = a;
}

bool ConfigPageBase::applyOnUiChange() const
{
    return mApplyOnUiChange;
}

void ConfigPageBase::apply()
{
}

void ConfigPageBase::cancel()
{
}

void ConfigPageBase::reset()
{
}
