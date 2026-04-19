#ifndef FTL_SOLVER_H
#define FTL_SOLVER_H

#include "strand.h"
#include <vector>

// Follow-The-Leader (FTL) constraint solver for inextensible strands.
// Performs a single root-to-tip pass to enforce rest lengths.
// Based on Müller et al., "Fast Simulation of Inextensible Hair and Fur".
void solve_ftl(Strand &strand);

// Same projection as solve_ftl, but returns the correction vector d_i for each
// particle i (index 0 is always zero).
void solve_ftl_with_corrections(Strand &strand,
                                std::vector<CGL::Vector3D> &corrections);

#endif /* FTL_SOLVER_H */
