#ifndef SCENE_LOADER_H
#define SCENE_LOADER_H

#include <string>
#include <vector>
#include "../collision/collisionObject.h"
#include "../hair/hairSystem.h"

using namespace std;

// Scene loading from JSON files.
// Extracted and generalized from HW4 main.cpp loadObjectsFromFile().
class SceneLoader {
public:
  // Load collision objects and hair parameters from a JSON scene file.
  // Returns false if the file could not be opened.
  static bool loadScene(const string &filename,
                        HairParameters *hair_params,
                        vector<CollisionObject *> *objects,
                        vector<CGL::Vector3D> *root_positions,
                        vector<CGL::Vector3D> *root_normals,
                        int sphere_num_lat = 40,
                        int sphere_num_lon = 40);
};

#endif /* SCENE_LOADER_H */
