namespace QtAV
{

class OpenGLVideo : public QObject
{
%TypeHeaderCode
#include <QtAV/OpenGLVideo.h>
%End

public:
    enum MeshType {
        RectMesh,
        SphereMesh
    };

    static bool isSupported(VideoFormat::PixelFormat pixfmt);
    OpenGLVideo();
    void setOpenGLContext(QOpenGLContext *ctx);
    QOpenGLContext* openGLContext();
    void setCurrentFrame(const VideoFrame& frame);
    void fill(const QColor& color);
    void render(const QRectF& target = QRectF(), const QRectF& roi = QRectF(), const QMatrix4x4& transform = QMatrix4x4());
    void setProjectionMatrixToRect(const QRectF& v);
    void setViewport(const QRectF& r);

    void setBrightness(qreal value);
    void setContrast(qreal value);
    void setHue(qreal value);
    void setSaturation(qreal value);

    // TODO: Add after we expose VideoShader
    //void setUserShader(VideoShader* shader);
    //VideoShader* userShader() const;

    void setMeshType(MeshType value);
    MeshType meshType() const;
signals:
    void beforeRendering();
    void afterRendering();
};

};
