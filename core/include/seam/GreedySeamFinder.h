#pragma once

#include "seam/SeamFinder.h"

namespace seam {

// Spec 3: start at the lowest-energy pixel of the top row, then in each following row
// step to the cheapest of the 3 neighbours below. O(rows) per seam (O(rows * cols) with
// tryAllStarts). Not optimal - compare its seamCost() against DP.
class GreedySeamFinder final : public SeamFinder {
public:
    explicit GreedySeamFinder(GreedyOptions opts = {}) : opts_(opts) {}
    Seam findVerticalSeam(const SeamInput& in) override;
    const char* name() const override { return "Greedy"; }

private:
    GreedyOptions opts_;
};

} // namespace seam
