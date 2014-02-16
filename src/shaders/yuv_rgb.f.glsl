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

//http://en.wikipedia.org/wiki/YUV calculation used
//http://www.fourcc.org/fccyvrgb.php
//GLSL: col first
// use bt601
const mat4 colorMatrix = mat4(1, 1, 1, 0,
                              0, -0.344, 1.773, 0,
                              1.403, -0.714, 0, 0,
                              0, 0, 0, 1)
                        * mat4(1, 0, 0, 0,
                               0, 1, 0, 0,
                               0, 0, 1, 0,
                               0, -0.5, -0.5, 1);
void main()
{
    gl_FragColor = clamp(colorMatrix* vec4(texture2D(u_Texture0, v_TexCoords).r,
                                           texture2D(u_Texture1, v_TexCoords).r,
                                           texture2D(u_Texture2, v_TexCoords).r,
                                           1)
                      , 0.0, 1.0);
}
