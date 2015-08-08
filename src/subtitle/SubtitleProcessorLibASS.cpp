/******************************************************************************
    QtAV:  Media play library based on Qt and LibASS
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#include "QtAV/private/SubtitleProcessor.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include "QtAV/private/prepost.h"
#include "QtAV/Packet.h"
#include "PlainText.h"
#include "utils/Logger.h"

//#define ASS_CAPI_NS // do not unload() manually!
//#define CAPI_LINK_ASS
#include "ass_api.h"
#include <stdarg.h>
#include <string>  //include after ass_api.h, stdio.h is included there in a different namespace

namespace QtAV {

class SubtitleProcessorLibASS Q_DECL_FINAL: public SubtitleProcessor, protected ass::api
{
public:
    SubtitleProcessorLibASS();
    ~SubtitleProcessorLibASS();
    void updateFontCache();
    SubtitleProcessorId id() const Q_DECL_OVERRIDE;
    QString name() const Q_DECL_OVERRIDE;
    QStringList supportedTypes() const Q_DECL_OVERRIDE;
    bool process(QIODevice* dev) Q_DECL_OVERRIDE;
    // supportsFromFile must be true
    bool process(const QString& path) Q_DECL_OVERRIDE;
    QList<SubtitleFrame> frames() const Q_DECL_OVERRIDE;
    bool canRender() const Q_DECL_OVERRIDE { return true;}
    QString getText(qreal pts) const Q_DECL_OVERRIDE;
    QImage getImage(qreal pts, QRect *boundingRect = 0) Q_DECL_OVERRIDE;
    bool processHeader(const QByteArray& codec, const QByteArray& data) Q_DECL_OVERRIDE;
    SubtitleFrame processLine(const QByteArray& data, qreal pts = -1, qreal duration = 0) Q_DECL_OVERRIDE;
protected:
    void onFrameSizeChanged(int width, int height) Q_DECL_OVERRIDE;
private:
    bool initRenderer();
    void updateFontCacheAsync();
    // render 1 ass image into a 32bit QImage with alpha channel.
    //use dstX, dstY instead of img->dst_x/y because image size is small then ass renderer size
    void renderASS32(QImage *image, ASS_Image* img, int dstX, int dstY);
    void processTrack(ASS_Track *track);
    bool m_update_cache;
    QByteArray m_codec;
    ASS_Library *m_ass;
    ASS_Renderer *m_renderer;
    ASS_Track *m_track;
    QList<SubtitleFrame> m_frames;
    //cache the image for the last invocation. return this if image does not change
    QImage m_image;
    QRect m_bound;
    mutable QMutex m_mutex;
};

static const SubtitleProcessorId SubtitleProcessorId_LibASS = QStringLiteral("qtav.subtitle.processor.libass");
namespace {
static const std::string kName("LibASS");
}
FACTORY_REGISTER_ID_AUTO(SubtitleProcessor, LibASS, kName)

void RegisterSubtitleProcessorLibASS_Man()
{
    FACTORY_REGISTER_ID_MAN(SubtitleProcessor, LibASS, kName)
}

// log level from ass_utils.h
#define MSGL_FATAL 0
#define MSGL_ERR 1
#define MSGL_WARN 2
#define MSGL_INFO 4
#define MSGL_V 6
#define MSGL_DBG2 7

static void ass_msg_cb(int level, const char *fmt, va_list va, void *data)
{
    Q_UNUSED(data)
    QString msg(QStringLiteral("{libass} ") + QString().vsprintf(fmt, va));
    if (level == MSGL_FATAL)
        qFatal("%s", msg.toUtf8().constData());
    else if (level <= 2)
        qWarning() << msg;
    else if (level <= MSGL_INFO)
        qDebug() << msg;
}

SubtitleProcessorLibASS::SubtitleProcessorLibASS()
    : m_update_cache(true)
    , m_ass(0)
    , m_renderer(0)
    , m_track(0)
{
    if (!ass::api::loaded())
        return;
    m_ass = ass_library_init();
    if (!m_ass) {
        qWarning("ass_library_init failed!");
        return;
    }
    ass_set_message_cb(m_ass, ass_msg_cb, NULL);
}

SubtitleProcessorLibASS::~SubtitleProcessorLibASS()
{ // ass dll is loaded if ass objects are available
    if (m_track) {
        ass_free_track(m_track);
        m_track = 0;
    }
    if (m_renderer) {
        QMutexLocker lock(&m_mutex);
        Q_UNUSED(lock);
        ass_renderer_done(m_renderer); // check async update cache!!
        m_renderer = 0;
    }
    if (m_ass) {
        ass_library_done(m_ass);
        m_ass = 0;
    }
}

SubtitleProcessorId SubtitleProcessorLibASS::id() const
{
    return SubtitleProcessorId_LibASS;
}

QString SubtitleProcessorLibASS::name() const
{
    return QLatin1String(kName.c_str());//SubtitleProcessorFactory::name(id());
}

QStringList SubtitleProcessorLibASS::supportedTypes() const
{
    // from LibASS/tests/fate/subtitles.mak
    // TODO: mp4
    static const QStringList sSuffixes = QStringList() << QStringLiteral("ass") << QStringLiteral("ssa");
    return sSuffixes;
}

QList<SubtitleFrame> SubtitleProcessorLibASS::frames() const
{
    return m_frames;
}

bool SubtitleProcessorLibASS::process(QIODevice *dev)
{
    if (!ass::api::loaded())
        return false;
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    if (m_track) {
        ass_free_track(m_track);
        m_track = 0;
    }
    if (!dev->isOpen()) {
        if (!dev->open(QIODevice::ReadOnly)) {
            qWarning() << "open qiodevice error: " << dev->errorString();
            return false;
        }
    }
    QByteArray data(dev->readAll());
    m_track = ass_read_memory(m_ass, (char*)data.constData(), data.size(), NULL); //utf-8
    if (!m_track) {
        qWarning("ass_read_memory error, ass track init failed!");
        return false;
    }
    processTrack(m_track);
    return true;
}


bool SubtitleProcessorLibASS::process(const QString &path)
{
    if (!ass::api::loaded())
        return false;
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    if (m_track) {
        ass_free_track(m_track);
        m_track = 0;
    }
    m_track = ass_read_file(m_ass, (char*)path.toUtf8().constData(), NULL);
    if (!m_track) {
        qWarning("ass_read_file error, ass track init failed!");
        return false;
    }
    processTrack(m_track);
    return true;
}

bool SubtitleProcessorLibASS::processHeader(const QByteArray& codec, const QByteArray &data)
{
    if (!ass::api::loaded())
        return false;
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    m_codec = codec;
    m_frames.clear();
    setFrameSize(0, 0);
    if (m_track) {
        ass_free_track(m_track);
        m_track = 0;
    }
    m_track = ass_new_track(m_ass);
    if (!m_track) {
        qWarning("failed to create an ass track");
        return false;
    }
    ass_process_codec_private(m_track, (char*)data.constData(), data.size());
    return true;
}

SubtitleFrame SubtitleProcessorLibASS::processLine(const QByteArray &data, qreal pts, qreal duration)
{
    if (!ass::api::loaded())
        return SubtitleFrame();
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    if (!m_track)
        return SubtitleFrame();
    const int nb_tracks = m_track->n_events;
    // TODO: confirm. ass/ssa path from mpv
    if (m_codec == QByteArrayLiteral("ass")) {
        ass_process_chunk(m_track, (char*)data.constData(), data.size(), pts*1000.0, duration*1000.0);
    } else { //ssa
        //ssa. mpv: flush_on_seek, broken ffmpeg ASS packet format
        ass_process_data(m_track, (char*)data.constData(), data.size());
    }
    if (nb_tracks == m_track->n_events)
        return SubtitleFrame();
    //qDebug("events: %d", m_track->n_events);
    for (int i = m_track->n_events-1; i >= 0; --i) {
        const ASS_Event& ae = m_track->events[i];
        //qDebug("ass_event[%d] %lld+%lld/%lld+%lld: %s", i, ae.Start, ae.Duration, (long long)(pts*1000.0),  (long long)(duration*1000.0), ae.Text);
        //packet.duration can be 0
        if (ae.Start == (long long)(pts*1000.0)) {// && ae.Duration == (long long)(duration*1000.0)) {
            SubtitleFrame frame;
            frame.text = PlainText::fromAss(ae.Text);
            frame.begin = qreal(ae.Start)/1000.0;
            frame.end = frame.begin + qreal(ae.Duration)/1000.0;
            return frame;
        }
    }
    return SubtitleFrame();
}

QString SubtitleProcessorLibASS::getText(qreal pts) const
{
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    QString text;
    for (int i = 0; i < m_frames.size(); ++i) {
        if (m_frames[i].begin <= pts && m_frames[i].end >= pts) {
            text += m_frames[i].text + QStringLiteral("\n");
            continue;
        }
        if (!text.isEmpty())
            break;
    }
    return text.trimmed();
}

QImage SubtitleProcessorLibASS::getImage(qreal pts, QRect *boundingRect)
{ // ass dll is loaded if ass library is available
    {
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    if (!m_ass) {
        qWarning("ass library not available");
        return QImage();
    }
    if (!m_track) {
        qWarning("ass track not available");
        return QImage();
    }
    if (!m_renderer) {
        initRenderer();
        if (!m_renderer) {
            qWarning("ass renderer not available");
            return QImage();
        }
    }
    }
    if (m_update_cache)
        updateFontCache();

    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    int detect_change = 0;
    ASS_Image *img = ass_render_frame(m_renderer, m_track, (long long)(pts * 1000.0), &detect_change);
    if (!detect_change) {
        if (boundingRect)
            *boundingRect = m_bound;
        return m_image;
    }
    QRect rect(0, 0, 0, 0);
    ASS_Image *i = img;
    while (i) {
        rect |= QRect(i->dst_x, i->dst_y, i->w, i->h);
        i = i->next;
    }
    m_bound = rect;
    if (boundingRect) {
        *boundingRect = m_bound;
    }
    QImage image(rect.size(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    i = img;
    while (i) {
        if (i->w <= 0 || i->h <= 0) {
            i = i->next;
            continue;
        }
        renderASS32(&image, i, i->dst_x - rect.x(), i->dst_y - rect.y());
        i = i->next;
    }
    m_image = image;
    return image;
}

void SubtitleProcessorLibASS::onFrameSizeChanged(int width, int height)
{
    if (!m_renderer) {
        initRenderer();
        if (!m_renderer)
            return;
    }
    ass_set_frame_size(m_renderer, width, height);
}

bool SubtitleProcessorLibASS::initRenderer()
{
    m_renderer = ass_renderer_init(m_ass);
    if (!m_renderer) {
        qWarning("ass_renderer_init failed!");
        return false;
    }
#if LIBASS_VERSION >= 0x01000000
    ass_set_shaper(m_renderer, ASS_SHAPING_SIMPLE);
#endif
    return true;
}
// TODO: set font cache dir. default is working dir which may be not writable on some platforms
void SubtitleProcessorLibASS::updateFontCache()
{ // ass dll is loaded if renderer is valid
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    if (!m_renderer)
        return;
    static QByteArray conf; //FC_CONFIG_FILE?
    if (conf.isEmpty()) {
        conf = qgetenv("QTAV_FC_FILE");
        if (conf.isEmpty())
            conf = qApp->applicationDirPath().append(QLatin1String("/fonts/fonts.conf")).toUtf8();
    }
    static QByteArray font;
    if (font.isEmpty()) {
        font = qgetenv("QTAV_SUB_FONT_FILE_DEFAULT");
    }
    static QByteArray family;
    if (family.isEmpty()) {
        family = qgetenv("QTAV_SUB_FONT_FAMILY_DEFAULT");
    }
    //ass_set_fonts_dir(m_ass, font_dir.constData()); // we can set it in fonts.conf <dir></dir>
    // update cache later (maybe async update in the future)
    ass_set_fonts(m_renderer, font.isEmpty() ? NULL : font.constData(), family.isEmpty() ? NULL : family.constData(), 1, conf.constData(), 0);
    ass_fonts_update(m_renderer);
    m_update_cache = false;
}

void SubtitleProcessorLibASS::updateFontCacheAsync()
{
    class FontCacheUpdater : public QThread {
        SubtitleProcessorLibASS *sp;
    public:
        FontCacheUpdater(SubtitleProcessorLibASS *p) : sp(p) {}
        void run() {
            if (!sp)
                return;
            sp->updateFontCache();
        }
    };
    FontCacheUpdater updater(this);
    QEventLoop loop;
    //QObject::connect(&updater, SIGNAL(finished()), &loop, SLOT(quit()));
    updater.start();
    while (updater.isRunning()) {
        loop.processEvents();
    }
    //loop.exec(); // what if updater is finished before exec()?
    //updater.wait();
}

void SubtitleProcessorLibASS::processTrack(ASS_Track *track)
{
    // language, track type
    m_frames.clear();
    for (int i = 0; i < track->n_events; ++i) {
        SubtitleFrame frame;
        const ASS_Event& ae = track->events[i];
        frame.text = PlainText::fromAss(ae.Text);
        frame.begin = qreal(ae.Start)/1000.0;
        frame.end = frame.begin + qreal(ae.Duration)/1000.0;
        m_frames.append(frame);
    }
}

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)

#define qRgba2(r, g, b, a) ((a << 24) | (r << 16) | (g  << 8) | b)

/*
 * ASS_Image: 1bit alpha per pixel + 1 rgb per image. less memory usage
 */
