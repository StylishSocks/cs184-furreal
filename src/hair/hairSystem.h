#ifndef HAIR_SYSTEM_H
#define HAIR_SYSTEM_H

#include <vector>
#include <nanogui/nanogui.h>
#include "strand.h"
#include "../core/wind.h"
#include "../collision/collisionObject.h"

using namespace CGL;
using namespace std;
using namespace nanogui;

struct HairParameters {
  int num_strands = 1000;
  int particles_per_strand = 10;
  double strand_length = 0.3;
  double damping = 0.01;
  double mass_per_particle = 0.01;
  WindParams wind;
};

// Main hair/fur simulation system.
// Manages a collection of strands, runs the FTL solver,
// handles collision and external forces.
class HairSystem {
public:
  HairSystem() {}
  ~HairSystem() {}

  // Build strands attached to a surface or set of root positions.
  // For now, roots are generated on a flat patch; later, from a mesh.
  void buildStrands(const vector<Vector3D> &root_positions,
                    const vector<Vector3D> &root_normals,
                    HairParameters *params);

  // Run one simulation step.
  void simulate(double frames_per_sec, double simulation_steps,
                HairParameters *params,
                vector<Vector3D> external_accelerations,
                vector<CollisionObject *> *collision_objects);

  // Render all strands using the given shader.
  void render(GLShader &shader);

  // Reset all strands to initial positions.
  void reset();

  vector<Strand> strands;
  double sim_time = 0.0;
};

#endif /* HAIR_SYSTEM_H */
