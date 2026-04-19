#ifndef FTL_SOLVER_H
#define FTL_SOLVER_H

#include "strand.h"

// Follow-The-Leader (FTL) constraint solver for inextensible strands.
// Performs a single root-to-tip pass to enforce rest lengths.
// Based on Müller et al., "Fast Simulation of Inextensible Hair and Fur".
void solve_ftl(Strand &strand);

#endif /* FTL_SOLVER_H */
