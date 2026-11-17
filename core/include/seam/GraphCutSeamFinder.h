#pragma once

#include "seam/SeamFinder.h"

namespace seam {

// Spec 4: one node per pixel, plus a source and a sink.
//   source -> every pixel in column 0          (infinite capacity)
//   every pixel in last column -> sink         (infinite capacity)
//   (i, j) -> (i, j+1)  capacity = cost of removing (i, j)  (energy or forward energy)
//   (i, j+1) -> (i, j)                          (infinite)
//   (i, j) -> (i-1, j-1) and (i, j) -> (i+1, j-1)  (infinite)  - forces one connected seam
// Run max-flow, BFS from the source in the residual graph to get side S; in each row the
// rightmost pixel in S is the seam pixel.
// Slow (one max-flow per seam) - use small images.
class GraphCutSeamFinder final : public SeamFinder {
public:
    explicit GraphCutSeamFinder(GraphCutOptions opts = {}) : opts_(opts) {}
    Seam findVerticalSeam(const SeamInput& in) override;
    const char* name() const override { return "GraphCut"; }

private:
    GraphCutOptions opts_;
};

} // namespace seam
