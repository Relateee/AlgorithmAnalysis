#include <seam/Carver.h>
#include <seam/Energy.h>
#include <seam/SeamFinder.h>

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <initializer_list>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

void check(bool condition) {
    if (!condition) throw std::runtime_error("Check failed");
}

template<class F> void rejects(F action) {
    bool threw = false;
    try { action(); }
    catch (const std::exception&) { threw = true; }
    check(threw);
}

// Exhaustively enumerate every connected seam, independently of the DP recurrence.
double exhaustiveCost(const cv::Mat& energy, const cv::Mat& gray, bool forward) {
    double minimum = std::numeric_limits<double>::infinity();
    std::function<void(int, int, double)> visit = [&](int r, int previous, double sum) {
        if (r == energy.rows) {
            minimum = std::min(minimum, sum);
            return;
        }
        const int first = r == 0 ? 0 : std::max(0, previous - 1);
        const int last = r == 0 ? energy.cols - 1 : std::min(energy.cols - 1, previous + 1);
        for (int c = first; c <= last; ++c) {
            double cost = energy.at<float>(r, c);
            if (forward && r > 0) {
                const float left = gray.at<float>(r, std::max(0, c - 1));
                const float right = gray.at<float>(r, std::min(gray.cols - 1, c + 1));
                cost = std::abs(right - left);
                if (previous != c)
                    cost += std::abs(gray.at<float>(r - 1, c) - (previous < c ? left : right));
            }
            visit(r + 1, c, sum + cost);
        }
    };
    visit(0, 0, 0);
    return minimum;
}

double objective(const cv::Mat& energy, const cv::Mat& gray, const seam::Seam& s, bool forward) {
    if (!forward) return seam::seamCost(energy, s);
    double cost = energy.at<float>(0, s[0]);
    for (int r = 1; r < energy.rows; ++r) {
        const auto f = seam::forwardCosts(gray, r, s[r]);
        cost += s[r - 1] < s[r] ? f.left : s[r - 1] > s[r] ? f.right : f.up;
    }
    return cost;
}

void energyFilters() {
    cv::Mat ramp(3, 3, CV_32F);
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            ramp.at<float>(r, c) = static_cast<float>(r * 3 + c);
    const cv::Mat gradient = seam::computeEnergy(ramp, seam::EnergyType::Gradient);
    check(gradient.at<float>(1, 1) == 8.f);
    check(gradient.at<float>(0, 0) == 4.f);
    check(gradient.at<float>(2, 2) == 4.f);
    for (int rows : {1, 2, 5}) {
        for (int cols : {1, 2, 6}) {
            cv::Mat gray(rows, cols, CV_32F);
            cv::RNG rng(rows * 10 + cols);
            rng.fill(gray, cv::RNG::UNIFORM, 0, 256);
            cv::Mat gx, gy;
            // OpenCV is a test oracle only; the production kernels are handwritten.
            cv::Sobel(gray, gx, CV_32F, 1, 0, 3, 1, 0, cv::BORDER_REPLICATE);
            cv::Sobel(gray, gy, CV_32F, 0, 1, 3, 1, 0, cv::BORDER_REPLICATE);
            const cv::Mat expected = cv::abs(gx) + cv::abs(gy);
            check(cv::norm(expected, seam::computeEnergy(gray, seam::EnergyType::Sobel), cv::NORM_INF) < 0.001);
            for (auto type : {seam::EnergyType::Gradient, seam::EnergyType::Sobel}) {
                const cv::Mat constant(rows, cols, CV_32F, cv::Scalar(42));
                check(cv::countNonZero(seam::computeEnergy(constant, type)) == 0);
            }
        }
    }
    const cv::Mat bgr(2, 3, CV_8UC3, cv::Scalar(0, 0, 255));
    const auto gray = seam::toGray(bgr);
    check(gray.type() == CV_32F && gray.size() == bgr.size());
    check(gray.at<float>(0, 0) == 76.f);
}

