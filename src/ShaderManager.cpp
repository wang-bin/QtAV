#include "ShaderManager.h"
#include "QtAV/VideoShader.h"

namespace QtAV {

ShaderManager::ShaderManager(QOpenGLContext *ctx) :
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QObject(ctx)
#else
    QObject(0)
#endif
  , m_ctx(ctx)
{
}

ShaderManager::~ShaderManager()
{
    invalidated();
}

VideoShader* ShaderManager::prepareMaterial(VideoMaterial *material)
{
    MaterialType* type = material->type();
    VideoShader *shader = shader_cache.value(type, 0);
    if (shader)
        return shader;
    shader = material->createShader();
    shader->initialize();
    shader_cache[type] = shader;
    return shader;
}

void ShaderManager::invalidated()
{
    // TODO: thread safe required?
    qDeleteAll(shader_cache.values());
    shader_cache.clear();
}

} //namespace QtAV
