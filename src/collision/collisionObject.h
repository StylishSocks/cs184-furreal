#ifndef COLLISIONOBJECT
#define COLLISIONOBJECT

#include <string>
#include <nanogui/nanogui.h>

#include "../core/particle.h"

using namespace CGL;
using namespace std;
using namespace nanogui;

class CollisionObject {
public:
  virtual void render(GLShader &shader) = 0;
  virtual void collide(PointMass &pm) = 0;
  virtual void translate(const Vector3D &offset) {}
  virtual Vector3D getPosition() const { return Vector3D(0, 0, 0); }
  virtual std::string getType() const { return "Object"; }

private:
  double friction;
};

#endif /* COLLISIONOBJECT */
