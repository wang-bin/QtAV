#ifndef CONFIGPAGEBASE_H
#define CONFIGPAGEBASE_H

#include <QWidget>

class ConfigPageBase : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigPageBase(QWidget *parent = 0);
    virtual QString name() const = 0;
    void applyOnUiChange(bool a);
    // default is true. in dialog is false, must call ConfigDialog::apply() to apply
    bool applyOnUiChange() const;

public slots:
    /*!
     * \brief apply
     * store the values on ui. call Config::xxx(value)
     */
    virtual void apply();
    /*!
     * \brief cancel
     * cancel the values change on ui and set to values from Config
     */
    virtual void cancel();
    /*!
     * \brief reset
     * reset to default
     */
    virtual void reset();
private:
    bool mApplyOnUiChange;
};

#endif // CONFIGPAGEBASE_H
