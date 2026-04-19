#version 330

uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

uniform vec4 u_color;

uniform sampler2D u_texture_2;
uniform vec2 u_texture_2_size;

uniform float u_normal_scaling;
uniform float u_height_scaling;

in vec4 v_position;
in vec4 v_normal;
in vec4 v_tangent;
in vec2 v_uv;

out vec4 out_color;

float h(vec2 uv) {
  return texture(u_texture_2, uv).r;
}

void main() {
  // Bump mapping: compute displaced normal using height map
  vec3 n = normalize(v_normal.xyz);
  vec3 t = normalize(v_tangent.xyz);
  vec3 b = cross(n, t);
  mat3 TBN = mat3(t, b, n);

  float w = u_texture_2_size.x;
  float he = u_texture_2_size.y;
  float kh = u_height_scaling;
  float kn = u_normal_scaling;

  float dU = (h(vec2(v_uv.x + 1.0 / w, v_uv.y)) - h(v_uv)) * kh * kn;
  float dV = (h(vec2(v_uv.x, v_uv.y + 1.0 / he)) - h(v_uv)) * kh * kn;

  vec3 n_o = vec3(-dU, -dV, 1.0);
  vec3 n_d = TBN * n_o;
  n_d = normalize(n_d);

  // Blinn-Phong with displaced normal
  vec3 l = u_light_pos - v_position.xyz;
  float r2 = dot(l, l);
  l = normalize(l);
  vec3 v_dir = normalize(u_cam_pos - v_position.xyz);
  vec3 h_vec = normalize(l + v_dir);

  float k_a = 0.1;
  float k_d = 1.0;
  float k_s = 0.5;
  vec3 I_a = vec3(1.0);
  float p = 100.0;

  vec3 ambient = k_a * I_a;
  vec3 diffuse = k_d * (u_light_intensity / r2) * max(0.0, dot(n_d, l));
  vec3 specular = k_s * (u_light_intensity / r2) * pow(max(0.0, dot(n_d, h_vec)), p);

  out_color = vec4(ambient + diffuse + specular, 1.0);
}

