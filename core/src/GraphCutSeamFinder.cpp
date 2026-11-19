#include "seam/GraphCutSeamFinder.h"

#include "seam/Energy.h"
#include "seam/MaxFlow.h"

namespace seam {

Seam GraphCutSeamFinder::findVerticalSeam(const SeamInput& in) {
    CV_Assert(!in.energy.empty() && in.energy.type() == CV_32F);
    if (opts_.forwardEnergy)
        CV_Assert(in.gray.type() == CV_32F && in.gray.size() == in.energy.size());
    const int rows = in.energy.rows;
    const int cols = in.energy.cols;
    if (cols == 1)
        return Seam(rows, 0); // one column: the only possible seam

    // An extra boundary node per row permits the cut after the final pixel.
    const int  stride = cols + 1;
    const int  source = rows * stride;
    const int  sink   = source + 1;
    const auto node   = [stride](int r, int c) { return r * stride + c; };
    const auto inf    = FlowGraph::kInf;
    FlowGraph  g(sink + 1);

    // Capacity of the edge leaving (r, c) to the right = cost of removing (r, c):
    // its energy e(r, c), or the forward energy |I(r, c+1) - I(r, c-1)|.
    const auto removalCost = [&](int r, int c) -> double {
        return opts_.forwardEnergy ? forwardCosts(in.gray, r, c).up : in.energy.at<float>(r, c);
    };

    for (int r = 0; r < rows; ++r) {
        // The sink is connected to a boundary beyond the last pixel.
        g.addEdge(source, node(r, 0), inf);
        g.addEdge(node(r, cols), sink, inf);

        for (int c = 0; c < cols; ++c) {
            // (r, c) -> (r, c+1) is cut when (r, c) is the removed pixel. The reverse edge
            // (r, c+1) -> (r, c) is infinite, so the source side of every row is a prefix of
            // columns and a finite cut crosses exactly one of these edges per row.
            g.addEdge(node(r, c), node(r, c + 1), removalCost(r, c), inf);
        }
        for (int c = 1; c <= cols; ++c) {
            // Infinite diagonals (r, c) -> (r-1, c-1) and (r, c) -> (r+1, c-1): if (r, c) is on
            // the source side, so are those neighbours. Removed columns in adjacent rows can
            // therefore differ by at most 1, i.e. the seam is connected.
            if (r > 0)
                g.addEdge(node(r, c), node(r - 1, c - 1), inf);
            if (r + 1 < rows)
                g.addEdge(node(r, c), node(r + 1, c - 1), inf);
        }
    }

    // Max-flow = min-cut capacity = total cost of the optimal seam.
    g.maxFlow(source, sink, opts_.maxFlow);
    const std::vector<bool> side = g.minCutSourceSide(source);

    // In each row the source side is columns 0..k; the seam pixel is the rightmost one, k.
    // The extra boundary node is on the sink side, so every pixel can be selected.
    Seam seam(rows);
    for (int r = 0; r < rows; ++r) {
        int c = cols - 1;
        while (c > 0 && !side[node(r, c)])
            --c;
        seam[r] = c;
    }
    return seam;
}

} // namespace seam
