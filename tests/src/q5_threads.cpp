// Question 5d: multi-threaded DP must give exactly the same seam as the single-thread DP.
#include <seam/Carver.h>
#include <seam/Energy.h>
#include <seam/SeamFinder.h>

#include <opencv2/core.hpp>

#include <iostream>
#include <memory>
#include <stdexcept>

namespace {

void check(bool condition) {
    if (!condition) throw std::runtime_error("Check failed");
}

seam::Seam findSeam(const cv::Mat& energy, const cv::Mat& gray, bool forward, bool rolling,
                    int threads) {
    seam::FinderOptions options;
    options.dp.forwardEnergy = forward;
    options.dp.rollingRows = rolling;
    options.dp.threads = threads;
    return seam::makeSeamFinder(seam::Algorithm::DP, options)->findVerticalSeam({energy, gray});
}

// Fills with random integer values in [0, levels) stored as float.
void fillMap(cv::Mat& m, cv::RNG& rng, int levels) {
    cv::Mat ints(m.size(), CV_32S);
    rng.fill(ints, cv::RNG::UNIFORM, 0, levels);
    ints.convertTo(m, CV_32F);
}

// Race stress: tall maps, rolling rows, many repetitions, vs 1 thread without rolling.
void raceStress() {
    for (int cols : {64, 2000}) {
        for (bool forward : {false, true}) {
            for (int rep = 0; rep < 20; ++rep) {
                cv::Mat energy(400, cols, CV_32F), gray(400, cols, CV_32F);
                cv::RNG rng(rep * 7 + cols);
                fillMap(energy, rng, rep % 2 ? 4 : 1000);
                fillMap(gray, rng, rep % 2 ? 4 : 256);
                check(findSeam(energy, gray, forward, true, 8)
                      == findSeam(energy, gray, forward, false, 1));
            }
        }
    }
    std::cout << "race stress passed\n";
}

void threadsMatchSingleThread() {
    struct Size { int rows, cols; };
    const Size sizes[] = {{1, 1}, {1, 9}, {7, 1}, {6, 2}, {5, 3}, {10, 7}, {12, 40}, {20, 500}};
    int cases = 0;
    for (const Size size : sizes) {
        for (unsigned seed = 0; seed < 6; ++seed) {
            cv::Mat energy(size.rows, size.cols, CV_32F), gray(size.rows, size.cols, CV_32F);
            cv::RNG rng(seed * 131 + size.rows * 17 + size.cols);
            // Odd seeds use only the integer values 0..2, so ties are frequent and the
            // tie-breaking is really exercised (continuous floats would never tie).
            fillMap(energy, rng, seed % 2 ? 3 : 100);
            fillMap(gray, rng, seed % 2 ? 3 : 256);
            for (bool forward : {false, true}) {
                for (bool rolling : {false, true}) {
                    // Reference: 1 thread, rolling OFF (also checks rolling == non-rolling).
                    const auto expected = findSeam(energy, gray, forward, false, 1);
                    for (int threads : {2, 3, 4, 8, 16}) {
                        const auto s = findSeam(energy, gray, forward, rolling, threads);
                        check(s == expected);
                        check(seam::seamCost(energy, s) == seam::seamCost(energy, expected));
                        ++cases;
                    }
                }
            }
        }
    }
    std::cout << cases << " multi-thread cases match the single-thread seam\n";
}

void invalidThreadCount() {
    const cv::Mat energy = cv::Mat::ones(3, 3, CV_32F);
    for (int threads : {0, -1}) {
        bool threw = false;
        try { findSeam(energy, energy, false, false, threads); }
        catch (const std::invalid_argument&) { threw = true; }
        check(threw);
    }
}

void carverEndToEnd() {
    cv::Mat image(60, 160, CV_8UC3);
    cv::RNG rng(7);
    rng.fill(image, cv::RNG::UNIFORM, 0, 256);
    for (bool forward : {false, true}) {
        cv::Mat results[2];
        int index = 0;
        for (int threads : {1, 4}) {
            seam::FinderOptions options;
            options.dp.forwardEnergy = forward;
            options.dp.threads = threads;
            seam::Carver carver(seam::makeSeamFinder(seam::Algorithm::DP, options),
                                seam::EnergyType::Gradient);
            results[index++] = carver.removeSeams(image, 30, seam::Orientation::Vertical);
        }
        check(results[0].size() == results[1].size());
        check(cv::norm(results[0], results[1], cv::NORM_INF) == 0);
    }
}

} // namespace

int main() {
    try {
        threadsMatchSingleThread();
        raceStress();
        invalidThreadCount();
        carverEndToEnd();
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << "\n";
        return 1;
    }
    std::cout << "PASS q5_threads\n";
    return 0;
}
