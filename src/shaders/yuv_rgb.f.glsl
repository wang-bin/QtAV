#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#else
#define highp
#define mediump
#define lowp
#endif

// u_TextureN: yuv. use array?
uniform sampler2D u_Texture0;
uniform sampler2D u_Texture1;
uniform sampler2D u_Texture2;
varying lowp vec2 v_TexCoords;
uniform mat4 u_colorMatrix;

//http://en.wikipedia.org/wiki/YUV calculation used
//http://www.fourcc.org/fccyvrgb.php
//GLSL: col first
// use bt601
const mat4 yuv2rgbMatrix = mat4(1, 1, 1, 0,
                              0, -0.344, 1.773, 0,
                              1.403, -0.714, 0, 0,
                              0, 0, 0, 1)
                        * mat4(1, 0, 0, 0,
                               0, 1, 0, 0,
                               0, 0, 1, 0,
                               0, -0.5, -0.5, 1);


// 10, 16bit: http://msdn.microsoft.com/en-us/library/windows/desktop/bb970578%28v=vs.85%29.aspx
void main()
{
#if defined(YUV16BITS_LE_LUMINANCE_ALPHA) || defined(YUV16BITS_BE_LUMINANCE_ALPHA)
    // FFmpeg supports 9, 10, 12, 14, 16 bits
#if defined(YUV9P)
    const float range = 511.0;
#elif defined(YUV10P)
    const float range = 1023.0;
#elif defined(YUV12P)
    const float range = 4095.0;
#elif defined(YUV14P)
    const float range = 16383.0;
#elif defined(YUV16P)
    const float range = 65535.0;
#endif
#endif //defined(YUV16BITS_LE_LUMINANCE_ALPHA)
    // 10p in little endian: yyyyyyyy yy000000 => (L, L, L, A)
    gl_FragColor = clamp(u_colorMatrix*yuv2rgbMatrix* vec4(
#if defined(YUV16BITS_LE_LUMINANCE_ALPHA)
                             (texture2D(u_Texture0, v_TexCoords).r + texture2D(u_Texture0, v_TexCoords).a*256.0)*255.0/range,
                             (texture2D(u_Texture1, v_TexCoords).r + texture2D(u_Texture1, v_TexCoords).a*256.0)*255.0/range,
                             (texture2D(u_Texture2, v_TexCoords).r + texture2D(u_Texture2, v_TexCoords).a*256.0)*255.0/range,
#elif defined(YUV16BITS_BE_LUMINANCE_ALPHA)
                             (texture2D(u_Texture0, v_TexCoords).r*256.0 + texture2D(u_Texture0, v_TexCoords).a)*255.0/range,
                             (texture2D(u_Texture1, v_TexCoords).r*256.0 + texture2D(u_Texture1, v_TexCoords).a)*255.0/range,
                             (texture2D(u_Texture2, v_TexCoords).r*256.0 + texture2D(u_Texture2, v_TexCoords).a)*255.0/range,
#else
                             texture2D(u_Texture0, v_TexCoords).r,
                             texture2D(u_Texture1, v_TexCoords).g,
                             texture2D(u_Texture2, v_TexCoords).a,
#endif //defined(YUV16BITS_LE_LUMINANCE_ALPHA)
                             1)
                         , 0.0, 1.0);
}
