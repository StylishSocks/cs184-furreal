#version 330

// (Every uniform is available here.)

uniform mat4 u_view_projection;
uniform mat4 u_model;

uniform float u_normal_scaling;
uniform float u_height_scaling;

uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

// Feel free to add your own textures. If you need more than 4,
// you will need to modify the skeleton.
uniform sampler2D u_texture_1;
uniform sampler2D u_texture_2;
uniform sampler2D u_texture_3;
uniform sampler2D u_texture_4;

// Environment map! Take a look at GLSL documentation to see how to
// sample from this.
uniform samplerCube u_texture_cubemap;

in vec4 v_position;
in vec4 v_normal;
in vec4 v_tangent;
in vec2 v_uv;

out vec4 out_color;

void main() {
  // Cel/Toon shader: non-photorealistic rendering
  vec3 n = normalize(v_normal.xyz);
  vec3 l = normalize(u_light_pos - v_position.xyz);
  vec3 v_dir = normalize(u_cam_pos - v_position.xyz);
  vec3 h_vec = normalize(l + v_dir);

  // 1. Quantized diffuse (3 discrete bands)
  float NdotL = dot(n, l);
  float intensity;
  if (NdotL > 0.6) {
    intensity = 1.0;          // bright band
  } else if (NdotL > 0.2) {
    intensity = 0.6;          // mid band
  } else if (NdotL > -0.1) {
    intensity = 0.35;         // shadow band
  } else {
    intensity = 0.15;         // deep shadow
  }

  // Base object color (warm off-white for cloth)
  vec3 base_color = vec3(0.85, 0.75, 0.65);
  vec3 diffuse = base_color * intensity;

  // 2. Specular highlight (hard cutof)
  float NdotH = dot(n, h_vec);
  float spec = 0.0;
  if (NdotH > 0.95) {
    spec = 1.0;
  }
  vec3 specular = vec3(1.0) * spec * 0.4;

  // 3. Rim/silhouette outline
  // Darken fragments where the normal is nearly perpendicular to the view
  float rim = dot(n, v_dir);
  float outline = 0.0;
  if (rim < 0.15) {
    outline = 1.0;  // edge pixel -> draw as dark outline
  }

  // Combine: if on the edge, draw black otherwise toon-shaded color
  vec3 final_color = mix(diffuse + specular, vec3(0.0), outline);

  out_color = vec4(final_color, 1.0);
}
