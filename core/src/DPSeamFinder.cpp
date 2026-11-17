#include "seam/DPSeamFinder.h"

namespace seam {

Seam DPSeamFinder::findVerticalSeam(const SeamInput& in) {
    const int rows = in.energy.rows;

    // TODO(spec 1b):
    //   1. M row 0 = energy row 0
    //   2. for each row r > 0, for each col c:
    //        M(r, c) = e(r, c) + min(M(r-1, c-1), M(r-1, c), M(r-1, c+1))   (skip out-of-range)
    //        parent(r, c) = column that gave the minimum
    //      (opts_.forwardEnergy: add forwardCosts(...).left/up/right to each candidate instead)
    //   3. start at argmin of the last row and follow parent() back up.
    //
    // Section 5d variants:
    //   opts_.rollingRows -> keep only prev/cur rows of M, but still store the parent table.
    //   opts_.threads > 1 -> columns of one row are independent; split them across threads.

    return Seam(rows, 0); // placeholder: valid straight seam down column 0
}

} // namespace seam
