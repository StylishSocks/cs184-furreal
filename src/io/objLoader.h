#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <string>
#include <vector>
#include "CGL/vector3D.h"

using namespace CGL;
using namespace std;

// Simple OBJ mesh loader for character models.
struct OBJMesh {
  vector<Vector3D> vertices;
  vector<Vector3D> normals;
  vector<Vector3D> texcoords;
  vector<vector<int>> faces; // Each face is a list of vertex indices (0-based)

  bool load(const string &filename);
};

#endif /* OBJ_LOADER_H */