//0xAARRGGBB
#if (Q_BYTE_ORDER == Q_BIG_ENDIAN)
#define ARGB32_SET(C, R, G, B, A) \
    C[0] = (A); \
    C[1] = (R); \
    C[2] = (G); \
    C[3] = (B);
#define ARGB32_ADD(C, R, G, B, A) \
    C[0] += (A); \
    C[1] += (R); \
    C[2] += (G); \
    C[3] += (B);
#define ARGB32_A(C) (C[0])
#define ARGB32_R(C) (C[1])
#define ARGB32_G(C) (C[2])
#define ARGB32_B(C) (C[3])
#else
#define ARGB32_SET(C, R, G, B, A) \
    C[0] = (B); \
    C[1] = (G); \
    C[2] = (R); \
    C[3] = (A);
#define ARGB32_ADD(C, R, G, B, A) \
    C[0] += (B); \
    C[1] += (G); \
    C[2] += (R); \
    C[3] += (A);
#define ARGB32_A(C) (C[3])
#define ARGB32_R(C) (C[2])
#define ARGB32_G(C) (C[1])
#define ARGB32_B(C) (C[0])
#endif
#define USE_QRGBA 0
// C[i] = C'[i] = (k*c[i]+(255-k)*C[i])/255 = C[i] + k*(c[i]-C[i])/255, min(c[i],C[i]) <= C'[i] <= max(c[i],C[i])
void SubtitleProcessorLibASS::renderASS32(QImage *image, ASS_Image *img, int dstX, int dstY)
{
    const quint8 a = 255 - _a(img->color);
    if (a == 0)
        return;
    const quint8 r = _r(img->color);
    const quint8 g = _g(img->color);
    const quint8 b = _b(img->color);
    quint8 *src = img->bitmap;
    // use QRgb to avoid endian issue
    QRgb *dst = (QRgb*)image->constBits() + dstY * image->width() + dstX;
    for (int y = 0; y < img->h; ++y) {
        for (int x = 0; x < img->w; ++x) {
            const unsigned k = ((unsigned) src[x])*a/255;
#if USE_QRGBA
            const unsigned A = qAlpha(dst[x]);
#else
            quint8 *c = (quint8*)(&dst[x]);
            const unsigned A = ARGB32_A(c);
#endif
            if (A == 0) { // dst color can be ignored
#if USE_QRGBA
                 dst[x] = qRgba(r, g, b, k);
#else
                 ARGB32_SET(c, r, g, b, k);
#endif //USE_QRGBA
            } else if (k == 0) { //no change
                //dst[x] = qRgba(qRed(dst[x])), qGreen(dst[x]), qBlue(dst[x]), qAlpha(dst[x])) == dst[x];
            } else if (k == 255) {
#if USE_QRGBA
                dst[x] = qRgba(r, g, b, k);
#else
                ARGB32_SET(c, r, g, b, k);
#endif //USE_QRGBA
            } else {
#if USE_QRGBA
                // no need to &0xff because always be 0~255
                dst[x] += qRgba2(k*(r-qRed(dst[x]))/255, k*(g-qGreen(dst[x]))/255, k*(b-qBlue(dst[x]))/255, k*(a-A)/255);
#else
                const unsigned R = ARGB32_R(c);
                const unsigned G = ARGB32_G(c);
                const unsigned B = ARGB32_B(c);
                ARGB32_ADD(c, r == R ? 0 : k*(r-R)/255, g == G ? 0 : k*(g-G)/255, b == B ? 0 : k*(b-B)/255, a == A ? 0 : k*(a-A)/255);
#endif
            }
        }
        src += img->stride;
        dst += image->width();
    }
}

} //namespace QtAV
