attribute vec4 a_Position;
attribute vec2 a_TexCoords0;
uniform mat4 u_MVP_matrix;
varying vec2 v_TexCoords0;

void main() {
  gl_Position = u_MVP_matrix * a_Position;
  v_TexCoords0 = a_TexCoords0;
}
