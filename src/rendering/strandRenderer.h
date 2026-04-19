#ifndef STRAND_RENDERER_H
#define STRAND_RENDERER_H

#include <vector>
#include <nanogui/nanogui.h>

#include "../hair/strand.h"

using namespace nanogui;

struct RenderableStrand {
  const Strand *strand = nullptr;
  float width_scale = 1.0f;
  float color_variation = 0.0f;
};

// Renders hair strands. Supports a debug line mode and
// a ribbon mode for denser fur-like appearance.
class StrandRenderer {
public:
  void renderLines(const std::vector<RenderableStrand> &strands,
                   GLShader &shader);

  void renderRibbons(const std::vector<RenderableStrand> &strands,
                     GLShader &shader,
                     const CGL::Vector3D &camera_pos,
                     float root_width, float tip_width);
};

#endif /* STRAND_RENDERER_H */
