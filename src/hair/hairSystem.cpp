#include "hairSystem.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

#include "ftlSolver.h"
#include "../core/integrator.h"
#include "../core/wind.h"
#include "../collision/box.h"
#include "../collision/sphere.h"
#include "../rendering/strandRenderer.h"

namespace {

static inline Vector3D safe_unit(const Vector3D &v, const Vector3D &fallback) {
  double n = v.norm();
  if (n < 1e-12) return fallback;
  return v / n;
}

static inline double rand01(std::mt19937 &rng) {
  static std::uniform_real_distribution<double> dist(0.0, 1.0);
  return dist(rng);
}

static inline int rand_int(std::mt19937 &rng, int lo, int hi) {
  std::uniform_int_distribution<int> dist(lo, hi);
  return dist(rng);
}

static inline Vector3D jitter_direction(std::mt19937 &rng,
                                        const Vector3D &base_dir,
                                        double max_angle_degrees) {
  const Vector3D n = safe_unit(base_dir, Vector3D(0, 1, 0));
  const double max_angle = std::max(0.0, max_angle_degrees) * M_PI / 180.0;
  if (max_angle < 1e-8) return n;

  const Vector3D helper = (fabs(n.y) < 0.95) ? Vector3D(0, 1, 0) : Vector3D(1, 0, 0);
  const Vector3D t = safe_unit(cross(n, helper), Vector3D(1, 0, 0));
  const Vector3D b = safe_unit(cross(n, t), Vector3D(0, 0, 1));

  const double phi = rand01(rng) * 2.0 * M_PI;
  const double cos_max = cos(max_angle);
  const double cos_theta = 1.0 - rand01(rng) * (1.0 - cos_max);
  const double sin_theta = sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));

  const Vector3D radial = t * cos(phi) + b * sin(phi);
  return safe_unit(n * cos_theta + radial * sin_theta, n);
}

} // namespace

const char *simStrategyName(HairSimStrategy strategy) {
  switch (strategy) {
  case HairSimStrategy::DFTL_CORE:
    return "dftl_core";
  case HairSimStrategy::FTL_REFERENCE:
    return "ftl_reference";
  default:
    return "dftl_core";
  }
}

const char *renderStrategyName(HairRenderStrategy strategy) {
  switch (strategy) {
  case HairRenderStrategy::DEBUG_LINES:
    return "debug_lines";
  case HairRenderStrategy::FUR_RIBBONS:
    return "fur_ribbons";
  default:
    return "fur_ribbons";
  }
}

const char *attachmentTypeName(HairAttachmentType type) {
  switch (type) {
  case HairAttachmentType::AUTO:
    return "auto";
  case HairAttachmentType::SPHERE:
    return "sphere";
  case HairAttachmentType::BOX:
    return "box";
  case HairAttachmentType::EXPLICIT_ROOTS:
    return "explicit_roots";
  default:
    return "auto";
  }
}

HairSimStrategy parseSimStrategy(const std::string &name) {
  if (name == "ftl_reference") return HairSimStrategy::FTL_REFERENCE;
  return HairSimStrategy::DFTL_CORE;
}

HairRenderStrategy parseRenderStrategy(const std::string &name) {
  if (name == "debug_lines") return HairRenderStrategy::DEBUG_LINES;
  return HairRenderStrategy::FUR_RIBBONS;
}

HairAttachmentType parseAttachmentType(const std::string &name) {
  if (name == "sphere") return HairAttachmentType::SPHERE;
  if (name == "box") return HairAttachmentType::BOX;
  if (name == "explicit_roots") return HairAttachmentType::EXPLICIT_ROOTS;
  return HairAttachmentType::AUTO;
}

