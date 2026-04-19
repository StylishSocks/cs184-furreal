#ifndef BVH_H
#define BVH_H

#include <vector>
#include <algorithm>
#include "particle.h"
#include "CGL/vector3D.h"

using namespace CGL;
using namespace std;

// Axis-Aligned Bounding Box
struct AABB {
  Vector3D min_pt;
  Vector3D max_pt;

  void expand(const Vector3D &pt) {
    min_pt.x = min(min_pt.x, pt.x);
    min_pt.y = min(min_pt.y, pt.y);
    min_pt.z = min(min_pt.z, pt.z);
    max_pt.x = max(max_pt.x, pt.x);
    max_pt.y = max(max_pt.y, pt.y);
    max_pt.z = max(max_pt.z, pt.z);
  }

  bool intersects_sphere(const Vector3D &center, double radius) const {
    double dist_sq = 0.0;
    for (int i = 0; i < 3; i++) {
      double v = center[i];
      if (v < min_pt[i]) dist_sq += (min_pt[i] - v) * (min_pt[i] - v);
      if (v > max_pt[i]) dist_sq += (v - max_pt[i]) * (v - max_pt[i]);
    }
    return dist_sq <= radius * radius;
  }
};

struct BVHNode {
  AABB bbox;
  int left = -1;
  int right = -1;
  int pm_start = 0;
  int pm_count = 0;
  bool is_leaf() const { return left == -1 && right == -1; }
};

// BVH acceleration structure for spatial queries on point masses.
// Extracted and generalized from HW4 cloth.cpp extra credit.
class BVHAccel {
public:
  // Build BVH from a vector of point masses.
  void build(vector<PointMass> &point_masses);

  // Query all point masses within radius of pos.
  void query(const Vector3D &pos, double radius, vector<PointMass *> &results);

  // Self-collision correction using the BVH.
  void self_collide(PointMass &pm, double thickness, double simulation_steps);

private:
  int build_recursive(int start, int count, int depth);
  void query_recursive(int node_idx, const Vector3D &pos, double radius,
                       vector<PointMass *> &results);

  vector<BVHNode> nodes_;
  vector<PointMass *> ordered_pms_;
};

#endif /* BVH_H */
