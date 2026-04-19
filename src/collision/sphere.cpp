#include <nanogui/nanogui.h>

#include "../core/particle.h"
#include "../misc/sphere_drawing.h"
#include "sphere.h"

using namespace nanogui;
using namespace CGL;

void Sphere::collide(PointMass &pm) {
  // (Part 3): Handle collisions with spheres.

  // Check if point mass is inside or on the sphere
  Vector3D dir = pm.position - origin;
  double dist = dir.norm();

  if (dist <= radius) {
    // Compute tangent point on surface of sphere
    Vector3D tangent_point = origin + dir.unit() * radius;
    // Correction vector from last_position to tangent point
    Vector3D correction = tangent_point - pm.last_position;
    // Updated position: last_position + correction scaled by (1 - friction)
    pm.position = pm.last_position + correction * (1.0 - friction);
  }
}

void Sphere::render(GLShader &shader) {
  // We decrease the radius here so flat triangles don't behave strangely
  // and intersect with the sphere when rendered
  m_sphere_mesh.draw_sphere(shader, origin, radius * 0.92);
}