void dpMatchesExhaustiveSearch() {
    for (int rows = 1; rows <= 6; ++rows) {
        for (int cols = 1; cols <= 5; ++cols) {
            for (unsigned seed = 0; seed < 8; ++seed) {
                cv::Mat energy(rows, cols, CV_32F), gray(rows, cols, CV_32F);
                cv::RNG rng(seed + 1);
                rng.fill(energy, cv::RNG::UNIFORM, -5, 40);
                rng.fill(gray, cv::RNG::UNIFORM, 0, 256);
                for (bool forward : {false, true}) {
                    seam::Seam baseline;
                    for (bool rolling : {false, true}) {
                        seam::FinderOptions options;
                        options.dp.forwardEnergy = forward;
                        options.dp.rollingRows = rolling;
                        const auto s = seam::makeSeamFinder(seam::Algorithm::DP, options)
                                           ->findVerticalSeam({energy, gray});
                        check(seam::isValidSeam(s, rows, cols));
                        check(std::abs(objective(energy, gray, s, forward)
                                       - exhaustiveCost(energy, gray, forward)) < 0.001);
                        if (rolling) check(s == baseline);
                        else baseline = s;
                    }
                }
            }
        }
    }
    const cv::Mat zero = cv::Mat::zeros(4, 5, CV_32F);
    const auto s = seam::makeSeamFinder(seam::Algorithm::DP)->findVerticalSeam({zero, zero});
    check(s == seam::Seam(4, 0)); // deterministic leftmost tie-breaking
}

void removalPreservesPixels() {
    for (int type : {CV_8UC3, CV_32FC1, CV_16UC4}) {
        cv::Mat storage(5, 8, type);
        cv::RNG rng(42);
        rng.fill(storage, cv::RNG::UNIFORM, 0, 100);
        const cv::Mat roi = storage(cv::Rect(1, 1, 6, 3));
        const cv::Mat before = storage.clone();
        for (const seam::Seam s : {seam::Seam{0, 0, 0}, seam::Seam{5, 5, 5}, seam::Seam{1, 2, 3}}) {
            const cv::Mat out = seam::Carver::removeVerticalSeam(roi, s);
            check(out.rows == 3 && out.cols == 5 && out.type() == type);
            for (int r = 0; r < out.rows; ++r)
                for (int c = 0; c < out.cols; ++c)
                    check(cv::norm(out.row(r).col(c), roi.row(r).col(c < s[r] ? c : c + 1), cv::NORM_INF) == 0);
            check(cv::norm(storage, before, cv::NORM_INF) == 0);
        }
    }
}

void bothOrientationsAndResize() {
    cv::Mat image(6, 7, CV_8UC3);
    cv::RNG rng(91);
    rng.fill(image, cv::RNG::UNIFORM, 0, 256);
    const cv::Mat before = image.clone();
    for (auto type : {seam::EnergyType::Gradient, seam::EnergyType::Sobel}) {
        for (auto orientation : {seam::Orientation::Vertical, seam::Orientation::Horizontal}) {
            seam::Carver carver(seam::makeSeamFinder(seam::Algorithm::DP), type);
            cv::Mat oriented = image;
            if (orientation == seam::Orientation::Horizontal) cv::transpose(image, oriented);
            const cv::Mat gray = seam::toGray(oriented);
            const cv::Mat energy = seam::computeEnergy(gray, type);
            const auto s = carver.findSeam(image, orientation);
            check(seam::isValidSeam(s, energy.rows, energy.cols));
            check(std::abs(seam::seamCost(energy, s) - exhaustiveCost(energy, gray, false)) < 0.001);
            const cv::Mat overlay = seam::Carver::drawSeam(image, s, orientation);
            for (int i = 0; i < static_cast<int>(s.size()); ++i)
                check(overlay.at<cv::Vec3b>(orientation == seam::Orientation::Vertical ? i : s[i],
                                           orientation == seam::Orientation::Vertical ? s[i] : i) == cv::Vec3b(0, 0, 255));
        }
        seam::Carver carver(seam::makeSeamFinder(seam::Algorithm::DP), type);
        const cv::Mat out = carver.resize(image, 3, 2);
        check(out.cols == 3 && out.rows == 2 && carver.stats().seamsRemoved == 8);
        check(cv::norm(image, before, cv::NORM_INF) == 0);
        carver.resetStats();
        check(cv::norm(carver.resize(image, 7, 6), image, cv::NORM_INF) == 0);
        check(carver.stats().seamsRemoved == 0);
        check(carver.resize(image, 1, 1).size() == cv::Size(1, 1));
    }
    // Transposing a shallow alias of a square Mat would silently edit the input.
    const cv::Mat square = image.colRange(0, 6).clone();
    const cv::Mat squareBefore = square.clone();
    seam::Carver carver(seam::makeSeamFinder(seam::Algorithm::DP), seam::EnergyType::Gradient);
    carver.findSeam(square, seam::Orientation::Horizontal);
    check(cv::norm(square, squareBefore, cv::NORM_INF) == 0);
    carver.removeSeams(square, 2, seam::Orientation::Horizontal);
    check(cv::norm(square, squareBefore, cv::NORM_INF) == 0);
}

