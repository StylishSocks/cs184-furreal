#ifndef MESH_TYPES_H
#define MESH_TYPES_H

#include "particle.h"
#include "CGL/vector3D.h"

using namespace CGL;

// Minimal half-edge mesh types for normal computation.
// Ported from HW4 clothMesh.h. Used when particles are part of a mesh.

class Triangle;
class Edge;

class Halfedge {
public:
  Edge *edge = nullptr;
  Halfedge *next = nullptr;
  Halfedge *twin = nullptr;
  Triangle *triangle = nullptr;
  PointMass *pm = nullptr;
};

class Triangle {
public:
  Triangle(PointMass *pm1, PointMass *pm2, PointMass *pm3,
           Vector3D uv1, Vector3D uv2, Vector3D uv3)
      : pm1(pm1), pm2(pm2), pm3(pm3), uv1(uv1), uv2(uv2), uv3(uv3) {}

  PointMass *pm1, *pm2, *pm3;
  Vector3D uv1, uv2, uv3;
  Halfedge *halfedge = nullptr;
};

class Edge {
public:
  Halfedge *halfedge = nullptr;
};

#endif /* MESH_TYPES_H */
