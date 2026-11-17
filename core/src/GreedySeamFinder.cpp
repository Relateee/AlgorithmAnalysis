#include "seam/GreedySeamFinder.h"

namespace seam {

Seam GreedySeamFinder::findVerticalSeam(const SeamInput& in) {
    const int rows = in.energy.rows;

    // TODO(spec 3):
    //   start = argmin of energy row 0
    //   for r = 1..rows-1: seam[r] = cheapest of seam[r-1]-1, seam[r-1], seam[r-1]+1 (in range)
    //
    // opts_.tryAllStarts -> repeat from every column of row 0, return the seam with the
    //                        lowest seamCost(). Still greedy, still not guaranteed optimal.

    return Seam(rows, 0); // placeholder
}

} // namespace seam
