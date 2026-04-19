#ifndef HAIR_SYSTEM_H
#define HAIR_SYSTEM_H

#include <string>
#include <vector>
#include <nanogui/nanogui.h>

#include "strand.h"
#include "../core/wind.h"
#include "../collision/collisionObject.h"

using namespace CGL;
using namespace std;
using namespace nanogui;

enum class HairSimStrategy {
  DFTL_CORE = 0,
  FTL_REFERENCE = 1
};

enum class HairRenderStrategy {
  DEBUG_LINES = 0,
  FUR_RIBBONS = 1
};

enum class HairAttachmentType {
  AUTO = 0,
  SPHERE = 1,
  BOX = 2,
  EXPLICIT_ROOTS = 3
};

struct HairAttachmentSettings {
  HairAttachmentType type = HairAttachmentType::AUTO;
  int proxy_index = -1;
  bool upper_hemisphere_only = true;
  int num_guides = 180;
  int followers_per_guide = 6;
};

struct FurRenderSettings {
  float root_width = 0.0035f;
  float tip_width = 0.0008f;
  float alpha = 0.90f;
  float primary_spec_power = 70.0f;
  float secondary_spec_power = 20.0f;
  float color_noise = 0.18f;
};

struct HairParameters {
  int num_strands = 1200;               // Visible strands (guides + followers)
  int particles_per_strand = 10;
  double strand_length = 0.3;
  double damping = 0.01;
  double mass_per_particle = 0.01;
  double dftl_velocity_damping = 0.85;  // Eq. (9)-style correction factor

  HairSimStrategy sim_strategy = HairSimStrategy::DFTL_CORE;
  HairRenderStrategy render_strategy = HairRenderStrategy::FUR_RIBBONS;
  HairAttachmentSettings attachment;
  FurRenderSettings fur_render;
  WindParams wind;

  unsigned int random_seed = 1337;
  string preset = "dense_fur";
};

struct RootAnchor {
  int proxy_index = -1;
  HairAttachmentType proxy_type = HairAttachmentType::EXPLICIT_ROOTS;
  Vector3D local_position;
  Vector3D local_normal;
  Vector3D world_position;
  Vector3D world_normal;
};

struct FollowerBinding {
  int guide_a = 0;
  int guide_b = 0;
  double blend = 0.5;
  Vector3D root_offset_local;
  double width_scale = 1.0;
};

const char *simStrategyName(HairSimStrategy strategy);
const char *renderStrategyName(HairRenderStrategy strategy);
const char *attachmentTypeName(HairAttachmentType type);

HairSimStrategy parseSimStrategy(const std::string &name);
HairRenderStrategy parseRenderStrategy(const std::string &name);
HairAttachmentType parseAttachmentType(const std::string &name);

// Main hair/fur simulation system.
// Maintains guide strands (simulated) and follower strands (interpolated).
class HairSystem {
public:
  HairSystem() {}
  ~HairSystem() {}

  // Build or rebuild strands from explicit roots or attachment proxy settings.
  void buildStrands(const vector<Vector3D> &root_positions,
                    const vector<Vector3D> &root_normals,
                    HairParameters *params,
                    vector<CollisionObject *> *collision_objects);

  // Backward-compatible overload for existing call sites.
  void buildStrands(const vector<Vector3D> &root_positions,
                    const vector<Vector3D> &root_normals,
                    HairParameters *params) {
    buildStrands(root_positions, root_normals, params, nullptr);
  }

  // Rebuild from the last scene source data.
  void rebuild(HairParameters *params, vector<CollisionObject *> *collision_objects);

  // Run one simulation step.
  void simulate(double frames_per_sec, double simulation_steps,
                HairParameters *params,
                vector<Vector3D> external_accelerations,
                vector<CollisionObject *> *collision_objects);

  // Render all visible strands using active render strategy.
  void render(GLShader &shader, HairParameters *params,
              const Vector3D &camera_pos);

  // Reset all guide/follower strands.
  void reset();

  int visibleStrandCount() const;
  int guideStrandCount() const { return (int)guide_strands.size(); }
  int followerStrandCount() const { return (int)follower_strands.size(); }

  double maxRelativeLengthError() const;
  double maxRootDrift() const;
  double sim_time = 0.0;

private:
  vector<Strand> guide_strands;
  vector<Strand> follower_strands;
  vector<RootAnchor> anchors;
  vector<int> guide_anchor_indices;
  vector<double> guide_width_scales;
  vector<double> guide_color_variations;
  vector<FollowerBinding> follower_bindings;
  vector<double> root_drift_samples;

  vector<Vector3D> source_explicit_roots;
  vector<Vector3D> source_explicit_normals;
  vector<CollisionObject *> *bound_collision_objects = nullptr;

  void buildGuidesAndFollowers(HairParameters *params,
                               vector<CollisionObject *> *collision_objects);
  void buildAnchorsFromProxies(HairParameters *params,
                               vector<CollisionObject *> *collision_objects);
  void buildAnchorsFromExplicitRoots(const vector<Vector3D> &root_positions,
                                     const vector<Vector3D> &root_normals);
  void updateAnchorWorldPositions(vector<CollisionObject *> *collision_objects);
  void updatePinnedRootsFromAnchors();
  void simulateGuidesDFTL(double delta_t, HairParameters *params,
                          const vector<Vector3D> &external_accelerations,
                          vector<CollisionObject *> *collision_objects);
  void simulateGuidesReference(double delta_t, HairParameters *params,
                               const vector<Vector3D> &external_accelerations,
                               vector<CollisionObject *> *collision_objects);
  void updateFollowersFromGuides();
  vector<Strand *> getVisibleStrands();
  vector<const Strand *> getVisibleStrands() const;
};

#endif /* HAIR_SYSTEM_H */
