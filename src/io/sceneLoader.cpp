#include "sceneLoader.h"

#include <iostream>
#include <fstream>
#include "../json.hpp"
#include "../collision/sphere.h"
#include "../collision/plane.h"
#include "../collision/box.h"
#include "../collision/capsule.h"

using json = nlohmann::json;

static void incompleteObjectError(const char *object, const char *attribute) {
  std::cout << "Incomplete " << object << " definition, missing " << attribute << std::endl;
  exit(-1);
}

bool SceneLoader::loadScene(const string &filename,
                            HairParameters *hair_params,
                            vector<CollisionObject *> *objects,
                            vector<CGL::Vector3D> *root_positions,
                            vector<CGL::Vector3D> *root_normals,
                            int sphere_num_lat,
                            int sphere_num_lon) {
  ifstream i(filename);
  if (!i.good()) {
    return false;
  }
  json j;
  i >> j;

  for (json::iterator it = j.begin(); it != j.end(); ++it) {
    string key = it.key();
    json object = it.value();

    if (key == "sphere") {
      CGL::Vector3D origin;
      double radius, friction;

      auto it_origin = object.find("origin");
      if (it_origin != object.end()) {
        vector<double> vec_origin = *it_origin;
        origin = CGL::Vector3D(vec_origin[0], vec_origin[1], vec_origin[2]);
      } else {
        incompleteObjectError("sphere", "origin");
      }

      auto it_radius = object.find("radius");
      if (it_radius != object.end()) {
        radius = *it_radius;
      } else {
        incompleteObjectError("sphere", "radius");
      }

      auto it_friction = object.find("friction");
      if (it_friction != object.end()) {
        friction = *it_friction;
      } else {
        incompleteObjectError("sphere", "friction");
      }

      Sphere *s = new Sphere(origin, radius, friction, sphere_num_lat, sphere_num_lon);
      objects->push_back(s);

    } else if (key == "plane") {
      CGL::Vector3D point, normal;
      double friction;

      auto it_point = object.find("point");
      if (it_point != object.end()) {
        vector<double> vec_point = *it_point;
        point = CGL::Vector3D(vec_point[0], vec_point[1], vec_point[2]);
      } else {
        incompleteObjectError("plane", "point");
      }

      auto it_normal = object.find("normal");
      if (it_normal != object.end()) {
        vector<double> vec_normal = *it_normal;
        normal = CGL::Vector3D(vec_normal[0], vec_normal[1], vec_normal[2]);
      } else {
        incompleteObjectError("plane", "normal");
      }

      auto it_friction = object.find("friction");
      if (it_friction != object.end()) {
        friction = *it_friction;
      } else {
        incompleteObjectError("plane", "friction");
      }

      Plane *p = new Plane(point, normal, friction);
      objects->push_back(p);

    } else if (key == "box") {
      CGL::Vector3D box_center, half_extents;
      double friction;

      auto it_center = object.find("center");
      if (it_center != object.end()) {
        vector<double> vec_center = *it_center;
        box_center = CGL::Vector3D(vec_center[0], vec_center[1], vec_center[2]);
      } else {
        incompleteObjectError("box", "center");
      }

      auto it_half_extents = object.find("half_extents");
      if (it_half_extents != object.end()) {
        vector<double> vec_he = *it_half_extents;
        half_extents = CGL::Vector3D(vec_he[0], vec_he[1], vec_he[2]);
      } else {
        incompleteObjectError("box", "half_extents");
      }

      auto it_friction = object.find("friction");
      if (it_friction != object.end()) {
        friction = *it_friction;
      } else {
        incompleteObjectError("box", "friction");
      }

      Box *b = new Box(box_center, half_extents, friction);
      objects->push_back(b);

    } else if (key == "capsule") {
      CGL::Vector3D ep_a, ep_b;
      double radius, friction;

      auto it_a = object.find("endpoint_a");
      if (it_a != object.end()) {
        vector<double> v = *it_a;
        ep_a = CGL::Vector3D(v[0], v[1], v[2]);
      } else {
        incompleteObjectError("capsule", "endpoint_a");
      }

      auto it_b = object.find("endpoint_b");
      if (it_b != object.end()) {
        vector<double> v = *it_b;
        ep_b = CGL::Vector3D(v[0], v[1], v[2]);
      } else {
        incompleteObjectError("capsule", "endpoint_b");
      }

      auto it_radius = object.find("radius");
      if (it_radius != object.end()) {
        radius = *it_radius;
      } else {
        incompleteObjectError("capsule", "radius");
      }

      auto it_friction = object.find("friction");
      if (it_friction != object.end()) {
        friction = *it_friction;
      } else {
        incompleteObjectError("capsule", "friction");
      }

      Capsule *c = new Capsule(ep_a, ep_b, radius, friction);
      objects->push_back(c);

    } else if (key == "hair") {
      // Parse hair simulation parameters
      auto it_num_strands = object.find("num_strands");
      if (it_num_strands != object.end()) {
        hair_params->num_strands = *it_num_strands;
      }

      auto it_particles = object.find("particles_per_strand");
      if (it_particles != object.end()) {
        hair_params->particles_per_strand = *it_particles;
      }

      auto it_length = object.find("strand_length");
      if (it_length != object.end()) {
        hair_params->strand_length = *it_length;
      }

      auto it_damping = object.find("damping");
      if (it_damping != object.end()) {
        hair_params->damping = *it_damping;
      }

      auto it_mass = object.find("mass_per_particle");
      if (it_mass != object.end()) {
        hair_params->mass_per_particle = *it_mass;
      }

      // Parse root positions if provided inline
      auto it_roots = object.find("roots");
      if (it_roots != object.end()) {
        for (auto &root : *it_roots) {
          vector<double> pos = root["position"];
          vector<double> nrm = root["normal"];
          root_positions->push_back(CGL::Vector3D(pos[0], pos[1], pos[2]));
          root_normals->push_back(CGL::Vector3D(nrm[0], nrm[1], nrm[2]));
        }
      }

    } else {
      std::cout << "Warning: unknown scene object '" << key << "'" << std::endl;
    }
  }

  i.close();
  return true;
}
