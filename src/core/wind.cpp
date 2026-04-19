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

  // Wind as acceleration, scaled by gust factor.
  Vector3D wind_accel = params.direction.unit() * params.strength * wind_factor;
  Vector3D axis = (normal.norm() > 1e-12) ? normal.unit() : Vector3D(0, 1, 0);

  // Hair responds mostly to the component perpendicular to the strand axis.
  Vector3D perpendicular = wind_accel - axis * dot(wind_accel, axis);
  Vector3D axial = axis * dot(wind_accel, axis);
  Vector3D response = perpendicular + axial * 0.15;

  return mass * response;
}
