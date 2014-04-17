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
#if P010
    // yyyyyyyy yy000000 => (L, L, L, A)
    gl_FragColor = clamp(u_colorMatrix*yuv2rgbMatrix* vec4(
                             (texture2D(u_Texture0, v_TexCoords).r*4.0 + texture2D(u_Texture0, v_TexCoords).a/64.0)*255.0/1023.0,
                             (texture2D(u_Texture1, v_TexCoords).r*4.0 + texture2D(u_Texture1, v_TexCoords).a/64.0)*255.0/1023.0,
                             (texture2D(u_Texture2, v_TexCoords).r*4.0 + texture2D(u_Texture2, v_TexCoords).a/64.0)*255.0/1023.0,
                             1)
                         , 0.0, 1.0);
#else
    // use r, g, a to work for both yv12 and nv12
    gl_FragColor = clamp(u_colorMatrix*yuv2rgbMatrix* vec4(texture2D(u_Texture0, v_TexCoords).r,
                                           texture2D(u_Texture1, v_TexCoords).g,
                                           texture2D(u_Texture2, v_TexCoords).a,
                                           1)
                      , 0.0, 1.0);
#endif
}
