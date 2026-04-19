#include "bvh.h"

void BVHAccel::build(vector<PointMass> &point_masses) {
  nodes_.clear();
  ordered_pms_.clear();
  ordered_pms_.resize(point_masses.size());
  for (size_t i = 0; i < point_masses.size(); i++) {
    ordered_pms_[i] = &point_masses[i];
  }
  if (!point_masses.empty()) {
    build_recursive(0, (int)point_masses.size(), 0);
  }
}

int BVHAccel::build_recursive(int start, int count, int depth) {
  int node_idx = (int)nodes_.size();
  nodes_.push_back(BVHNode());

  nodes_[node_idx].pm_start = start;
  nodes_[node_idx].pm_count = count;

  Vector3D p0 = ordered_pms_[start]->position;
  nodes_[node_idx].bbox.min_pt = p0;
  nodes_[node_idx].bbox.max_pt = p0;
  for (int i = start + 1; i < start + count; i++) {
    nodes_[node_idx].bbox.expand(ordered_pms_[i]->position);
  }

  if (count <= 8) {
    return node_idx;
  }

  Vector3D extent = nodes_[node_idx].bbox.max_pt - nodes_[node_idx].bbox.min_pt;
  int axis;
  if (extent.x >= extent.y && extent.x >= extent.z) axis = 0;
  else if (extent.y >= extent.x && extent.y >= extent.z) axis = 1;
  else axis = 2;

  int mid = start + count / 2;
  nth_element(ordered_pms_.begin() + start,
              ordered_pms_.begin() + mid,
              ordered_pms_.begin() + start + count,
              [axis](PointMass *a, PointMass *b) {
                return a->position[axis] < b->position[axis];
              });

  int left_count = mid - start;
  int right_count = count - left_count;

  int left_idx = build_recursive(start, left_count, depth + 1);
  int right_idx = build_recursive(mid, right_count, depth + 1);

  nodes_[node_idx].left = left_idx;
  nodes_[node_idx].right = right_idx;

  return node_idx;
}

void BVHAccel::query(const Vector3D &pos, double radius,
                     vector<PointMass *> &results) {
  if (!nodes_.empty()) {
    query_recursive(0, pos, radius, results);
  }
}

void BVHAccel::query_recursive(int node_idx, const Vector3D &pos,
                                double radius, vector<PointMass *> &results) {
  const BVHNode &node = nodes_[node_idx];

  if (!node.bbox.intersects_sphere(pos, radius)) {
    return;
  }

  if (node.is_leaf()) {
    for (int i = node.pm_start; i < node.pm_start + node.pm_count; i++) {
      results.push_back(ordered_pms_[i]);
    }
    return;
  }

  if (node.left >= 0) query_recursive(node.left, pos, radius, results);
  if (node.right >= 0) query_recursive(node.right, pos, radius, results);
}

void BVHAccel::self_collide(PointMass &pm, double thickness,
                            double simulation_steps) {
  Vector3D correction(0, 0, 0);
  int count = 0;
  double search_radius = 2.0 * thickness;

  vector<PointMass *> nearby;
  query(pm.position, search_radius, nearby);

  for (PointMass *candidate : nearby) {
    if (candidate == &pm) continue;

    Vector3D diff = pm.position - candidate->position;
    double dist = diff.norm();

    if (dist < search_radius) {
      correction += diff.unit() * (search_radius - dist);
      count++;
    }
  }

  if (count > 0) {
    pm.position += correction / (double)count / simulation_steps;
  }
}
