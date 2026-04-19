#include "wind.h"
#include <cmath>

Vector3D compute_wind_force(const Vector3D &position, const Vector3D &normal,
                            double mass, const WindParams &params,
                            double sim_time) {
  if (!params.enable || params.direction.norm() == 0) {
    return Vector3D(0, 0, 0);
  }

  // Spatially and temporally varying gust factor
  double spatial_var = sin(position.x * 3.0 + sim_time * 2.0)
                     * cos(position.z * 2.0 + sim_time * 1.5);
  double wind_factor = 0.7 + 0.3 * spatial_var;

  // Wind as acceleration, scaled by gust factor
  Vector3D wind_accel = params.direction.unit() * params.strength * wind_factor;

  // Aerodynamic model: force proportional to normal component of wind
  double normal_component = dot(wind_accel, normal);
  return mass * normal_component * normal;
}