void HairSystem::buildStrands(const vector<Vector3D> &root_positions,
                              const vector<Vector3D> &root_normals,
                              HairParameters *params,
                              vector<CollisionObject *> *collision_objects) {
  source_explicit_roots = root_positions;
  source_explicit_normals = root_normals;
  bound_collision_objects = collision_objects;
  root_drift_samples.clear();

  buildGuidesAndFollowers(params, collision_objects);
  sim_time = 0.0;
}

void HairSystem::rebuild(HairParameters *params,
                         vector<CollisionObject *> *collision_objects) {
  if (collision_objects) {
    bound_collision_objects = collision_objects;
  }
  buildGuidesAndFollowers(params, bound_collision_objects);
  sim_time = 0.0;
}

void HairSystem::buildAnchorsFromExplicitRoots(const vector<Vector3D> &root_positions,
                                               const vector<Vector3D> &root_normals) {
  anchors.clear();
  const size_t n = std::min(root_positions.size(), root_normals.size());
  anchors.reserve(n);
  for (size_t i = 0; i < n; i++) {
    RootAnchor a;
    a.proxy_index = -1;
    a.proxy_type = HairAttachmentType::EXPLICIT_ROOTS;
    a.local_position = root_positions[i];
    a.local_normal = safe_unit(root_normals[i], Vector3D(0, 1, 0));
    a.world_position = root_positions[i];
    a.world_normal = a.local_normal;
    anchors.push_back(a);
  }
}

