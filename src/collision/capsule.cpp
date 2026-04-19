#include "capsule.h"

void Capsule::render(GLShader &shader) {
  // TODO: Render capsule as two spheres + cylinder
  // For now, stub — will be implemented when character model collision is added
}

void Capsule::collide(PointMass &pm) {
  // Capsule collision: find closest point on line segment (a, b) to pm.position,
  // then treat as sphere collision with that closest point and the capsule radius.

  Vector3D ab = endpoint_b - endpoint_a;
  double ab_len_sq = dot(ab, ab);

  if (ab_len_sq < 1e-12) {
    // Degenerate capsule: treat as sphere at endpoint_a
    Vector3D diff = pm.position - endpoint_a;
    double dist = diff.norm();
    if (dist < radius) {
      Vector3D correction = diff.unit() * (radius - dist);
      pm.position += correction * (1.0 - friction);
    }
    return;
  }

  // Project pm onto the capsule axis
  double t = dot(pm.position - endpoint_a, ab) / ab_len_sq;
  t = max(0.0, min(1.0, t));
  Vector3D closest = endpoint_a + t * ab;

  Vector3D diff = pm.position - closest;
  double dist = diff.norm();

  if (dist < radius) {
    Vector3D correction = diff.unit() * (radius - dist);
    pm.position += correction * (1.0 - friction);
  }
}
