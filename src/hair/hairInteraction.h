#ifndef HAIR_INTERACTION_H
#define HAIR_INTERACTION_H

#include <vector>
#include "strand.h"

// Density-grid based hair-hair repulsion and friction.
// Stretch goal: approximates bulk interaction between many strands
// using a voxel grid rather than pairwise strand-strand checks.
class HairInteraction {
public:
  // Build density grid from all strands.
  void buildDensityGrid(const std::vector<Strand> &strands,
                        double cell_size);

  // Apply repulsion forces to strand particles based on local density.
  void applyRepulsion(std::vector<Strand> &strands,
                      double repulsion_strength);

private:
  // TODO: Implement voxel grid storage
};

#endif /* HAIR_INTERACTION_H */
