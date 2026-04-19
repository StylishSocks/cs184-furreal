#ifndef COLLISIONOBJECT_CAPSULE_H
#define COLLISIONOBJECT_CAPSULE_H

#include "../core/particle.h"
#include "collisionObject.h"

using namespace CGL;
using namespace std;

// Capsule collision primitive: two endpoints (a, b) defining the central axis,
// plus a radius. Useful for approximating character limbs.
struct Capsule : public CollisionObject {
public:
  Capsule(const Vector3D &endpoint_a, const Vector3D &endpoint_b,
          double radius, double friction)
      : endpoint_a(endpoint_a), endpoint_b(endpoint_b),
        radius(radius), friction(friction) {}

  void render(GLShader &shader) override;
  void collide(PointMass &pm) override;
  void translate(const Vector3D &offset) override {
    endpoint_a += offset;
    endpoint_b += offset;
  }
  Vector3D getPosition() const override {
    return (endpoint_a + endpoint_b) * 0.5;
  }
  std::string getType() const override { return "Capsule"; }

  Vector3D endpoint_a;
  Vector3D endpoint_b;
  double radius;
  double friction;
};

#endif /* COLLISIONOBJECT_CAPSULE_H */
