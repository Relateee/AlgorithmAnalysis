#include "seam/GraphCutSeamFinder.h"

#include "seam/MaxFlow.h"

namespace seam {

Seam GraphCutSeamFinder::findVerticalSeam(const SeamInput& in) {
    const int rows = in.energy.rows;
    const int cols = in.energy.cols;

    // Node layout: pixel (r, c) -> r * cols + c, then source and sink.
    const int source = rows * cols;
    const int sink   = source + 1;
    FlowGraph g(rows * cols + 2);
    (void)sink;

    // TODO(spec 4):
    //   1. source -> (r, 0) and (r, cols-1) -> sink, infinite.
    //   2. (r, c) -> (r, c+1) with capacity = cost of removing (r, c)
    //        energy:          in.energy.at<float>(r, c)
    //        forward energy:  see forwardCosts() / opts_.forwardEnergy
    //      reverse edge (r, c+1) -> (r, c) infinite (use addEdge's revCap).
    //   3. (r, c) -> (r-1, c-1) and (r, c) -> (r+1, c-1), infinite.
    //   4. g.maxFlow(source, sink, opts_.maxFlow);
    //   5. side = g.minCutSourceSide(source);
    //      seam[r] = rightmost c with side[r * cols + c] == true.

    (void)g;
    (void)cols;
    return Seam(rows, 0); // placeholder
}

} // namespace seam