// A controlled finder verifies that every iteration receives the newly computed
// intensity/energy map, rather than an old map with its seam merely deleted.
class RecalculationFinder : public seam::SeamFinder {
public:
    int calls = 0;
    seam::Seam findVerticalSeam(const seam::SeamInput& in) override {
        check(in.gray.cols == 4 - calls);
        const float expectedFirst = calls == 0 ? 10.f : calls == 1 ? 30.f : 60.f;
        check(in.gray.at<float>(0, 0) == expectedFirst);
        check(in.energy.at<float>(0, 0) == (calls == 0 ? 20.f : 30.f));
        ++calls;
        return seam::Seam(in.energy.rows, 0);
    }
    const char* name() const override { return "recalculation test"; }
};

void energyIsRecomputed() {
    cv::Mat row(1, 4, CV_8U);
    const uchar values[] = {10, 30, 60, 90};
    for (int c = 0; c < 4; ++c)
        row.at<uchar>(0, c) = values[c];
    cv::Mat image;
    cv::repeat(row, 3, 1, image);
    auto finder = std::make_unique<RecalculationFinder>();
    auto* observer = finder.get();
    seam::Carver carver(std::move(finder), seam::EnergyType::Gradient);
    const cv::Mat out = carver.removeSeams(image, 3, seam::Orientation::Vertical);
    check(observer->calls == 3 && out.cols == 1);
    check(cv::countNonZero(out != 90) == 0);
}

void invalidInputs() {
    const cv::Mat image(3, 4, CV_8UC3, cv::Scalar(0));
    seam::Carver carver(seam::makeSeamFinder(seam::Algorithm::DP), seam::EnergyType::Gradient);
    rejects([&] { carver.resize(image, 0, 2); });
    rejects([&] { carver.resize(image, 5, 2); });
    rejects([&] { carver.removeSeams(image, -1, seam::Orientation::Vertical); });
    rejects([&] { carver.removeSeams(image, 3, seam::Orientation::Horizontal); });
    rejects([&] { seam::Carver::removeVerticalSeam(image, {0, 3, 0}); });
    rejects([&] { seam::Carver::removeVerticalSeam(image, {0, 0}); });
    rejects([&] { seam::Carver::removeVerticalSeam(image, {4, 4, 4}); });
    rejects([&] { seam::Carver::removeVerticalSeam(image.col(0), {0, 0, 0}); });
    rejects([&] { seam::computeEnergy(cv::Mat{}, seam::EnergyType::Gradient); });
    rejects([&] { carver.findSeam(cv::Mat{}, seam::Orientation::Vertical); });
}

} // namespace

int main() {
    int failed = 0;
    for (const auto& [name, test] : std::initializer_list<std::pair<const char*, void(*)()>>{
             {"energy filters and borders", energyFilters},
             {"DP versus exhaustive search (960 option combinations)", dpMatchesExhaustiveSearch},
             {"pixel preservation, types and noncontiguous inputs", removalPreservesPixels},
             {"horizontal/vertical optimum and resizing", bothOrientationsAndResize},
             {"energy recalculated after each removal", energyIsRecomputed},
             {"invalid inputs", invalidInputs}}) {
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& e) {
            ++failed;
            std::cerr << "[FAIL] " << name << ": " << e.what() << '\n';
        }
    }
    return failed == 0 ? 0 : 1;
}
