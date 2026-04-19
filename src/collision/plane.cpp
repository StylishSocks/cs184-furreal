#include "iostream"
#include <nanogui/nanogui.h>

#include "../core/particle.h"
#include "plane.h"
#include "plane.h"

using namespace std;
using namespace CGL;

#define SURFACE_OFFSET 0.0001

void Plane::collide(PointMass &pm) {
  // (Part 3): Handle collisions with planes.

  // Check if point mass crossed the plane this timestep
  double d_curr = dot(pm.position - point, normal);
  double d_last = dot(pm.last_position - point, normal);

  // If signs differ, the point crossed
  if (d_curr * d_last < 0.0) {
    // Project position onto the plane
    Vector3D tangent_point = pm.position - d_curr * normal;
    // Correction vector: from last_position to tangent point + small offset
    Vector3D correction = tangent_point - pm.last_position + SURFACE_OFFSET * normal * (d_last < 0.0 ? -1.0 : 1.0);
    // Updated position: last_position + correction scaled by (1 - friction)
    pm.position = pm.last_position + correction * (1.0 - friction);
  }
}

void Plane::render(GLShader &shader) {
  nanogui::Color color(0.7f, 0.7f, 0.7f, 1.0f);

  Vector3f sPoint(point.x, point.y, point.z);
  Vector3f sNormal(normal.x, normal.y, normal.z);
  Vector3f sParallel(normal.y - normal.z, normal.z - normal.x,
                     normal.x - normal.y);
  sParallel.normalize();
  Vector3f sCross = sNormal.cross(sParallel);

  MatrixXf positions(3, 4);
  MatrixXf normals(3, 4);

  positions.col(0) << sPoint + 2 * (sCross + sParallel);
  positions.col(1) << sPoint + 2 * (sCross - sParallel);
  positions.col(2) << sPoint + 2 * (-sCross + sParallel);
  positions.col(3) << sPoint + 2 * (-sCross - sParallel);

  normals.col(0) << sNormal;
  normals.col(1) << sNormal;
  normals.col(2) << sNormal;
  normals.col(3) << sNormal;

  if (shader.uniform("u_color", false) != -1) {
    shader.setUniform("u_color", color);
  }
  shader.uploadAttrib("in_position", positions);
  if (shader.attrib("in_normal", false) != -1) {
    shader.uploadAttrib("in_normal", normals);
  }

  shader.drawArray(GL_TRIANGLE_STRIP, 0, 4);
}
