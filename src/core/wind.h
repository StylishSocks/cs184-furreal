#ifndef WIND_H
#define WIND_H

#include "particle.h"
#include "CGL/vector3D.h"

using namespace CGL;

struct WindParams {
  bool enable = false;
  double strength = 20.0;
  Vector3D direction = Vector3D(1, 0, 0);
};

// Compute wind force on a particle using a local strand/surface axis.
// Uses a simple aerodynamic model with spatially and temporally varying gusts.
//
// normal: local axis at the particle (surface normal or strand tangent)
// mass: particle mass
// sim_time: current simulation time (for temporal variation)
Vector3D compute_wind_force(const Vector3D &position, const Vector3D &normal,
                            double mass, const WindParams &params,
                            double sim_time);

#endif /* WIND_H */