void HairSystem::buildAnchorsFromProxies(HairParameters *params,
                                         vector<CollisionObject *> *collision_objects) {
  anchors.clear();
  if (!collision_objects || collision_objects->empty()) return;

  HairAttachmentType req = params->attachment.type;
  if (req == HairAttachmentType::EXPLICIT_ROOTS) return;

  std::vector<int> candidate_indices;
  for (size_t i = 0; i < collision_objects->size(); i++) {
    CollisionObject *obj = (*collision_objects)[i];
    const bool sphere_ok =
        dynamic_cast<Sphere *>(obj) && (req == HairAttachmentType::AUTO || req == HairAttachmentType::SPHERE);
    const bool box_ok =
        dynamic_cast<Box *>(obj) && (req == HairAttachmentType::AUTO || req == HairAttachmentType::BOX);
    if (sphere_ok || box_ok) {
      candidate_indices.push_back((int)i);
    }
  }
  if (candidate_indices.empty()) return;

  int picked = candidate_indices[0];
  if (params->attachment.proxy_index >= 0 &&
      params->attachment.proxy_index < (int)collision_objects->size()) {
    CollisionObject *forced = (*collision_objects)[params->attachment.proxy_index];
    if ((req == HairAttachmentType::AUTO ||
         (req == HairAttachmentType::SPHERE && dynamic_cast<Sphere *>(forced)) ||
         (req == HairAttachmentType::BOX && dynamic_cast<Box *>(forced)))) {
      picked = params->attachment.proxy_index;
    }
  }

  CollisionObject *obj = (*collision_objects)[picked];
  const int n_anchors = std::max(1, params->attachment.num_guides);
  std::mt19937 rng(params->random_seed + 17);

  if (Sphere *sphere = dynamic_cast<Sphere *>(obj)) {
    const Vector3D center = sphere->getPosition();
    const double radius = sphere->getRadius();
    const bool upper = params->attachment.upper_hemisphere_only;
    const double golden = 2.39996322972865332;

    anchors.reserve(n_anchors);
    for (int i = 0; i < n_anchors; i++) {
      double u = (i + 0.5) / n_anchors;
      double y = upper ? (1.0 - u) : (1.0 - 2.0 * u);
      double radial = sqrt(std::max(0.0, 1.0 - y * y));
      double phi = i * golden;
      Vector3D n(cos(phi) * radial, y, sin(phi) * radial);
      n = safe_unit(n, Vector3D(0, 1, 0));

      RootAnchor a;
      a.proxy_index = picked;
      a.proxy_type = HairAttachmentType::SPHERE;
      a.local_normal = n;
      a.local_position = n * radius;
      a.world_normal = n;
      a.world_position = center + a.local_position;
      anchors.push_back(a);
    }
    return;
  }

  if (Box *box = dynamic_cast<Box *>(obj)) {
    const Vector3D c = box->center;
    const Vector3D h = box->half_extents;
    const bool upper = params->attachment.upper_hemisphere_only;

    struct FaceDesc {
      Vector3D n;
      Vector3D axis_u;
      Vector3D axis_v;
      double half_u;
      double half_v;
      double d;
      double area;
    };
    std::vector<FaceDesc> faces;
    faces.reserve(6);

    auto add_face = [&faces](const Vector3D &n, const Vector3D &u, const Vector3D &v,
                             double half_u, double half_v, double d) {
      FaceDesc f;
      f.n = n;
      f.axis_u = u;
      f.axis_v = v;
      f.half_u = half_u;
      f.half_v = half_v;
      f.d = d;
      f.area = 4.0 * half_u * half_v;
      faces.push_back(f);
    };

    if (upper) {
      add_face(Vector3D(0, 1, 0), Vector3D(1, 0, 0), Vector3D(0, 0, 1), h.x, h.z, h.y);
    } else {
      add_face(Vector3D(1, 0, 0), Vector3D(0, 1, 0), Vector3D(0, 0, 1), h.y, h.z, h.x);
      add_face(Vector3D(-1, 0, 0), Vector3D(0, 1, 0), Vector3D(0, 0, 1), h.y, h.z, -h.x);
      add_face(Vector3D(0, 1, 0), Vector3D(1, 0, 0), Vector3D(0, 0, 1), h.x, h.z, h.y);
      add_face(Vector3D(0, -1, 0), Vector3D(1, 0, 0), Vector3D(0, 0, 1), h.x, h.z, -h.y);
      add_face(Vector3D(0, 0, 1), Vector3D(1, 0, 0), Vector3D(0, 1, 0), h.x, h.y, h.z);
      add_face(Vector3D(0, 0, -1), Vector3D(1, 0, 0), Vector3D(0, 1, 0), h.x, h.y, -h.z);
    }

    std::vector<double> prefix(faces.size(), 0.0);
    double area_sum = 0.0;
    for (size_t i = 0; i < faces.size(); i++) {
      area_sum += faces[i].area;
      prefix[i] = area_sum;
    }

    anchors.reserve(n_anchors);
    for (int i = 0; i < n_anchors; i++) {
      double r = rand01(rng) * area_sum;
      size_t face_i = 0;
      while (face_i + 1 < prefix.size() && prefix[face_i] < r) {
        face_i++;
      }
      const FaceDesc &f = faces[face_i];
      double su = (rand01(rng) * 2.0 - 1.0) * f.half_u;
      double sv = (rand01(rng) * 2.0 - 1.0) * f.half_v;

      Vector3D local = f.n * f.d + f.axis_u * su + f.axis_v * sv;
      RootAnchor a;
      a.proxy_index = picked;
      a.proxy_type = HairAttachmentType::BOX;
      a.local_position = local;
      a.local_normal = f.n;
      a.world_position = c + local;
      a.world_normal = f.n;
      anchors.push_back(a);
    }
  }
}

void HairSystem::updateAnchorWorldPositions(vector<CollisionObject *> *collision_objects) {
  for (RootAnchor &a : anchors) {
    if (a.proxy_index < 0 || !collision_objects ||
        a.proxy_index >= (int)collision_objects->size()) {
      a.world_position = a.local_position;
      a.world_normal = a.local_normal;
      continue;
    }

    CollisionObject *obj = (*collision_objects)[a.proxy_index];
    switch (a.proxy_type) {
    case HairAttachmentType::SPHERE: {
      Sphere *s = dynamic_cast<Sphere *>(obj);
      if (s) {
        a.world_position = s->getPosition() + a.local_position;
        a.world_normal = a.local_normal;
      }
      break;
    }
    case HairAttachmentType::BOX: {
      Box *b = dynamic_cast<Box *>(obj);
      if (b) {
        a.world_position = b->center + a.local_position;
        a.world_normal = a.local_normal;
      }
      break;
    }
    case HairAttachmentType::EXPLICIT_ROOTS:
    case HairAttachmentType::AUTO:
    default:
      a.world_position = a.local_position;
      a.world_normal = a.local_normal;
      break;
    }
  }
}

