#include "seam/Energy.h"

#include <opencv2/imgproc.hpp>

namespace seam {

cv::Mat toGray(const cv::Mat& bgr) {
    cv::Mat gray8, gray32;
    if (bgr.channels() == 3)
        cv::cvtColor(bgr, gray8, cv::COLOR_BGR2GRAY);
    else
        gray8 = bgr;
    gray8.convertTo(gray32, CV_32F);
    return gray32;
}

cv::Mat computeEnergy(const cv::Mat& gray, EnergyType type) {
    CV_Assert(gray.type() == CV_32F);
    cv::Mat energy = cv::Mat::zeros(gray.size(), CV_32F);

    switch (type) {
        case EnergyType::Gradient:
            // TODO(spec 1a): e(r, c) = |I(r, c+1) - I(r, c-1)| + |I(r+1, c) - I(r-1, c)|
            //                Clamp to the border at the edges.
            break;
        case EnergyType::Sobel:
            // TODO(spec 1a): apply 3x3 Sobel kernels by hand, e = |Gx| + |Gy|
            break;
    }
    return energy;
}

ForwardCosts forwardCosts(const cv::Mat& gray, int r, int c) {
    // TODO: implement the CU / CL / CR formulas documented in Energy.h.
    (void)gray;
    (void)r;
    (void)c;
    return {0.f, 0.f, 0.f};
}

double seamCost(const cv::Mat& energy, const Seam& seam) {
    double sum = 0;
    for (int r = 0; r < static_cast<int>(seam.size()); ++r)
        sum += energy.at<float>(r, seam[r]);
    return sum;
}

} // namespace seam
