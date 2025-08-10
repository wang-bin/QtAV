#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 posTransform;
    mat4 colorTransform;
} ubo;

void main() {
    gl_Position = ubo.posTransform * vec4(inPosition, 0.0, 1.0);
    fragTexCoord = inTexCoord;
}
