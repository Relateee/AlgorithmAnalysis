#pragma once

#include "seam/Types.h"

#include <opencv2/core.hpp>

namespace seam {

// BGR (CV_8UC3) -> grayscale intensity (CV_32F). Allowed helper; not graded.
cv::Mat toGray(const cv::Mat& bgr);

// Spec 1a: per-pixel energy map (CV_32F, same size as the image).
// Implement the filters yourself (central differences / Sobel kernels) so the
// "no built-in content-aware functions" rule is clearly respected.
cv::Mat computeEnergy(const cv::Mat& gray, EnergyType type);

// Forward-energy transition costs (Rubinstein et al. 2008) for pixel (r, c) of `gray`:
//   CU = |I(r, c+1) - I(r, c-1)|                       (always paid)
//   CL = CU + |I(r-1, c) - I(r, c-1)|                  (arrived from up-left)
//   CR = CU + |I(r-1, c) - I(r, c+1)|                  (arrived from up-right)
// Used by DP (optional) and by the graph-cut edge weights, so both optimise the same cost.
struct ForwardCosts {
    float left;
    float up;
    float right;
};
ForwardCosts forwardCosts(const cv::Mat& gray, int r, int c);

// Sum of energy along a vertical seam - used to compare algorithms (spec 3, 4).
double seamCost(const cv::Mat& energy, const Seam& seam);

} // namespace seam
