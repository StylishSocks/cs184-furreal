#include "integrator.h"

void verlet_step(PointMass &pm, double delta_t, double damping) {
  if (pm.pinned) return;

  Vector3D accel = pm.forces / pm.mass;
  Vector3D new_pos = pm.position
                   + (1.0 - damping) * (pm.position - pm.last_position)
                   + accel * delta_t * delta_t;

  pm.last_position = pm.position;
  pm.position = new_pos;
}
