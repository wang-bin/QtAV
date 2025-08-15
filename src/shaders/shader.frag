#version 450
#extension GL_ARB_separate_shader_objects : enable

// 绑定Y、U、V三个平面的纹理采样器
layout(binding = 1) uniform sampler2D texY;
layout(binding = 2) uniform sampler2D texU;
layout(binding = 3) uniform sampler2D texV;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 posTransform;
    mat4 colorTransform;
} ubo;

void main() {
    // 采样Y、U、V分量（R通道存储单分量数据）
    float y = texture(texY, fragTexCoord).r;
    float u = texture(texU, fragTexCoord).r;
    float v = texture(texV, fragTexCoord).r;
    
    // YUV到RGB的转换（BT.709标准，适合高清视频）
    // 首先将分量从[0,1]范围转换到实际范围
    y = y * 1.16438356 - 0.0627451017;  // 转换Y范围 [16/255, 235/255] -> [0, 1]
    u = u - 0.5;                         // 转换U范围 [16/255, 240/255] -> [-0.5, 0.5]
    v = v - 0.5;                         // 转换V范围 [16/255, 240/255] -> [-0.5, 0.5]
    
    // 应用转换公式
    float r = y + 1.79274107 * v;
    float g = y - 0.213248614 * u - 0.532909328 * v;
    float b = y + 2.11240179 * u;
    
    vec4 transformedColor = ubo.colorTransform * vec4(r, g, b, 1.0);
    
    transformedColor.rgb = clamp(transformedColor.rgb, 0.0, 1.0);
    
    outColor = vec4(transformedColor.rgb, 1.0);
}
