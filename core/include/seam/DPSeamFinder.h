#pragma once

#include "seam/SeamFinder.h"

namespace seam {

// Spec 1b: M(i, j) = e(i, j) + min(M(i-1, j-1), M(i-1, j), M(i-1, j+1)),
// then backtrack from the minimum in the last row.
// Time O(rows * cols) per seam. Space O(rows * cols), or O(cols) cost + parent table with rollingRows.
class DPSeamFinder final : public SeamFinder {
public:
    explicit DPSeamFinder(DPOptions opts = {}) : opts_(opts) {}
    Seam findVerticalSeam(const SeamInput& in) override;
    const char* name() const override { return "DP"; }

private:
    DPOptions opts_;
};

} // namespace seam
