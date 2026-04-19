#ifndef STRAND_RENDERER_H
#define STRAND_RENDERER_H

#include <vector>
#include <nanogui/nanogui.h>
#include "../hair/strand.h"

using namespace nanogui;

// Renders hair strands. Supports GL_LINES (simple) and
// triangle-strip modes for thicker strand visualization.
class StrandRenderer {
public:
  // Render strands as simple GL_LINES.
  void renderLines(const std::vector<Strand> &strands, GLShader &shader);

  // Render strands as triangle strips for Kajiya-Kay shading.
  // camera_pos needed to compute view-facing quads.
  void renderStrips(const std::vector<Strand> &strands, GLShader &shader,
                    const CGL::Vector3D &camera_pos, float width = 0.002f);
};

#endif /* STRAND_RENDERER_H */
