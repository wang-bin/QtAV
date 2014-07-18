#include "QtAV/private/ShaderManager.h"
#include "QtAV/OpenGLVideo.h"

namespace QtAV {

QHash<MaterialType*, VideoShader*> ShaderManager::shader_cache;

ShaderManager::ShaderManager(QObject *parent) :
    QObject(parent)
{
}

ShaderManager::~ShaderManager()
{
    qDeleteAll(shader_cache);
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
