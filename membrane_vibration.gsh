#version 450 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

// Triangle vertices position
in vec3 vr_pos[];

out vec3 g_pos;  // Position of vertices relative camera
out vec3 normal; // Calculated normal of input triangle
out vec3 gr_pos; // Original position of vertices

void main() {
  vec3 p1 = gl_in[0].gl_Position.xyz;
  vec3 p2 = gl_in[1].gl_Position.xyz;
  vec3 p3 = gl_in[2].gl_Position.xyz;

  vec3 v1 = p2 - p1;
  vec3 v2 = p3 - p1;

  // Calculation of normal of input triangle
  normal = normalize(vec3(v1.y*v2.z - v2.y*v1.z,
                          v2.x*v1.z - v1.x*v2.z,
                          v1.x*v2.y - v2.x*v1.y));

  gl_Position = gl_in[0].gl_Position;
  g_pos = p1;
  gr_pos = vr_pos[0];
  EmitVertex();

  gl_Position = gl_in[1].gl_Position;
  g_pos = p2;
  gr_pos = vr_pos[1];
  EmitVertex();

  gl_Position = gl_in[2].gl_Position;
  g_pos = p3;
  gr_pos = vr_pos[2];
  EmitVertex();

  EndPrimitive();
}
