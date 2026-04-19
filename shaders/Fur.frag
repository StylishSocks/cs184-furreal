#version 330

uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

uniform vec3 u_root_color;
uniform vec3 u_tip_color;
uniform float u_alpha;
uniform float u_primary_spec_power;
uniform float u_secondary_spec_power;

in vec4 v_position;
in vec4 v_normal;
in vec4 v_tangent; // .w stores optional per-strand color variation
in vec2 v_uv;

out vec4 out_color;

void main() {
  vec3 N = normalize(v_normal.xyz);
  vec3 T = normalize(v_tangent.xyz);
  vec3 L = normalize(u_light_pos - v_position.xyz);
  vec3 V = normalize(u_cam_pos - v_position.xyz);
  vec3 H = normalize(L + V);

  float ndotl = max(0.0, dot(N, L));
  float th = dot(T, H);
  float sin_th = sqrt(max(0.0, 1.0 - th * th));

  float spec1 = pow(sin_th, u_primary_spec_power);
  float spec2 = pow(sin_th, u_secondary_spec_power) * 0.35;
  float anisotropic = (spec1 + spec2) * ndotl;

  vec3 base = mix(u_root_color, u_tip_color, v_uv.y);
  base *= (0.70 + 0.30 * ndotl);
  base *= (1.0 + v_tangent.w * 0.25);

  // Slightly fade thin outer ribbon edges to avoid hard strips.
  float edge = abs(v_uv.x * 2.0 - 1.0);
  float alpha = u_alpha * (1.0 - 0.35 * edge);

  vec3 lit = base + anisotropic * u_light_intensity * 0.05;
  out_color = vec4(lit, clamp(alpha, 0.0, 1.0));
}
