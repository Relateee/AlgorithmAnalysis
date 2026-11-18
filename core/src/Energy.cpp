#include "seam/Energy.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>

namespace seam {

cv::Mat toGray(const cv::Mat& bgr) {
    CV_Assert(!bgr.empty() && (bgr.channels() == 1 || bgr.channels() == 3));
    cv::Mat gray8, gray32;
    if (bgr.channels() == 3)
        cv::cvtColor(bgr, gray8, cv::COLOR_BGR2GRAY);
    else
        gray8 = bgr;
    gray8.convertTo(gray32, CV_32F);
    return gray32;
}

namespace {

// Energy of one pixel; shared by computeEnergy and updateEnergyRegion so both give
// bit-identical values.
float pixelEnergy(const cv::Mat& gray, EnergyType type, int r, int c) {
    // Replicate the nearest border pixel, including for one-row/one-column images.
    const auto intensity = [&](int rr, int cc) {
        return gray.at<float>(std::clamp(rr, 0, gray.rows - 1),
                              std::clamp(cc, 0, gray.cols - 1));
    };
    float gx, gy;
    if (type == EnergyType::Gradient) {
        gx = intensity(r, c + 1) - intensity(r, c - 1);
        gy = intensity(r + 1, c) - intensity(r - 1, c);
    } else {
        // Sobel kernels [-1 0 1; -2 0 2; -1 0 1] and its transpose.
        gx = intensity(r - 1, c + 1) + 2.f * intensity(r, c + 1)
           + intensity(r + 1, c + 1) - intensity(r - 1, c - 1)
           - 2.f * intensity(r, c - 1) - intensity(r + 1, c - 1);
        gy = intensity(r + 1, c - 1) + 2.f * intensity(r + 1, c)
           + intensity(r + 1, c + 1) - intensity(r - 1, c - 1)
           - 2.f * intensity(r - 1, c) - intensity(r - 1, c + 1);
    }
    return std::abs(gx) + std::abs(gy);
}

} // namespace

cv::Mat computeEnergy(const cv::Mat& gray, EnergyType type) {
    CV_Assert(!gray.empty() && gray.type() == CV_32F);
    CV_Assert(type == EnergyType::Gradient || type == EnergyType::Sobel);
    cv::Mat energy = cv::Mat::zeros(gray.size(), CV_32F);
    for (int r = 0; r < gray.rows; ++r)
        updateEnergyRegion(gray, type, energy, r, 0, gray.cols - 1);
    return energy;
}

void updateEnergyRegion(const cv::Mat& gray, EnergyType type, cv::Mat& energy,
                        int row, int c0, int c1) {
    CV_Assert(!gray.empty() && gray.type() == CV_32F);
    CV_Assert(energy.size() == gray.size() && energy.type() == CV_32F);
    CV_Assert(type == EnergyType::Gradient || type == EnergyType::Sobel);
    CV_Assert(row >= 0 && row < gray.rows);
    c0 = std::max(c0, 0);
    c1 = std::min(c1, gray.cols - 1);
    for (int c = c0; c <= c1; ++c)
        energy.at<float>(row, c) = pixelEnergy(gray, type, row, c);
}

ForwardCosts forwardCosts(const cv::Mat& gray, int r, int c) {
    CV_Assert(!gray.empty() && gray.type() == CV_32F);
    CV_Assert(r >= 0 && r < gray.rows && c >= 0 && c < gray.cols);
    const float left = gray.at<float>(r, std::max(0, c - 1));
    const float right = gray.at<float>(r, std::min(gray.cols - 1, c + 1));
    const float above = gray.at<float>(std::max(0, r - 1), c);
    const float up = std::abs(right - left);
    return {up + std::abs(above - left), up, up + std::abs(above - right)};
}

double seamCost(const cv::Mat& energy, const Seam& seam) {
    CV_Assert(!energy.empty() && energy.type() == CV_32F);
    CV_Assert(static_cast<int>(seam.size()) == energy.rows);
    double sum = 0;
    for (int r = 0; r < static_cast<int>(seam.size()); ++r) {
        CV_Assert(seam[r] >= 0 && seam[r] < energy.cols);
        sum += energy.at<float>(r, seam[r]);
    }
    return sum;
}

} // namespace seam
