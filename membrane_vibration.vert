#version 450 core

// Position of grid point along z axis
// See more in membrane_vibration.glsl shader
layout (location = 0) in vec4 u;
// Positions of grid point along x and y axises
layout (location = 1) in vec4 xy;

// Output point
out vec3 vr_pos;

uniform vec3 shift; // Membrain shift
uniform mat3 model; // Membrain rotation
uniform mat4 view;  // Camera position operator
uniform mat4 perspective; // Prespective operator

// Index of U[idx][t] in u variable
// See more in membrane_vibration.glsl shader
uniform int c_out;

void main() {
  vr_pos = vec3(xy.xy, u[c_out]);
  vec3 pos = model*vr_pos + shift;
  gl_Position = perspective*view*vec4(pos, 1.0);
}
