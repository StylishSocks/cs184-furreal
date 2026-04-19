#include "particle.h"
#include "meshTypes.h"

Vector3D PointMass::normal() {
  // For standalone particles (not in a half-edge mesh),
  // return a default up vector. When used in a mesh context,
  // this should be overridden or computed from the mesh topology.
  if (halfedge) {
    // Half-edge mesh normal computation (from HW4 clothMesh.cpp)
    Vector3D n(0, 0, 0);
    Halfedge *start = halfedge;
    Halfedge *iter = start;

    do {
      n = n + cross(iter->next->pm->position - position,
                     iter->next->next->pm->position - position);
      if (iter->next->next->twin) {
        iter = iter->next->next->twin;
      } else {
        break;
      }
    } while (iter != start);

    if (iter != start) {
      start = halfedge;
      iter = start;
      if (iter->twin) {
        do {
          n = n + cross(iter->twin->next->next->pm->position - position,
                         iter->twin->pm->position - position);
          if (iter->twin->next->twin) {
            iter = iter->twin->next;
          } else {
            break;
          }
        } while (iter != start);
      }
    }

    return n.unit();
  }

  // Default: no mesh connectivity, return up vector
  return Vector3D(0, 1, 0);
}
