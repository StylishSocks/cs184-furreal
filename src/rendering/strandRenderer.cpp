#include "strandRenderer.h"

void StrandRenderer::renderLines(const std::vector<Strand> &strands,
                                 GLShader &shader) {
  int total_segments = 0;
  for (const Strand &s : strands) {
    total_segments += s.num_particles() - 1;
  }
  if (total_segments == 0) return;

  Eigen::MatrixXf positions(4, total_segments * 2);
  int idx = 0;

  for (const Strand &s : strands) {
    for (int i = 0; i < s.num_particles() - 1; i++) {
      const CGL::Vector3D &p1 = s.particles[i].position;
      const CGL::Vector3D &p2 = s.particles[i + 1].position;
      positions.col(idx)     << p1.x, p1.y, p1.z, 1.0;
      positions.col(idx + 1) << p2.x, p2.y, p2.z, 1.0;
      idx += 2;
    }
  }

  shader.uploadAttrib("in_position", positions, false);
  shader.drawArray(GL_LINES, 0, total_segments * 2);
}

void StrandRenderer::renderStrips(const std::vector<Strand> &strands,
                                  GLShader &shader,
                                  const CGL::Vector3D &camera_pos,
                                  float width) {
  // TODO: Implement triangle-strip rendering for Kajiya-Kay shading.
  // For each strand segment, generate a view-facing quad.
  // For now, fall back to line rendering.
  renderLines(strands, shader);
}
