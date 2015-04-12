attribute vec4 a_Position;
attribute vec2 a_TexCoords0;
attribute vec2 a_TexCoords1;
attribute vec2 a_TexCoords2;
uniform mat4 u_MVP_matrix;
varying vec2 v_TexCoords0;
varying vec2 v_TexCoords1;
varying vec2 v_TexCoords2;
#ifdef PLANE_4
attribute vec2 a_TexCoords3;
varying vec2 v_TexCoords3;
#endif
void main() {
  gl_Position = u_MVP_matrix * a_Position;
  v_TexCoords0 = a_TexCoords0;
  v_TexCoords1 = a_TexCoords1;
  v_TexCoords2 = a_TexCoords2;
#ifdef PLANE_4
  v_TexCoords3 = a_TexCoords3;
#endif
}
