#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include "particle.h"
#include "CGL/vector3D.h"

using namespace CGL;

// Verlet integration step for a single particle.
// Extracted from HW4 cloth.cpp simulate() method.
//
// delta_t: timestep
// damping: damping factor in [0, 1] (typically cp->damping / 100.0)
void verlet_step(PointMass &pm, double delta_t, double damping);

#endif /* INTEGRATOR_H */
