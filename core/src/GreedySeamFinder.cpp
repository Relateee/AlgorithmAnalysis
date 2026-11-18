#include "seam/GreedySeamFinder.h"

#include "seam/Energy.h"

#include <algorithm>
#include <limits>

namespace seam {

namespace {

// Builds one seam from `start` by always stepping to the cheapest of the (up to) three
// pixels below the current one. Ties choose the leftmost candidate, matching DP.
// Only the current row is ever looked at: no lookahead and no table. O(rows).
Seam greedyFrom(const cv::Mat& energy, int start) {
    const int rows = energy.rows;
    const int cols = energy.cols;
    Seam seam(rows);
    seam[0] = start;
    for (int r = 1; r < rows; ++r) {
        const float* row  = energy.ptr<float>(r);
        const int    prev = seam[r - 1];
        int best = std::max(0, prev - 1);
        for (int c = best + 1; c <= std::min(cols - 1, prev + 1); ++c)
            if (row[c] < row[best])
                best = c;
        seam[r] = best;
    }
    return seam;
}

} // namespace

Seam GreedySeamFinder::findVerticalSeam(const SeamInput& in) {
    CV_Assert(!in.energy.empty() && in.energy.type() == CV_32F);
    const cv::Mat& energy = in.energy;

    if (!opts_.tryAllStarts) {
        // Start at the lowest-energy pixel of the top row (leftmost on ties).
        const float* top   = energy.ptr<float>(0);
        const int    start = static_cast<int>(std::min_element(top, top + energy.cols) - top);
        return greedyFrom(energy, start);
    }

    // Variant: run greedy from every top-row column and keep the cheapest seam.
    // O(rows * cols) like DP, but still a local rule, so still not guaranteed optimal.
    Seam   best;
    double bestCost = std::numeric_limits<double>::infinity();
    for (int start = 0; start < energy.cols; ++start) {
        Seam         candidate = greedyFrom(energy, start);
        const double cost      = seamCost(energy, candidate);
        if (cost < bestCost) {
            bestCost = cost;
            best     = std::move(candidate);
        }
    }
    return best;
}

} // namespace seam
