#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#else
#define highp
#define mediump
#define lowp
#endif

uniform sampler2D u_Texture0;
varying vec2 v_TexCoords;

void main() {
  gl_FragColor = texture2D(u_Texture0, v_TexCoords);
}
