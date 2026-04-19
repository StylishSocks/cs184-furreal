#include "objLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool OBJMesh::load(const string &filename) {
  ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: could not open OBJ file: " << filename << std::endl;
    return false;
  }

  string line;
  while (getline(file, line)) {
    istringstream iss(line);
    string prefix;
    iss >> prefix;

    if (prefix == "v") {
      double x, y, z;
      iss >> x >> y >> z;
      vertices.push_back(Vector3D(x, y, z));
    } else if (prefix == "vn") {
      double x, y, z;
      iss >> x >> y >> z;
      normals.push_back(Vector3D(x, y, z));
    } else if (prefix == "vt") {
      double u, v;
      iss >> u >> v;
      texcoords.push_back(Vector3D(u, v, 0));
    } else if (prefix == "f") {
      vector<int> face;
      string vertex;
      while (iss >> vertex) {
        // Parse "v", "v/vt", "v/vt/vn", or "v//vn"
        int v_idx = 0;
        sscanf(vertex.c_str(), "%d", &v_idx);
        face.push_back(v_idx - 1); // Convert to 0-based
      }
      faces.push_back(face);
    }
  }

  file.close();
  std::cout << "Loaded OBJ: " << vertices.size() << " vertices, "
            << faces.size() << " faces" << std::endl;
  return true;
}
