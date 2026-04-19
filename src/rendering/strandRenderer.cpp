#include "strandRenderer.h"
#include <algorithm>

namespace {

static inline CGL::Vector3D safe_unit(const CGL::Vector3D &v,
                                      const CGL::Vector3D &fallback) {
  double n = v.norm();
  if (n < 1e-12) return fallback;
  return v / n;
}

} // namespace

void StrandRenderer::renderLines(const std::vector<RenderableStrand> &strands,
                                 GLShader &shader) {
  int total_segments = 0;
  for (const RenderableStrand &r : strands) {
    if (!r.strand) continue;
    total_segments += std::max(0, r.strand->num_particles() - 1);
  }
  if (total_segments == 0) return;

  Eigen::MatrixXf positions(4, total_segments * 2);
  int idx = 0;

  for (const RenderableStrand &r : strands) {
    if (!r.strand) continue;
    const Strand &s = *r.strand;
    for (int i = 0; i < s.num_particles() - 1; i++) {
      const CGL::Vector3D &p1 = s.particles[i].position;
      const CGL::Vector3D &p2 = s.particles[i + 1].position;
      positions.col(idx) << (float)p1.x, (float)p1.y, (float)p1.z, 1.0f;
      positions.col(idx + 1) << (float)p2.x, (float)p2.y, (float)p2.z, 1.0f;
      idx += 2;
    }
  }

  shader.uploadAttrib("in_position", positions, false);
  shader.drawArray(GL_LINES, 0, total_segments * 2);
}

void StrandRenderer::renderRibbons(const std::vector<RenderableStrand> &strands,
                                   GLShader &shader,
                                   const CGL::Vector3D &camera_pos,
                                   float root_width, float tip_width) {
  int total_segments = 0;
  for (const RenderableStrand &r : strands) {
    if (!r.strand) continue;
    total_segments += std::max(0, r.strand->num_particles() - 1);
  }
  if (total_segments == 0) return;

  const int verts_per_segment = 6; // 2 triangles
  const int total_verts = total_segments * verts_per_segment;
  Eigen::MatrixXf positions(4, total_verts);
  Eigen::MatrixXf normals(4, total_verts);
  Eigen::MatrixXf tangents(4, total_verts);
  Eigen::MatrixXf uvs(2, total_verts);

  int vi = 0;
  for (const RenderableStrand &r : strands) {
    if (!r.strand) continue;
    const Strand &s = *r.strand;
    const int n = s.num_particles();
    if (n < 2) continue;

    for (int i = 0; i < n - 1; i++) {
      const CGL::Vector3D p0 = s.particles[i].position;
      const CGL::Vector3D p1 = s.particles[i + 1].position;
      CGL::Vector3D tangent = safe_unit(p1 - p0, CGL::Vector3D(0, 1, 0));

      const CGL::Vector3D mid = (p0 + p1) * 0.5;
      CGL::Vector3D view_dir = safe_unit(camera_pos - mid, CGL::Vector3D(0, 0, 1));
      CGL::Vector3D side = safe_unit(cross(tangent, view_dir), CGL::Vector3D(1, 0, 0));
      CGL::Vector3D normal = safe_unit(cross(side, tangent), CGL::Vector3D(0, 1, 0));

      float t0 = (float)i / (float)(n - 1);
      float t1 = (float)(i + 1) / (float)(n - 1);
      float w0 = (root_width + (tip_width - root_width) * t0) * r.width_scale;
      float w1 = (root_width + (tip_width - root_width) * t1) * r.width_scale;

      CGL::Vector3D a = p0 - side * w0;
      CGL::Vector3D b = p0 + side * w0;
      CGL::Vector3D c = p1 - side * w1;
      CGL::Vector3D d = p1 + side * w1;

      CGL::Vector3D verts[6] = {a, c, b, b, c, d};
      float uv_u[6] = {0, 0, 1, 1, 0, 1};
      float uv_v[6] = {t0, t1, t0, t0, t1, t1};

      for (int k = 0; k < 6; k++) {
        positions.col(vi) << (float)verts[k].x, (float)verts[k].y, (float)verts[k].z, 1.0f;
        normals.col(vi) << (float)normal.x, (float)normal.y, (float)normal.z, 0.0f;
        tangents.col(vi) << (float)tangent.x, (float)tangent.y, (float)tangent.z, r.color_variation;
        uvs.col(vi) << uv_u[k], uv_v[k];
        vi++;
      }
    }
  }

  if (vi == 0) return;
  if (vi != total_verts) {
    positions.conservativeResize(Eigen::NoChange, vi);
    normals.conservativeResize(Eigen::NoChange, vi);
    tangents.conservativeResize(Eigen::NoChange, vi);
    uvs.conservativeResize(Eigen::NoChange, vi);
  }

  shader.uploadAttrib("in_position", positions, false);
  shader.uploadAttrib("in_normal", normals, false);
  shader.uploadAttrib("in_tangent", tangents, false);
  shader.uploadAttrib("in_uv", uvs, false);
  shader.drawArray(GL_TRIANGLES, 0, vi);
}
