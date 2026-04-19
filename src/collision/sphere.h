#ifndef COLLISIONOBJECT_SPHERE_H
#define COLLISIONOBJECT_SPHERE_H

#include "../core/particle.h"
#include "../misc/sphere_drawing.h"
#include "collisionObject.h"

using namespace CGL;
using namespace std;

struct Sphere : public CollisionObject {
public:
  Sphere(const Vector3D &origin, double radius, double friction, int num_lat = 40, int num_lon = 40)
      : origin(origin), radius(radius), radius2(radius * radius),
        friction(friction), m_sphere_mesh(Misc::SphereMesh(num_lat, num_lon)) {}

  void render(GLShader &shader) override;
  void collide(PointMass &pm) override;
  void translate(const Vector3D &offset) override { origin += offset; }
  Vector3D getPosition() const override { return origin; }
  double getRadius() const { return radius; }
  std::string getType() const override { return "Sphere"; }

private:
  Vector3D origin;
  double radius;
  double radius2;

  double friction;
  
  Misc::SphereMesh m_sphere_mesh;
};

#endif /* COLLISIONOBJECT_SPHERE_H */
