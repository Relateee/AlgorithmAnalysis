#pragma once

#include "seam/Types.h"

#include <opencv2/core.hpp>

#include <memory>

namespace seam {

// Everything a finder may need. Finders only ever find VERTICAL seams;
// Carver transposes the image for horizontal ones.
struct SeamInput {
    const cv::Mat& energy; // CV_32F energy map
    const cv::Mat& gray;   // CV_32F intensity (needed for forward energy)
};

class SeamFinder {
public:
    virtual ~SeamFinder() = default;
    virtual Seam findVerticalSeam(const SeamInput& in) = 0;
    virtual const char* name() const = 0;
};

// Options for the section 5d experiments - each can be toggled and benchmarked.
struct DPOptions {
    bool forwardEnergy = false; // use forwardCosts() instead of the plain energy map
    bool rollingRows   = false; // keep only 2 rows of cumulative cost (+ parent table)
    int  threads       = 1;     // split each row across threads
};

struct GreedyOptions {
    bool tryAllStarts = false; // run greedy from every top-row column, keep the cheapest
};

struct GraphCutOptions {
    bool             forwardEnergy = true;
    MaxFlowAlgorithm maxFlow       = MaxFlowAlgorithm::Dinic;
};

struct FinderOptions {
    DPOptions       dp;
    GreedyOptions   greedy;
    GraphCutOptions graphCut;
};

std::unique_ptr<SeamFinder> makeSeamFinder(Algorithm algo, const FinderOptions& opts = {});

// True if the seam has one entry per row, stays in bounds, and moves at most 1 column per row.
bool isValidSeam(const Seam& seam, int rows, int cols);

} // namespace seam
