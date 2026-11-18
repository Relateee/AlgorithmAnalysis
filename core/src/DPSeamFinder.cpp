#include "seam/DPSeamFinder.h"

#include "seam/Energy.h"

#include <algorithm>
#include <stdexcept>

namespace seam {

Seam DPSeamFinder::findVerticalSeam(const SeamInput& in) {
    CV_Assert(!in.energy.empty() && in.energy.type() == CV_32F);
    if (opts_.threads != 1)
        throw std::invalid_argument("Multi-threaded DP is not implemented yet; use one thread.");
    if (opts_.forwardEnergy)
        CV_Assert(in.gray.type() == CV_32F && in.gray.size() == in.energy.size());
    const int rows = in.energy.rows;
    const int cols = in.energy.cols;
    // Double precision avoids accumulating rounding error over long seams.
    cv::Mat costs(opts_.rollingRows ? 2 : rows, cols, CV_64F);
    cv::Mat parents(rows, cols, CV_32S);
    for (int c = 0; c < cols; ++c)
        costs.at<double>(0, c) = in.energy.at<float>(0, c);

    for (int r = 1; r < rows; ++r) {
        const double* previous = costs.ptr<double>(opts_.rollingRows ? (r - 1) % 2 : r - 1);
        double* current = costs.ptr<double>(opts_.rollingRows ? r % 2 : r);
        for (int c = 0; c < cols; ++c) {
            const ForwardCosts forward = opts_.forwardEnergy
                ? forwardCosts(in.gray, r, c) : ForwardCosts{0.f, 0.f, 0.f};
            int bestParent = std::max(0, c - 1);
            double best = 0;
            for (int p = bestParent; p <= std::min(cols - 1, c + 1); ++p) {
                const float transition = p < c ? forward.left : p > c ? forward.right : forward.up;
                const double candidate = previous[p] + transition;
                if (p == std::max(0, c - 1) || candidate < best) {
                    best = candidate;
                    bestParent = p;
                }
            }
            // Backward mode sums pixel energies. Forward mode instead pays the
            // disruption caused by newly adjacent pixels (top row uses pixel energy).
            current[c] = best + (opts_.forwardEnergy ? 0.f : in.energy.at<float>(r, c));
            parents.at<int>(r, c) = bestParent;
        }
    }

    const double* last = costs.ptr<double>(opts_.rollingRows ? (rows - 1) % 2 : rows - 1);
    Seam seam(rows);
    seam.back() = static_cast<int>(std::min_element(last, last + cols) - last);
    for (int r = rows - 1; r > 0; --r)
        seam[r - 1] = parents.at<int>(r, seam[r]);
    return seam;
}

} // namespace seam
