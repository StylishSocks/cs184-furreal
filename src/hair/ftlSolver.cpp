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

void solve_ftl_with_corrections(Strand &strand,
                                std::vector<CGL::Vector3D> &corrections) {
  const int n = strand.num_particles();
  corrections.clear();
  corrections.resize(n, CGL::Vector3D(0, 0, 0));
  if (n < 2) return;

  for (int i = 0; i < n - 1; i++) {
    PointMass &parent = strand.particles[i];
    PointMass &child = strand.particles[i + 1];

    if (child.pinned) continue;

    CGL::Vector3D dir = child.position - parent.position;
    double dist = dir.norm();
    if (dist < 1e-12) continue;

    CGL::Vector3D projected = parent.position + dir.unit() * strand.rest_lengths[i];
    corrections[i + 1] = projected - child.position;
    child.position = projected;
  }
}
