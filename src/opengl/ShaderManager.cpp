#include "ShaderManager.h"
#include "QtAV/VideoShader.h"

namespace QtAV {
class ShaderManager::Private
{
public:
    ~Private() {
        // TODO: thread safe required?
        qDeleteAll(shader_cache.values());
        shader_cache.clear();
    }

    QHash<qint32, VideoShader*> shader_cache;
};

ShaderManager::ShaderManager(QObject *parent) :
    QObject(parent)
  , d(new Private())
{
}

ShaderManager::~ShaderManager()
{
    delete d;
    d = 0;
}

VideoShader* ShaderManager::prepareMaterial(VideoMaterial *material, qint32 materialType)
{
    const qint32 type = materialType != -1 ? materialType : material->type();
    VideoShader *shader = d->shader_cache.value(type, 0);
    if (shader)
        return shader;
    qDebug() << QString("[ShaderManager] cache a new shader material type(%1): %2").arg(type).arg(VideoMaterial::typeName(type));
    shader = material->createShader();
    shader->initialize();
    d->shader_cache[type] = shader;
    return shader;
}
} //namespace QtAV
