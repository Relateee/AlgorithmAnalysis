#include "seam/Carver.h"

#include "seam/Energy.h"
#include "seam/Timer.h"

#include <opencv2/imgproc.hpp>

#include <stdexcept>

namespace seam {

Carver::Carver(std::unique_ptr<SeamFinder> finder, EnergyType energy)
    : finder_(std::move(finder)), energy_(energy) {
    if (!finder_)
        throw std::invalid_argument("Carver needs a SeamFinder");
}

Seam Carver::findSeam(const cv::Mat& bgr, Orientation o) {
    // Horizontal seams = vertical seams of the transposed image.
    cv::Mat img = bgr;
    if (o == Orientation::Horizontal)
        cv::transpose(bgr, img);

    Timer t;
    const cv::Mat gray   = toGray(img);
    const cv::Mat energy = computeEnergy(gray, energy_);
    stats_.msEnergy += t.ms();

    t.reset();
    Seam seam = finder_->findVerticalSeam({energy, gray});
    stats_.msFind += t.ms();

    stats_.totalSeamCost += seamCost(energy, seam);
    return seam;
}

cv::Mat Carver::removeSeams(const cv::Mat& bgr, int count, Orientation o) {
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

cv::Mat Carver::resize(const cv::Mat& bgr, int targetWidth, int targetHeight) {
    CV_Assert(targetWidth <= bgr.cols && targetHeight <= bgr.rows);
    // Simple order: all vertical seams, then all horizontal ones.
    // (Optional extra: choose the optimal interleaving with a second DP - see the paper.)
    cv::Mat out = removeSeams(bgr, bgr.cols - targetWidth, Orientation::Vertical);
    return removeSeams(out, out.rows - targetHeight, Orientation::Horizontal);
}

cv::Mat Carver::removeVerticalSeam(const cv::Mat& img, const Seam& seam) {
    // TODO(spec 1c): build a Mat with cols - 1 columns; for each row r copy every pixel
    //                except column seam[r] (works for any type: use img.elemSize()).
    (void)seam;
    return img.clone();
}

cv::Mat Carver::drawSeam(const cv::Mat& bgr, const Seam& seam, Orientation o, cv::Scalar color) {
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
