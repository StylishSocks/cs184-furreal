#include "ftlSolver.h"

void solve_ftl(Strand &strand) {
  // Follow-The-Leader: single pass from root to tip.
  // For each consecutive pair (i, i+1), project particle i+1
  // so that it is exactly rest_length away from particle i.
  // Root (particle 0) is pinned and does not move.

  for (int i = 0; i < (int)strand.rest_lengths.size(); i++) {
    PointMass &parent = strand.particles[i];
    PointMass &child = strand.particles[i + 1];

    if (child.pinned) continue;

    Vector3D dir = child.position - parent.position;
    double dist = dir.norm();

    if (dist < 1e-12) continue;

    // Project child to be exactly rest_length from parent
    child.position = parent.position + dir.unit() * strand.rest_lengths[i];
  }
}