void HairSystem::buildGuidesAndFollowers(HairParameters *params,
                                         vector<CollisionObject *> *collision_objects) {
  guide_strands.clear();
  follower_strands.clear();
  guide_anchor_indices.clear();
  guide_width_scales.clear();
  guide_color_variations.clear();
  follower_bindings.clear();

  if (!source_explicit_roots.empty() &&
      source_explicit_roots.size() == source_explicit_normals.size()) {
    buildAnchorsFromExplicitRoots(source_explicit_roots, source_explicit_normals);
  } else {
    buildAnchorsFromProxies(params, collision_objects);
  }

  if (anchors.empty()) return;
  updateAnchorWorldPositions(collision_objects);

  int guide_count = params->attachment.num_guides;
  if (guide_count <= 0) {
    guide_count = std::max(1, params->num_strands / std::max(1, params->attachment.followers_per_guide + 1));
  }
  guide_count = std::min(guide_count, params->num_strands);
  if (guide_count <= 0) guide_count = 1;

  std::mt19937 rng(params->random_seed);
  std::vector<int> anchor_order((int)anchors.size());
  for (int i = 0; i < (int)anchors.size(); i++) anchor_order[i] = i;
  std::shuffle(anchor_order.begin(), anchor_order.end(), rng);

  guide_strands.resize(guide_count);
  guide_anchor_indices.resize(guide_count);
  guide_width_scales.resize(guide_count, 1.0);
  guide_color_variations.resize(guide_count, 0.0);

  for (int g = 0; g < guide_count; g++) {
    int anchor_id = anchor_order[g % anchor_order.size()];
    guide_anchor_indices[g] = anchor_id;
    const RootAnchor &a = anchors[anchor_id];

    double length_scale = 0.85 + 0.30 * rand01(rng);
    double width_scale = 0.85 + 0.40 * rand01(rng);
    double color_shift = (rand01(rng) * 2.0 - 1.0) * params->fur_render.color_noise;

    guide_width_scales[g] = width_scale;
    guide_color_variations[g] = color_shift;

    // Break perfect symmetry so top-face strands on boxes can naturally droop.
    const double max_tilt_deg =
        (a.proxy_type == HairAttachmentType::BOX) ? 26.0 : 14.0;
    const Vector3D init_dir = jitter_direction(rng, a.world_normal, max_tilt_deg);

    guide_strands[g].init(a.world_position + a.world_normal * 1e-4,
                          init_dir,
                          params->strand_length * length_scale,
                          params->particles_per_strand,
                          params->mass_per_particle);
  }

  int follower_count = std::max(0, params->num_strands - guide_count);
  follower_strands.resize(follower_count);
  follower_bindings.resize(follower_count);
  for (int i = 0; i < follower_count; i++) {
    FollowerBinding b;
    b.guide_a = rand_int(rng, 0, guide_count - 1);
    b.guide_b = rand_int(rng, 0, guide_count - 1);
    if (guide_count > 1 && b.guide_a == b.guide_b) {
      b.guide_b = (b.guide_b + 1) % guide_count;
    }
    b.blend = rand01(rng);
    double angle = rand01(rng) * 2.0 * M_PI;
    double r = (0.5 + rand01(rng)) * params->fur_render.root_width * 3.0;
    b.root_offset_local = Vector3D(cos(angle) * r, sin(angle) * r, 0.0);
    b.width_scale = 0.70 + 0.60 * rand01(rng);
    follower_bindings[i] = b;

    const RootAnchor &a = anchors[guide_anchor_indices[b.guide_a]];
    follower_strands[i].init(a.world_position + a.world_normal * 1e-4,
                             a.world_normal,
                             params->strand_length,
                             params->particles_per_strand,
                             params->mass_per_particle);
  }

  updateFollowersFromGuides();
}

