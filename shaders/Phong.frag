#version 330

uniform vec4 u_color;
uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

in vec4 v_position;
in vec4 v_normal;
in vec2 v_uv;

out vec4 out_color;

void main() {
  // Blinn-Phong shading: L = k_a * I_a + k_d * (I/r^2) * max(0, n.l) + k_s * (I/r^2) * max(0, n.h)^p
  vec3 n = normalize(v_normal.xyz);
  vec3 l = u_light_pos - v_position.xyz;
  float r2 = dot(l, l);
  l = normalize(l);
  vec3 v = normalize(u_cam_pos - v_position.xyz);
  vec3 h = normalize(l + v);

  float k_a = 0.1;
  float k_d = 1.0;
  float k_s = 0.5;
  vec3 I_a = vec3(1.0);
  float p = 100.0;

  vec3 ambient = k_a * I_a;
  vec3 diffuse = k_d * (u_light_intensity / r2) * max(0.0, dot(n, l));
  vec3 specular = k_s * (u_light_intensity / r2) * pow(max(0.0, dot(n, h)), p);

  out_color = vec4(ambient + diffuse + specular, 1.0);
}

