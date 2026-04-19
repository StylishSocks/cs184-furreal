#ifndef COLLISIONOBJECT_BOX_H
#define COLLISIONOBJECT_BOX_H

#include <nanogui/nanogui.h>

#include "../core/particle.h"
#include "collisionObject.h"

using namespace CGL;
using namespace std;
using namespace nanogui;

struct Box : public CollisionObject {
public:
  Box(const Vector3D &center, const Vector3D &half_extents, double friction)
      : center(center), half_extents(half_extents), friction(friction) {}

  void render(GLShader &shader) override;
  void collide(PointMass &pm) override;
  void translate(const Vector3D &offset) override { center += offset; }
  Vector3D getPosition() const override { return center; }
  std::string getType() const override { return "Box"; }

  Vector3D center;
  Vector3D half_extents;
  double friction;
};

#endif /* COLLISIONOBJECT_BOX_H */
