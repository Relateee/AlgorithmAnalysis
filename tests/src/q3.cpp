// Question 3: greedy seam finder.

#include <seam/Carver.h>
#include <seam/Energy.h>
#include <seam/SeamFinder.h>

#include <opencv2/core.hpp>

#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

void check(bool condition) {
    if (!condition) throw std::runtime_error("Check failed");
}

seam::Seam findWith(seam::Algorithm algo, const cv::Mat& energy, bool tryAllStarts = false) {
    seam::FinderOptions o;
    o.greedy.tryAllStarts = tryAllStarts;
    const cv::Mat gray = cv::Mat::zeros(energy.size(), CV_32F);
    return seam::makeSeamFinder(algo, o)->findVerticalSeam({energy, gray});
}

cv::Mat randomEnergy(int rows, int cols, unsigned seed) {
    cv::Mat e(rows, cols, CV_32F);
    cv::RNG rng(seed);
    rng.fill(e, cv::RNG::UNIFORM, 0, 100);
    return e;
}

// Independent restatement of the greedy rule, checked step by step against the output.
void greedyFollowsLocalRule() {
    for (int rows : {1, 2, 7, 20}) {
        for (int cols : {1, 2, 3, 15}) {
            for (unsigned seed = 0; seed < 10; ++seed) {
                const cv::Mat e = randomEnergy(rows, cols, seed + 100);
                const seam::Seam s = findWith(seam::Algorithm::Greedy, e);
                check(seam::isValidSeam(s, rows, cols));
                // Start: leftmost minimum of the top row.
                for (int c = 0; c < cols; ++c)
                    check(e.at<float>(0, s[0]) < e.at<float>(0, c) || c >= s[0]);
                // Each step: the chosen pixel is the leftmost minimum of the 3 below.
                for (int r = 1; r < rows; ++r) {
                    const int lo = std::max(0, s[r - 1] - 1);
                    const int hi = std::min(cols - 1, s[r - 1] + 1);
                    check(s[r] >= lo && s[r] <= hi);
                    for (int c = lo; c <= hi; ++c)
                        check(e.at<float>(r, s[r]) < e.at<float>(r, c) || c >= s[r]);
                }
            }
        }
    }
}

void greedyNeverBeatsDP() {
    for (unsigned seed = 0; seed < 200; ++seed) {
        const cv::Mat e = randomEnergy(12 + seed % 9, 3 + seed % 13, seed);
        const double dp     = seam::seamCost(e, findWith(seam::Algorithm::DP, e));
        const double greedy = seam::seamCost(e, findWith(seam::Algorithm::Greedy, e));
        const double all    = seam::seamCost(e, findWith(seam::Algorithm::Greedy, e, true));
        check(seam::isValidSeam(findWith(seam::Algorithm::Greedy, e, true), e.rows, e.cols));
        check(dp <= all + 1e-6);     // DP is optimal
        check(all <= greedy + 1e-6); // best-of-all-starts includes the single start
    }
}

// The cheap start (column 0) leads into a wall the greedy rule cannot see coming.
//   0   5   5
//   0   5   5
// 100 100   5
// Greedy:              0 -> 0 -> 100            = 100
// Greedy, all starts:  best is col 2: 5 -> 5 -> 5 = 15
// DP (optimal):        0 -> 5 -> 5               = 10
void greedyCanBeTrapped() {
    const float values[3][3] = {{0, 5, 5}, {0, 5, 5}, {100, 100, 5}};
    cv::Mat e(3, 3, CV_32F);
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            e.at<float>(r, c) = values[r][c];

    check(seam::seamCost(e, findWith(seam::Algorithm::Greedy, e)) == 100.0);
    check(seam::seamCost(e, findWith(seam::Algorithm::Greedy, e, true)) == 15.0);
    check(seam::seamCost(e, findWith(seam::Algorithm::DP, e)) == 10.0);
}

void greedyMatchesDPOnObviousPath() {
    // A single zero-energy column: both methods must follow it.
    cv::Mat e(10, 8, CV_32F, cv::Scalar(50));
    e.col(5).setTo(0);
    check(findWith(seam::Algorithm::Greedy, e) == seam::Seam(10, 5));
    check(findWith(seam::Algorithm::DP, e) == seam::Seam(10, 5));
    // Ties: an all-zero map gives the leftmost straight seam, like DP.
    const cv::Mat zero = cv::Mat::zeros(4, 5, CV_32F);
    check(findWith(seam::Algorithm::Greedy, zero) == seam::Seam(4, 0));
    check(findWith(seam::Algorithm::Greedy, zero, true) == seam::Seam(4, 0));
}

void carverWorksWithGreedy() {
    cv::Mat image(30, 40, CV_8UC3);
    cv::RNG rng(7);
    rng.fill(image, cv::RNG::UNIFORM, 0, 256);
    const cv::Mat before = image.clone();
    for (bool all : {false, true}) {
        seam::FinderOptions o;
        o.greedy.tryAllStarts = all;
        for (auto type : {seam::EnergyType::Gradient, seam::EnergyType::Sobel}) {
            seam::Carver carver(seam::makeSeamFinder(seam::Algorithm::Greedy, o), type);
            const cv::Mat out = carver.resize(image, 25, 18);
            check(out.cols == 25 && out.rows == 18);
            check(carver.stats().seamsRemoved == 15 + 12);
            check(cv::norm(image, before, cv::NORM_INF) == 0);
        }
    }
}

} // namespace

int main() {
    int failed = 0;
    for (const auto& [name, test] : std::initializer_list<std::pair<const char*, void (*)()>>{
             {"greedy follows the local rule (start + every step)", greedyFollowsLocalRule},
             {"DP <= greedy(all starts) <= greedy on 200 random maps", greedyNeverBeatsDP},
             {"hand-built trap: greedy 100, all-starts 15, DP 10", greedyCanBeTrapped},
             {"obvious path and tie-breaking", greedyMatchesDPOnObviousPath},
             {"carver resize with greedy, both energies", carverWorksWithGreedy}}) {
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
