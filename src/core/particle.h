#ifndef PARTICLE_H
#define PARTICLE_H

#include "CGL/CGL.h"
#include "CGL/misc.h"
#include "CGL/vector3D.h"

using namespace CGL;

// Forward declaration for half-edge mesh connectivity (used by renderers)
class Halfedge;

struct PointMass {
  PointMass(Vector3D position, bool pinned)
      : pinned(pinned), start_position(position), position(position),
        last_position(position) {}

  Vector3D normal();
  Vector3D velocity(double delta_t) {
    return (position - last_position) / delta_t;
  }

  bool pinned;
  double mass = 1.0; // Per-particle mass (hair strands need equi-mass particles)
  Vector3D start_position;
  Vector3D position;
  Vector3D last_position;
  Vector3D forces;
  Halfedge *halfedge = nullptr;
};

#endif /* PARTICLE_H */
