#ifndef SPATIAL_HASH_H
#define SPATIAL_HASH_H

#include <unordered_map>
#include <vector>
#include "particle.h"
#include "CGL/vector3D.h"

using namespace CGL;
using namespace std;

// Generalized spatial hash grid for neighbor queries on point masses.
// Extracted and generalized from HW4 cloth.cpp.
class SpatialHashGrid {
public:
  // Build the hash grid from a set of point masses.
  // cell_w, cell_h, cell_t define the 3D cell dimensions.
  void build(vector<PointMass> &point_masses,
             double cell_w, double cell_h, double cell_t);

  // Query neighbors within the same hash cell as pos.
  vector<PointMass *> query(const Vector3D &pos);

  // Perform self-collision correction for a single point mass.
  // thickness: minimum separation distance (2*thickness is used as threshold).
  // simulation_steps: used to scale the correction.
  void self_collide(PointMass &pm, double thickness, double simulation_steps);

private:
  float hash_position(const Vector3D &pos);

  double w_, h_, t_;
  unordered_map<float, vector<PointMass *> *> map_;

  void clear();
};

#endif /* SPATIAL_HASH_H */