void HairSystem::updatePinnedRootsFromAnchors() {
  if (anchors.empty()) return;
  for (size_t g = 0; g < guide_strands.size(); g++) {
    Strand &strand = guide_strands[g];
    if (strand.particles.empty()) continue;

    const RootAnchor &a = anchors[guide_anchor_indices[g]];
    PointMass &root = strand.particles[0];
    Vector3D old_root = root.position;
    Vector3D delta = a.world_position - old_root;
    root_drift_samples.push_back(delta.norm());

    for (PointMass &pm : strand.particles) {
      pm.position += delta;
      pm.last_position += delta;
      pm.start_position += delta;
    }

    root.position = a.world_position;
    root.last_position = a.world_position;
    root.start_position = a.world_position;
    root.pinned = true;
  }
}

void HairSystem::simulateGuidesDFTL(double delta_t, HairParameters *params,
                                    const vector<Vector3D> &external_accelerations,
                                    vector<CollisionObject *> *collision_objects) {
  for (Strand &strand : guide_strands) {
    const int n = strand.num_particles();
    if (n == 0) continue;

    vector<Vector3D> x(n, Vector3D(0, 0, 0));
    for (int i = 0; i < n; i++) {
      x[i] = strand.particles[i].position;
    }

    // Prediction step
    for (int i = 0; i < n; i++) {
      PointMass &pm = strand.particles[i];
      if (pm.pinned) {
        pm.position = pm.start_position;
        pm.last_position = pm.start_position;
        continue;
      }

      Vector3D force(0, 0, 0);
      for (const Vector3D &a : external_accelerations) {
        force += a * pm.mass;
      }

      if (params->wind.enable) {
        Vector3D tangent(0, 1, 0);
        if (i > 0 && i + 1 < n) {
          tangent = safe_unit(strand.particles[i + 1].position - strand.particles[i - 1].position,
                              Vector3D(0, 1, 0));
        } else if (i > 0) {
          tangent = safe_unit(strand.particles[i].position - strand.particles[i - 1].position,
                              Vector3D(0, 1, 0));
        } else if (i + 1 < n) {
          tangent = safe_unit(strand.particles[i + 1].position - strand.particles[i].position,
                              Vector3D(0, 1, 0));
        }
        force += compute_wind_force(pm.position, tangent, pm.mass, params->wind, sim_time);
      }

      Vector3D accel = force / pm.mass;
      Vector3D predicted =
          pm.position +
          (1.0 - params->damping) * (pm.position - pm.last_position) +
          accel * delta_t * delta_t;
      pm.position = predicted;
    }

    vector<Vector3D> corrections;
    solve_ftl_with_corrections(strand, corrections);

    if (collision_objects) {
      for (PointMass &pm : strand.particles) {
        if (pm.pinned) continue;
        for (CollisionObject *co : *collision_objects) {
          co->collide(pm);
        }
      }
      vector<Vector3D> collision_corr;
      solve_ftl_with_corrections(strand, collision_corr);
      if (collision_corr.size() == corrections.size()) {
        for (size_t i = 0; i < corrections.size(); i++) {
          corrections[i] += collision_corr[i];
        }
      }
    }

    // DFTL velocity update with correction term from next particle.
    for (int i = 0; i < n; i++) {
      PointMass &pm = strand.particles[i];
      if (pm.pinned) {
        pm.position = pm.start_position;
        pm.last_position = pm.start_position;
        continue;
      }

      Vector3D v = (pm.position - x[i]) / delta_t;
      if (i + 1 < n) {
        v += params->dftl_velocity_damping * (-corrections[i + 1] / delta_t);
      }
      pm.last_position = pm.position - v * delta_t;
    }
  }
}

