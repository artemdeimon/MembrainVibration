#version 450 core

// These variables are described in membrane_vibration.gsh
in vec3 g_pos;
in vec3 normal;
in vec3 gr_pos;

// Output frgment
layout (location = 0) out vec4 frag;

// Light description structure
struct Light {
  vec3 pos; // Posiiton of light point
  vec3 la;  // Ambient color
  vec3 ld;  // Diffusion color
  vec3 lr;  // Reflection color
};

// Material description structure
struct Material {
  vec3 ma; // Ambient color
  vec3 md; // Diffusion color
  vec3 mr; // Reflection color
  float shininess; // Shininess power of reflect color
};

uniform Light light;        // Light uniform variable
uniform Material material;  // Material of mebrain uniform variable

uniform mat4 view; // Camera position operator

// Z scale of color for height map
uniform float z_scale = 1.0;

// Subrountine function for shade model
subroutine vec4 shadeModelType();
// An uniform variable defines a shading model
// There are two models:
// - ADSModel is statndart shade model with ambient,
//   diffusion and specular reflection colors
// - heightShadeMode is heigh map shade model
subroutine uniform shadeModelType shadeModel;

void main() {
//  if shadeModelType -> ADSModel then
//    if (gl_FrontFacing) frag = ADSModel(g_pos, normal);
//    else frag = ADSModel(g_pos, -normal);
//  if shadeModelType -> heightShadeModem then
//    frag = heightColorMode(gr_pos);
  frag = shadeModel();
}

vec4 ADSshade(vec3 p, vec3 n) {
  // Vector from fragment to light point
  vec3 s = normalize(p - light.pos);

  // Diffusion component of color
  float sn = dot(s, n);

  // Calculation of reflection color
  float rv = 0;
  if (sn > 0) {
    vec3 v = normalize(vec3(view*vec4(p, 0.0)));
    vec3 r = reflect(-s, n);
    rv = max(dot(v, r), 0);
  }

  vec3 a = light.la*material.ma;
  vec3 d = light.ld*material.md*max(sn, 0.0);
  vec3 r = light.lr*material.mr*pow(rv, material.shininess);
  return vec4(a + d + r, 1.0);
}

subroutine(shadeModelType) vec4 ADSModel() {
  if (gl_FrontFacing) return ADSshade(g_pos, normal);
  else return ADSshade(g_pos, -normal);
}

subroutine(shadeModelType) vec4 heightShadeMode() {
  vec3 high_c = vec3(1.0, 0.0, 0.0);
  vec3 low_c = vec3(0.0, 0.0, 1.0);

  // Height map color is mix red and blue colors
  // defined by z position of fragment
  vec3 r_c = mix(low_c, high_c, gr_pos.z*z_scale);
  return vec4(r_c, 1.0);
}


