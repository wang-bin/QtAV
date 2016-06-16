#include "QmlAV/QuickFilter.h"
#include "QtAV/private/Filter_p.h"
#include "QtAV/LibAVFilter.h"
#include "QtAV/GLSLFilter.h"
#include "QtAV/VideoShaderObject.h"
#include "QtAV/OpenGLVideo.h"

//namespace QtAV {

class QuickVideoFilterPrivate : public VideoFilterPrivate
{
public:
    QuickVideoFilterPrivate() : type(QuickVideoFilter::AVFilter)
      , avfilter(new LibAVFilterVideo())
      , glslfilter(new GLSLFilter())
    {
        filter = avfilter.data();
    }

    QuickVideoFilter::FilterType type;
    VideoFilter *filter;
    QScopedPointer<VideoFilter> user_filter;
    QScopedPointer<LibAVFilterVideo> avfilter;
    QScopedPointer<GLSLFilter> glslfilter;
};

QuickVideoFilter::QuickVideoFilter(QObject *parent)
    : VideoFilter(*new QuickVideoFilterPrivate(), parent)
{
    DPTR_D(QuickVideoFilter);
    connect(d.avfilter.data(), SIGNAL(optionsChanged()), this, SIGNAL(avfilterChanged()));
}

bool QuickVideoFilter::isSupported(VideoFilterContext::Type ct) const
{
    DPTR_D(const QuickVideoFilter);
    if (d.filter)
        return d.filter->isSupported(ct);
    return false;
}

QuickVideoFilter::FilterType QuickVideoFilter::type() const
{
    return d_func().type;
}

void QuickVideoFilter::setType(FilterType value)
{
    DPTR_D(QuickVideoFilter);
    if (d.type == value)
        return;
    d.type = value;
    if (value == GLSLFilter)
        d.filter = d.glslfilter.data();
    else if (value == AVFilter)
        d.filter = d.avfilter.data();
    else
        d.filter = d.user_filter.data();
    Q_EMIT typeChanged();
}

QStringList QuickVideoFilter::supportedAVFilters() const
{
    return d_func().avfilter->filters();
}

QString QuickVideoFilter::avfilter() const
{
    return d_func().avfilter->options();
}

void QuickVideoFilter::setAVFilter(const QString &options)
{
    d_func().avfilter->setOptions(options);
}

VideoFilter* QuickVideoFilter::userFilter() const
{
    return d_func().user_filter.data();
}

void QuickVideoFilter::setUserFilter(VideoFilter *f)
{
    DPTR_D(QuickVideoFilter);
    if (d.user_filter.data() == f)
        return;
    d.user_filter.reset(f);
    Q_EMIT userFilterChanged();
}

DynamicShaderObject* QuickVideoFilter::shader() const
{
    return static_cast<DynamicShaderObject*>(d_func().glslfilter->opengl()->userShader());
}

void QuickVideoFilter::setShader(DynamicShaderObject *value)
{
    DPTR_D(QuickVideoFilter);
    if (shader() == value)
        return;
    d.glslfilter->opengl()->setUserShader(value);
    Q_EMIT shaderChanged();
}

void QuickVideoFilter::process(Statistics *statistics, VideoFrame *frame)
{
    DPTR_D(QuickVideoFilter);
    if (!d.filter)
        return;
    d.filter->apply(statistics, frame);
}

class QuickAudioFilterPrivate : public AudioFilterPrivate
{
public:
    QuickAudioFilterPrivate() : AudioFilterPrivate()
      , type(QuickAudioFilter::AVFilter)
      , avfilter(new LibAVFilterAudio())
    {
        filter = avfilter.data();
    }

    QuickAudioFilter::FilterType type;
    AudioFilter *filter;
    QScopedPointer<AudioFilter> user_filter;
    QScopedPointer<LibAVFilterAudio> avfilter;
};

QuickAudioFilter::QuickAudioFilter(QObject *parent)
    : AudioFilter(*new QuickAudioFilterPrivate(), parent)
{
    DPTR_D(QuickAudioFilter);
    connect(d.avfilter.data(), SIGNAL(optionsChanged()), this, SIGNAL(avfilterChanged()));
}

QuickAudioFilter::FilterType QuickAudioFilter::type() const
{
    return d_func().type;
}

void QuickAudioFilter::setType(FilterType value)
{
    DPTR_D(QuickAudioFilter);
    if (d.type == value)
        return;
    d.type = value;
    if (value == AVFilter)
        d.filter = d.avfilter.data();
    else
        d.filter = d.user_filter.data();
    Q_EMIT typeChanged();
}

QStringList QuickAudioFilter::supportedAVFilters() const
{
    return d_func().avfilter->filters();
}

QString QuickAudioFilter::avfilter() const
{
    return d_func().avfilter->options();
}

void QuickAudioFilter::setAVFilter(const QString &options)
{
    d_func().avfilter->setOptions(options);
}

AudioFilter *QuickAudioFilter::userFilter() const
{
    return d_func().user_filter.data();
}

void QuickAudioFilter::setUserFilter(AudioFilter *f)
{
    DPTR_D(QuickAudioFilter);
    if (d.user_filter.data() == f)
        return;
    d.user_filter.reset(f);
    Q_EMIT userFilterChanged();
}

void QuickAudioFilter::process(Statistics *statistics, AudioFrame *frame)
{
    DPTR_D(QuickAudioFilter);
    if (!d.filter)
        return;
    d.filter->apply(statistics, frame);
}
//} //namespace QtAV
