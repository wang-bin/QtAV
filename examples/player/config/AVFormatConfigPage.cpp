#include "AVFormatConfigPage.h"
#include <limits>
#include <QLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>

#include "common/Config.h"

AVFormatConfigPage::AVFormatConfigPage(QWidget *parent) :
    ConfigPageBase(parent)
{
    setObjectName("avformat");
    QGridLayout *gl = new QGridLayout();
    setLayout(gl);
    gl->setSizeConstraint(QLayout::SetFixedSize);
    int r = 0;
    m_direct = new QCheckBox(tr("Reduce buffering"));
    m_direct->setChecked(Config::instance().reduceBuffering());
    gl->addWidget(m_direct, r++, 0);
    gl->addWidget(new QLabel(tr("Probe size")), r, 0, Qt::AlignRight);
    m_probeSize = new QSpinBox();
    m_probeSize->setMaximum(std::numeric_limits<int>::max());
    m_probeSize->setMinimum(0);
    m_probeSize->setValue(Config::instance().probeSize());
    m_probeSize->setToolTip("0: auto");
    gl->addWidget(m_probeSize, r++, 1, Qt::AlignLeft);
    gl->addWidget(new QLabel(tr("Max analyze duration")), r, 0, Qt::AlignRight);
    m_analyzeDuration = new QSpinBox();
    m_analyzeDuration->setMaximum(std::numeric_limits<int>::max());
    m_analyzeDuration->setValue(Config::instance().analyzeDuration());
    m_analyzeDuration->setToolTip("0: auto. how many microseconds are analyzed to probe the input");
    gl->addWidget(m_analyzeDuration, r++, 1, Qt::AlignLeft);

    gl->addWidget(new QLabel(tr("Extra")), r, 0, Qt::AlignRight);
    m_extra = new QLineEdit();
    m_extra->setText(Config::instance().avformatExtra());
    m_extra->setToolTip("key1=value1 key2=value2 ...");
    gl->addWidget(m_extra, r++, 1, Qt::AlignLeft);
}


void AVFormatConfigPage::apply()
{
    Config::instance().probeSize(m_probeSize->value())
            .analyzeDuration(m_analyzeDuration->value())
            .reduceBuffering(m_direct->isChecked())
            .avformatExtra(m_extra->text());
}

QString AVFormatConfigPage::name() const
{
    return tr("AVFormat");
}

void AVFormatConfigPage::cancel()
{
    m_direct->setChecked(false);
    m_probeSize->setValue(5000000);
    m_analyzeDuration->setValue(5000000);
    m_extra->setText("");
}

void AVFormatConfigPage::reset()
{
}
