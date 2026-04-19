#include "strand.h"

void Strand::init(const Vector3D &root_pos, const Vector3D &direction,
                  double length, int num_segments, double mass_per_particle) {
  particles.clear();
  rest_lengths.clear();

  double seg_len = length / num_segments;
  Vector3D dir = direction.unit();

  for (int i = 0; i <= num_segments; i++) {
    Vector3D pos = root_pos + dir * seg_len * i;
    bool pinned = (i == 0); // Root is pinned
    PointMass pm(pos, pinned);
    pm.mass = mass_per_particle;
    particles.push_back(pm);
  }

  for (int i = 0; i < num_segments; i++) {
    rest_lengths.push_back(seg_len);
  }
}

void Strand::reset() {
  for (PointMass &pm : particles) {
    pm.position = pm.start_position;
    pm.last_position = pm.start_position;
    pm.forces = Vector3D(0, 0, 0);
  }
}
