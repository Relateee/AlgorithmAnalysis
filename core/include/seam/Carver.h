#pragma once

#include "seam/SeamFinder.h"
#include "seam/Types.h"

#include <opencv2/core.hpp>

#include <memory>
#include <vector>

namespace seam {

// Time spent in each phase, accumulated over a carve - feeds the benchmark CSV.
struct CarveStats {
    int    seamsRemoved = 0;
    double msEnergy     = 0;
    double msFind       = 0;
    double msRemove     = 0;
    double totalSeamCost = 0; // sum of seamCost() of every removed seam

    double msTotal() const { return msEnergy + msFind + msRemove; }
};

// Spec 1c: compute energy -> find seam -> remove seam -> repeat.
class Carver {
public:
    Carver(std::unique_ptr<SeamFinder> finder, EnergyType energy);

    // Find (but don't remove) one seam in the given orientation.
    // Vertical: seam[row] = col. Horizontal: seam[col] = row.
    Seam findSeam(const cv::Mat& bgr, Orientation o);

    // Remove `count` seams of one orientation.
    cv::Mat removeSeams(const cv::Mat& bgr, int count, Orientation o);

    // Resize to (targetWidth, targetHeight), both <= current size.
    cv::Mat resize(const cv::Mat& bgr, int targetWidth, int targetHeight);

    // Q5: keep image/gray/energy across iterations and recompute energy only near the removed
    // seam. Output is identical to the full recompute; default off.
    void setIncrementalEnergy(bool on) { incremental_ = on; }
    bool incrementalEnergy() const { return incremental_; }

    const CarveStats& stats() const { return stats_; }
    void resetStats() { stats_ = {}; }

    // Removes one vertical seam from any Mat type (image or energy map).
    static cv::Mat removeVerticalSeam(const cv::Mat& img, const Seam& seam);

    // Paints a seam onto a copy of the image (for visual comparison).
    static cv::Mat drawSeam(const cv::Mat& bgr, const Seam& seam, Orientation o,
                            cv::Scalar color = {0, 0, 255});

private:
    std::unique_ptr<SeamFinder> finder_;
    EnergyType                  energy_;
    CarveStats                  stats_;
    bool                        incremental_ = false;

    cv::Mat removeSeamsIncremental(const cv::Mat& bgr, int count, Orientation o);
};

} // namespace seam
