#version 450 core
layout (local_size_x = 10, local_size_y = 10) in;


// A buffer U is a buffer with points position along z in the
// membrain grid. There are need to store several positions
// per grid point along z in the buffer U for the task purpose.
// These positions are U[idx][t], U[idx][t - 1], U[idx][t - 2],
// when idx is an index of xy position in the grid membrain, t
// is a current time step. The buffer U stores these positions
// in the following way:
// - U[idx][c_out] is a value U[idx][t];
// - U[idx][(c_out + 1) % 3] is a value U[idx][t - 1];
// - U[idx][(c_out + 2) % 3] is a value U[idx][t - 2];
// when c_out is an uniform variable, which increments
// every time step in the main program
// In fact 4 values are stored in the buffer U per grid point.
// The 3 first values have been discussed above. The 4th value
// is a parameter, that defines different meanings of
// the corresponding grid point.
// If U[idx][3] == 0, XY[idx][2] is an attenuation coefficient,
// which is effect with wave propagation through grid point with
// index idx.
// If U[idx][3] == 1, a grid point with index idx is a fixed
// node with U[idx][(c_out + 1) % 3] as z position.
layout (std430, binding = 0) buffer U {
  vec4 u[];
};

// A buffer XY sotres x, y positions of each grid points.
// XY[idx][0] is an x position of grid point.
// XY[idx][1] is an y position of grid point.
// Also XY[idx][2] is an attenuation coefficient, see above.
// XY[idx][3] is unused.
layout (std430, binding = 1) buffer XY {
  vec4 xy[];
};


// Grid size:
// sz[0] is a size along x axis
// sz[1] is a size along y axis
uniform ivec2 sz;

// Meaning of c_out is described above
uniform int c_out;

uniform float dx; // Grid step along x axis
uniform float dy; // Grid step along y axis
uniform float dt; // Time step duration
uniform float v;  // Phase velocity
uniform float dx2; // dx*dx
uniform float dy2; // dy*dy
uniform float dt2; // dt*dt
uniform float v2;  // v*v


// Function returns index of grid point by x and y
// indices
uint getIdx(uint idx_x, uint idx_y) {
  return idx_y*sz.x + idx_x;
}


void main() {
  // Get x and y indices of a grid point
  // Indices of a grid point is the same
  // as invocation id in the compute space
  uint idx_x = gl_GlobalInvocationID.x;
  uint idx_y = gl_GlobalInvocationID.y;

  // Indices of U[idx][t - 1] and U[idx][t - 2]
  int l_idx = (c_out + 1) % 3;
  int ll_idx = (c_out + 2) % 3;

  // A grid point index in U and XY buffers
  uint xy_idx = getIdx(idx_x, idx_y);

  // A parameter which defines a role of a grid point
  float param = u[xy_idx][3];
  if (param == 1) {
    u[xy_idx][c_out] = u[xy_idx][l_idx];
    return;
  }

  // There is used next symbol below.
  // A dnm sybmbol, when n and m are equals 0 or 1
  // Expression (idx +- dxx) means index
  // corresponding grid point index by (x +- n)
  // and (y +- m) indices along x and y axises.
  // For example, (idx - d01) corresponding
  // with point by (x, y - 1) indices

  float um10m1 = 0; // U[idx - d10][t - 1]
  float up10m1 = 0; // U[idx + d10][t - 1]

  float u0m1m1 = 0; // U(idx - d01)[t - 1]
  float u0p1m1 = 0; // U(idx - d01)[t - 1]

  // A membrain boundries must be considered
  if (idx_x > 0)
    um10m1 = u[getIdx(idx_x - 1, idx_y)][l_idx];
  if (idx_x < sz.x - 1)
    up10m1 = u[getIdx(idx_x + 1, idx_y)][l_idx];

  if (idx_y > 0)
    u0m1m1 = u[getIdx(idx_x, idx_y - 1)][l_idx];
  if (idx_y < sz.y - 1)
    u0p1m1 = u[getIdx(idx_x, idx_y + 1)][l_idx];

  float u00m1 = u[xy_idx][l_idx];  // U[idx][t - 1]
  float u00m2 = u[xy_idx][ll_idx]; // U[idx][t - 2]

  // Second order differences by x and y axises
  float d2u_dn2 = (um10m1 - 2*u00m1 + up10m1)/dx2;
  float d2u_dm2 = (u0m1m1 - 2*u00m1 + u0p1m1)/dy2;
  float Dumn = d2u_dn2 + d2u_dm2; // Laplace operator result

  // U[idx][t] result
  u[xy_idx][c_out] = v2*dt2*Dumn + 2*u00m1 - u00m2;

  // An attenuation coefficient consideration, if U[idx][3] == 0
  // Attenuation coefficient affects first order difference
  if (param == 0) {
    float a = xy[xy_idx][2]*(u[xy_idx][c_out] - u[xy_idx][l_idx])/dt;
    u[xy_idx][c_out] -= a;
  }

}