void HairSystem::simulateGuidesReference(double delta_t, HairParameters *params,
                                         const vector<Vector3D> &external_accelerations,
                                         vector<CollisionObject *> *collision_objects) {
  for (Strand &strand : guide_strands) {
    const int n = strand.num_particles();
    for (int i = 0; i < n; i++) {
      PointMass &pm = strand.particles[i];
      pm.forces = Vector3D(0, 0, 0);
      if (pm.pinned) {
        pm.position = pm.start_position;
        pm.last_position = pm.start_position;
        continue;
      }

      for (const Vector3D &a : external_accelerations) {
        pm.forces += a * pm.mass;
      }

      if (params->wind.enable) {
        Vector3D tangent(0, 1, 0);
        if (i > 0 && i + 1 < n) {
          tangent = safe_unit(strand.particles[i + 1].position - strand.particles[i - 1].position,
                              Vector3D(0, 1, 0));
        } else if (i > 0) {
          tangent = safe_unit(strand.particles[i].position - strand.particles[i - 1].position,
                              Vector3D(0, 1, 0));
        } else if (i + 1 < n) {
          tangent = safe_unit(strand.particles[i + 1].position - strand.particles[i].position,
                              Vector3D(0, 1, 0));
        }
        pm.forces += compute_wind_force(pm.position, tangent, pm.mass, params->wind, sim_time);
      }
    }

    for (PointMass &pm : strand.particles) {
      verlet_step(pm, delta_t, params->damping);
    }

    solve_ftl(strand);

    if (collision_objects) {
      for (PointMass &pm : strand.particles) {
        if (pm.pinned) continue;
        for (CollisionObject *co : *collision_objects) {
          co->collide(pm);
        }
      }
    }
  }
}

void HairSystem::updateFollowersFromGuides() {
  if (follower_strands.empty() || guide_strands.empty()) return;

  for (size_t i = 0; i < follower_strands.size(); i++) {
    Strand &out = follower_strands[i];
    const FollowerBinding &bind = follower_bindings[i];
    const Strand &sa = guide_strands[bind.guide_a];
    const Strand &sb = guide_strands[bind.guide_b];

    int n = std::min(out.num_particles(), std::min(sa.num_particles(), sb.num_particles()));
    if (n < 2) continue;

    Vector3D dA = sa.particles[1].position - sa.particles[0].position;
    Vector3D dB = sb.particles[1].position - sb.particles[0].position;
    Vector3D root_dir = safe_unit((1.0 - bind.blend) * dA + bind.blend * dB, Vector3D(0, 1, 0));

    Vector3D helper = fabs(root_dir.y) < 0.95 ? Vector3D(0, 1, 0) : Vector3D(1, 0, 0);
    Vector3D t = safe_unit(cross(root_dir, helper), Vector3D(1, 0, 0));
    Vector3D b = safe_unit(cross(root_dir, t), Vector3D(0, 0, 1));
    Vector3D root_offset = t * bind.root_offset_local.x + b * bind.root_offset_local.y +
                           root_dir * bind.root_offset_local.z;

    for (int p = 0; p < n; p++) {
      double s = (double)p / (double)(n - 1);
      Vector3D pa = sa.particles[p].position;
      Vector3D pb = sb.particles[p].position;
      Vector3D base = pa * (1.0 - bind.blend) + pb * bind.blend;
      Vector3D pos = base + root_offset * (1.0 - s);

      PointMass &pm = out.particles[p];
      pm.position = pos;
      pm.last_position = pos;
      pm.start_position = pos;
      pm.pinned = (p == 0);
    }
  }
}

