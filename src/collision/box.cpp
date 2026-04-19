#include <nanogui/nanogui.h>
#include <cmath>

#include "../core/particle.h"
#include "box.h"

using namespace nanogui;
using namespace CGL;

#define SURFACE_OFFSET 0.0001

void Box::collide(PointMass &pm) {
  // Check if point mass is inside the axis-aligned box

  Vector3D d = pm.position - center;

  bool inside = (fabs(d.x) < half_extents.x) &&
                (fabs(d.y) < half_extents.y) &&
                (fabs(d.z) < half_extents.z);

  if (inside) {
    // Penetration depth along each axis
    double pen_x = half_extents.x - fabs(d.x);
    double pen_y = half_extents.y - fabs(d.y);
    double pen_z = half_extents.z - fabs(d.z);

    // Find axis of minimum penetration for shortest escape
    Vector3D tangent_point = pm.position;

    if (pen_x <= pen_y && pen_x <= pen_z) {
      double sign = (d.x > 0) ? 1.0 : -1.0;
      tangent_point.x = center.x + sign * (half_extents.x + SURFACE_OFFSET);
    } else if (pen_y <= pen_x && pen_y <= pen_z) {
      double sign = (d.y > 0) ? 1.0 : -1.0;
      tangent_point.y = center.y + sign * (half_extents.y + SURFACE_OFFSET);
    } else {
      double sign = (d.z > 0) ? 1.0 : -1.0;
      tangent_point.z = center.z + sign * (half_extents.z + SURFACE_OFFSET);
    }

    // Apply friction-scaled correction from last_position
    Vector3D correction = tangent_point - pm.last_position;
    pm.position = pm.last_position + correction * (1.0 - friction);
  }
}

void Box::render(GLShader &shader) {
  // Render a slightly smaller box (like sphere does) to avoid z-fighting
  double s = 0.98;
  Vector3D hx(half_extents.x * s, 0, 0);
  Vector3D hy(0, half_extents.y * s, 0);
  Vector3D hz(0, 0, half_extents.z * s);

  // 8 corner vertices
  Vector3D c[8] = {
    center - hx - hy - hz, // 0: ---
    center + hx - hy - hz, // 1: +--
    center - hx + hy - hz, // 2: -+-
    center + hx + hy - hz, // 3: ++-
    center - hx - hy + hz, // 4: --+
    center + hx - hy + hz, // 5: +-+
    center - hx + hy + hz, // 6: -++
    center + hx + hy + hz, // 7: +++
  };

  // 6 faces: each defined as 4 corner indices (CCW winding from outside)
  // and outward-pointing normal
  int faces[6][4] = {
    {1, 3, 7, 5}, // +X face
    {0, 4, 6, 2}, // -X face
    {2, 6, 7, 3}, // +Y face
    {0, 1, 5, 4}, // -Y face
    {4, 5, 7, 6}, // +Z face
    {0, 2, 3, 1}, // -Z face
  };

  Vector3D face_normals[6] = {
    Vector3D( 1, 0, 0),
    Vector3D(-1, 0, 0),
    Vector3D( 0, 1, 0),
    Vector3D( 0,-1, 0),
    Vector3D( 0, 0, 1),
    Vector3D( 0, 0,-1),
  };

  // 6 faces × 2 triangles × 3 vertices = 36 vertices
  MatrixXf positions(3, 36);
  MatrixXf normals(3, 36);

  int vi = 0;
  for (int f = 0; f < 6; f++) {
    Vector3D n = face_normals[f];

    // Triangle 1: v0, v1, v2
    positions.col(vi) << c[faces[f][0]].x, c[faces[f][0]].y, c[faces[f][0]].z;
    normals.col(vi) << n.x, n.y, n.z; vi++;
    positions.col(vi) << c[faces[f][1]].x, c[faces[f][1]].y, c[faces[f][1]].z;
    normals.col(vi) << n.x, n.y, n.z; vi++;
    positions.col(vi) << c[faces[f][2]].x, c[faces[f][2]].y, c[faces[f][2]].z;
    normals.col(vi) << n.x, n.y, n.z; vi++;

    // Triangle 2: v0, v2, v3
    positions.col(vi) << c[faces[f][0]].x, c[faces[f][0]].y, c[faces[f][0]].z;
    normals.col(vi) << n.x, n.y, n.z; vi++;
    positions.col(vi) << c[faces[f][2]].x, c[faces[f][2]].y, c[faces[f][2]].z;
    normals.col(vi) << n.x, n.y, n.z; vi++;
    positions.col(vi) << c[faces[f][3]].x, c[faces[f][3]].y, c[faces[f][3]].z;
    normals.col(vi) << n.x, n.y, n.z; vi++;
  }

  nanogui::Color color(0.6f, 0.4f, 0.2f, 1.0f);

  if (shader.uniform("u_color", false) != -1) {
    shader.setUniform("u_color", color);
  }
  shader.uploadAttrib("in_position", positions);
  if (shader.attrib("in_normal", false) != -1) {
    shader.uploadAttrib("in_normal", normals);
  }

  shader.drawArray(GL_TRIANGLES, 0, 36);
}
