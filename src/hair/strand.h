#ifndef STRAND_H
#define STRAND_H

#include <vector>
#include "../core/particle.h"
#include "CGL/vector3D.h"

using namespace CGL;
using namespace std;

// A single hair/fur strand represented as a chain of particles.
// Root particle is pinned; tip is free.
struct Strand {
  vector<PointMass> particles;
  vector<double> rest_lengths; // rest distance between consecutive particles

  // Initialize a straight strand from root_pos in the given direction.
  // length: total strand length
  // num_segments: number of segments (num_segments+1 particles)
  // mass_per_particle: mass assigned to each particle
  void init(const Vector3D &root_pos, const Vector3D &direction,
            double length, int num_segments, double mass_per_particle);

  // Reset all particles to their start positions.
  void reset();

  int num_particles() const { return (int)particles.size(); }
};

#endif /* STRAND_H */