void HairSystem::simulate(double frames_per_sec, double simulation_steps,
                          HairParameters *params,
                          vector<Vector3D> external_accelerations,
                          vector<CollisionObject *> *collision_objects) {
  if (guide_strands.empty()) return;

  if (collision_objects) {
    bound_collision_objects = collision_objects;
  } else {
    collision_objects = bound_collision_objects;
  }

  const double delta_t = 1.0 / frames_per_sec / simulation_steps;
  updateAnchorWorldPositions(collision_objects);
  updatePinnedRootsFromAnchors();

  if (params->sim_strategy == HairSimStrategy::FTL_REFERENCE) {
    simulateGuidesReference(delta_t, params, external_accelerations, collision_objects);
  } else {
    simulateGuidesDFTL(delta_t, params, external_accelerations, collision_objects);
  }

  updateFollowersFromGuides();
  sim_time += delta_t;
}

void HairSystem::syncAttachmentTransforms(vector<CollisionObject *> *collision_objects) {
  if (guide_strands.empty()) return;

  if (collision_objects) {
    bound_collision_objects = collision_objects;
  } else {
    collision_objects = bound_collision_objects;
  }

  updateAnchorWorldPositions(collision_objects);
  updatePinnedRootsFromAnchors();
  updateFollowersFromGuides();
}

vector<Strand *> HairSystem::getVisibleStrands() {
  vector<Strand *> all;
  all.reserve(guide_strands.size() + follower_strands.size());
  for (Strand &s : guide_strands) all.push_back(&s);
  for (Strand &s : follower_strands) all.push_back(&s);
  return all;
}

vector<const Strand *> HairSystem::getVisibleStrands() const {
  vector<const Strand *> all;
  all.reserve(guide_strands.size() + follower_strands.size());
  for (const Strand &s : guide_strands) all.push_back(&s);
  for (const Strand &s : follower_strands) all.push_back(&s);
  return all;
}

void HairSystem::render(GLShader &shader, HairParameters *params,
                        const Vector3D &camera_pos) {
  StrandRenderer renderer;
  std::vector<RenderableStrand> renderables;
  renderables.reserve(guide_strands.size() + follower_strands.size());

  for (size_t i = 0; i < guide_strands.size(); i++) {
    RenderableStrand r;
    r.strand = &guide_strands[i];
    r.width_scale = (float)guide_width_scales[i];
    r.color_variation = (float)guide_color_variations[i];
    renderables.push_back(r);
  }

  for (size_t i = 0; i < follower_strands.size(); i++) {
    RenderableStrand r;
    r.strand = &follower_strands[i];
    r.width_scale = (float)follower_bindings[i].width_scale;
    r.color_variation = 0.0f;
    renderables.push_back(r);
  }

  if (params->render_strategy == HairRenderStrategy::DEBUG_LINES) {
    renderer.renderLines(renderables, shader);
    return;
  }

  renderer.renderRibbons(renderables, shader, camera_pos,
                         params->fur_render.root_width,
                         params->fur_render.tip_width);
}

int HairSystem::visibleStrandCount() const {
  return (int)(guide_strands.size() + follower_strands.size());
}

double HairSystem::maxRelativeLengthError() const {
  double max_err = 0.0;
  for (const Strand &s : guide_strands) {
    for (int i = 0; i < (int)s.rest_lengths.size(); i++) {
      double current = (s.particles[i + 1].position - s.particles[i].position).norm();
      double rest = s.rest_lengths[i];
      if (rest < 1e-12) continue;
      double rel = fabs(current - rest) / rest;
      max_err = std::max(max_err, rel);
    }
  }
  return max_err;
}

double HairSystem::maxRootDrift() const {
  double max_drift = 0.0;
  for (double d : root_drift_samples) {
    max_drift = std::max(max_drift, d);
  }
  return max_drift;
}

void HairSystem::reset() {
  for (Strand &s : guide_strands) {
    s.reset();
  }
  updateFollowersFromGuides();
  sim_time = 0.0;
  root_drift_samples.clear();
}
