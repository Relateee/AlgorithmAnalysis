#include "seam/DPSeamFinder.h"

#include "seam/Energy.h"

#include <algorithm>
#include <barrier>
#include <stdexcept>
#include <thread>
#include <vector>

namespace seam {

Seam DPSeamFinder::findVerticalSeam(const SeamInput& in) {
    CV_Assert(!in.energy.empty() && in.energy.type() == CV_32F);
    if (opts_.threads < 1)
        throw std::invalid_argument("DPOptions::threads must be at least 1.");
    if (opts_.forwardEnergy)
        CV_Assert(in.gray.type() == CV_32F && in.gray.size() == in.energy.size());
    const int rows = in.energy.rows;
    const int cols = in.energy.cols;
    // Double precision avoids accumulating rounding error over long seams.
    cv::Mat costs(opts_.rollingRows ? 2 : rows, cols, CV_64F);
    cv::Mat parents(rows, cols, CV_32S);
    for (int c = 0; c < cols; ++c)
        costs.at<double>(0, c) = in.energy.at<float>(0, c);

    // Fills columns [c0, c1) of row r. It reads only row r-1 (`previous`) and writes only its own
    // columns of row r (`current`, `parents`), so disjoint column ranges never conflict.
    auto computeRange = [&](int r, int c0, int c1) {
        const double* previous = costs.ptr<double>(opts_.rollingRows ? (r - 1) % 2 : r - 1);
        double* current = costs.ptr<double>(opts_.rollingRows ? r % 2 : r);
        for (int c = c0; c < c1; ++c) {
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
    };

    // Never more threads than columns (an empty chunk would do nothing but wait).
    // With fewer than 2 rows there is nothing to compute in parallel either.
    const int threads = rows < 2 ? 1 : std::min(opts_.threads, cols);
    if (threads == 1) {
        // Plain sequential path: no thread or barrier overhead.
        for (int r = 1; r < rows; ++r)
            computeRange(r, 0, cols);
    } else {
        // Row r needs the finished row r-1, so ROWS MUST STAY SEQUENTIAL. Only the columns of one
        // row are independent, so each thread owns a contiguous block of columns. The workers are
        // created once per call (creating threads per row would cost far more than a row of work);
        // the barrier makes every thread wait until all of them finished row r before anyone
        // starts row r+1. The per-row work is tiny, so this only pays off for wide images.
        // Rolling rows need no second barrier: buffer r%2 is overwritten again only in row r+2,
        // i.e. after the barrier that ends row r+1, by which time everyone has finished reading it.
        // computeRange is not expected to throw; only thread creation can fail (handled below).
        // The arithmetic per column is identical to the sequential path, so the seam is identical.
        std::barrier rowDone(threads);
        auto worker = [&](int t) {
            const int c0 = static_cast<int>(static_cast<long long>(cols) * t / threads);
            const int c1 = static_cast<int>(static_cast<long long>(cols) * (t + 1) / threads);
            for (int r = 1; r < rows; ++r) {
                computeRange(r, c0, c1);
                rowDone.arrive_and_wait();
            }
        };
        std::vector<std::jthread> pool; // joins automatically on scope exit
        try {
            for (int t = 1; t < threads; ++t)
                pool.emplace_back(worker, t);
        } catch (...) {
            // Spawning failed: the barrier still expects the missing threads and this one. Drop
            // them so the workers already running can finish (their join happens during unwinding
            // when `pool` is destroyed), then report the error.
            const int started = static_cast<int>(pool.size());
            for (int i = 0; i < threads - 1 - started + 1; ++i)
                rowDone.arrive_and_drop();
            throw;
        }
        worker(0); // the calling thread takes the first block
    }

    const double* last = costs.ptr<double>(opts_.rollingRows ? (rows - 1) % 2 : rows - 1);
    Seam seam(rows);
    seam.back() = static_cast<int>(std::min_element(last, last + cols) - last);
    for (int r = rows - 1; r > 0; --r)
        seam[r - 1] = parents.at<int>(r, seam[r]);
    return seam;
}

} // namespace seam
