#include "seam/Carver.h"

#include "seam/Energy.h"
#include "seam/Timer.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <stdexcept>
#include <cstring>

namespace seam {

Carver::Carver(std::unique_ptr<SeamFinder> finder, EnergyType energy)
    : finder_(std::move(finder)), energy_(energy) {
    if (!finder_)
        throw std::invalid_argument("Carver needs a SeamFinder");
}

Seam Carver::findSeam(const cv::Mat& bgr, Orientation o) {
    CV_Assert(!bgr.empty());
    // Horizontal seams = vertical seams of the transposed image.
    cv::Mat img;
    if (o == Orientation::Horizontal)
        cv::transpose(bgr, img);
    else
        img = bgr;

    Timer t;
    const cv::Mat gray   = toGray(img);
    const cv::Mat energy = computeEnergy(gray, energy_);
    stats_.msEnergy += t.ms();

    t.reset();
    Seam seam = finder_->findVerticalSeam({energy, gray});
    CV_Assert(isValidSeam(seam, img.rows, img.cols));
    stats_.msFind += t.ms();

    stats_.totalSeamCost += seamCost(energy, seam);
    return seam;
}

cv::Mat Carver::removeSeams(const cv::Mat& bgr, int count, Orientation o) {
    CV_Assert(!bgr.empty());
    const int dimension = o == Orientation::Vertical ? bgr.cols : bgr.rows;
    if (count < 0 || count >= dimension)
        throw std::invalid_argument("Seam count must be nonnegative and leave at least one pixel.");
    if (incremental_)
        return removeSeamsIncremental(bgr, count, o);
    cv::Mat img = bgr.clone();
    for (int i = 0; i < count; ++i) {
        const Seam seam = findSeam(img, o);

        Timer t;
        if (o == Orientation::Vertical) {
            img = removeVerticalSeam(img, seam);
        } else {
            cv::Mat transposed;
            cv::transpose(img, transposed);
            cv::transpose(removeVerticalSeam(transposed, seam), img);
        }
        stats_.msRemove += t.ms();
        ++stats_.seamsRemoved;
    }
    return img;
}

// Same loop as removeSeams, but gray and energy are kept and patched after every removal.
// Both filters read only the 3x3 window around a pixel (Gradient a +-1 cross, Sobel the full
// block). After removing seam s, new pixel (r, c) sees the same window as before unless a
// removed pixel fell inside it, i.e. unless s[r'] lies in [c, c+1] for some r' in r-1..r+1.
// So in row r only columns [lo-1, hi] can change, where lo/hi are the min/max seam column over
// rows r-1..r+1. Replicated borders need no extra care: clamping is unchanged by the removal.
// Everything else keeps its energy (just shifted left with the image).
cv::Mat Carver::removeSeamsIncremental(const cv::Mat& bgr, int count, Orientation o) {
    // Horizontal seams are vertical seams of the transposed image; stay transposed for the loop.
    cv::Mat img;
    if (o == Orientation::Horizontal)
        cv::transpose(bgr, img);
    else
        img = bgr.clone();

    Timer t;
    cv::Mat gray   = toGray(img);
    cv::Mat energy = computeEnergy(gray, energy_);
    stats_.msEnergy += t.ms();

    for (int i = 0; i < count; ++i) {
        t.reset();
        const Seam seam = finder_->findVerticalSeam({energy, gray});
        CV_Assert(isValidSeam(seam, img.rows, img.cols));
        stats_.msFind += t.ms();
        stats_.totalSeamCost += seamCost(energy, seam);

        t.reset();
        img    = removeVerticalSeam(img, seam);
        gray   = removeVerticalSeam(gray, seam);
        energy = removeVerticalSeam(energy, seam);
        stats_.msRemove += t.ms();

        t.reset();
        const int rows = img.rows;
        for (int r = 0; r < rows; ++r) {
            int lo = seam[r], hi = seam[r];
            if (r > 0) { lo = std::min(lo, seam[r - 1]); hi = std::max(hi, seam[r - 1]); }
            if (r + 1 < rows) { lo = std::min(lo, seam[r + 1]); hi = std::max(hi, seam[r + 1]); }
            updateEnergyRegion(gray, energy_, energy, r, lo - 1, hi);
        }
        stats_.msEnergy += t.ms();
        ++stats_.seamsRemoved;
    }

    if (o == Orientation::Horizontal) {
        cv::Mat out;
        cv::transpose(img, out);
        return out;
    }
    return img;
}

cv::Mat Carver::resize(const cv::Mat& bgr, int targetWidth, int targetHeight) {
    CV_Assert(!bgr.empty());
    if (targetWidth < 1 || targetHeight < 1 || targetWidth > bgr.cols || targetHeight > bgr.rows)
        throw std::invalid_argument("Target dimensions must be positive and cannot enlarge the image.");
    // Simple order: all vertical seams, then all horizontal ones.
    // (Optional extra: choose the optimal interleaving with a second DP - see the paper.)
    cv::Mat out = removeSeams(bgr, bgr.cols - targetWidth, Orientation::Vertical);
    return removeSeams(out, out.rows - targetHeight, Orientation::Horizontal);
}

cv::Mat Carver::removeVerticalSeam(const cv::Mat& img, const Seam& seam) {
    CV_Assert(!img.empty() && img.cols > 1);
    CV_Assert(isValidSeam(seam, img.rows, img.cols));
    cv::Mat out(img.rows, img.cols - 1, img.type());
    const size_t pixelBytes = img.elemSize();
    for (int r = 0; r < img.rows; ++r) {
        const size_t leftBytes = static_cast<size_t>(seam[r]) * pixelBytes;
        const size_t rightBytes = static_cast<size_t>(img.cols - seam[r] - 1) * pixelBytes;
        // Access each row separately: an input ROI need not be contiguous.
        if (leftBytes > 0)
            std::memcpy(out.ptr(r), img.ptr(r), leftBytes);
        if (rightBytes > 0)
            std::memcpy(out.ptr(r) + leftBytes, img.ptr(r) + leftBytes + pixelBytes, rightBytes);
    }
    return out;
}

cv::Mat Carver::drawSeam(const cv::Mat& bgr, const Seam& seam, Orientation o, cv::Scalar color) {
    CV_Assert(!bgr.empty() && bgr.type() == CV_8UC3);
    CV_Assert(isValidSeam(seam, o == Orientation::Vertical ? bgr.rows : bgr.cols,
                         o == Orientation::Vertical ? bgr.cols : bgr.rows));
    cv::Mat out = bgr.clone();
    const cv::Vec3b c(static_cast<uchar>(color[0]), static_cast<uchar>(color[1]),
                      static_cast<uchar>(color[2]));
    for (int i = 0; i < static_cast<int>(seam.size()); ++i) {
        if (o == Orientation::Vertical)
            out.at<cv::Vec3b>(i, seam[i]) = c;
        else
            out.at<cv::Vec3b>(seam[i], i) = c;
    }
    return out;
}

} // namespace seam
