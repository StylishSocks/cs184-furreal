#include "spatial_hash.h"
#include <cmath>

void SpatialHashGrid::clear() {
  for (const auto &entry : map_) {
    delete entry.second;
  }
  map_.clear();
}

void SpatialHashGrid::build(vector<PointMass> &point_masses,
                            double cell_w, double cell_h, double cell_t) {
  clear();
  w_ = cell_w;
  h_ = cell_h;
  t_ = cell_t;

  for (PointMass &pm : point_masses) {
    float hash = hash_position(pm.position);
    if (map_.find(hash) == map_.end()) {
      map_[hash] = new vector<PointMass *>();
    }
    map_[hash]->push_back(&pm);
  }
}

vector<PointMass *> SpatialHashGrid::query(const Vector3D &pos) {
  float hash = hash_position(pos);
  if (map_.find(hash) != map_.end()) {
    return *(map_[hash]);
  }
  return {};
}

void SpatialHashGrid::self_collide(PointMass &pm, double thickness,
                                   double simulation_steps) {
  float hash = hash_position(pm.position);
  Vector3D correction(0, 0, 0);
  int count = 0;

  if (map_.find(hash) != map_.end()) {
    for (PointMass *candidate : *(map_[hash])) {
      if (candidate == &pm) continue;

      Vector3D diff = pm.position - candidate->position;
      double dist = diff.norm();

      if (dist < 2.0 * thickness) {
        Vector3D corr = diff.unit() * (2.0 * thickness - dist);
        correction += corr;
        count++;
      }
    }
  }

  if (count > 0) {
    pm.position += correction / (double)count / simulation_steps;
  }
}

float SpatialHashGrid::hash_position(const Vector3D &pos) {
  double box_x = floor(pos.x / w_);
  double box_y = floor(pos.y / h_);
  double box_z = floor(pos.z / t_);

  return (float)(box_x * 73856093.0 + box_y * 19349663.0 + box_z * 83492791.0);
}
