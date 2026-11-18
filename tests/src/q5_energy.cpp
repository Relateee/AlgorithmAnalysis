// Q5: incremental (local) energy updates must reproduce the full recompute exactly.
#include <seam/Carver.h>
#include <seam/Energy.h>
#include <seam/SeamFinder.h>

#include <opencv2/core.hpp>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void check(bool condition, const std::string& what = "") {
    if (!condition) throw std::runtime_error("Check failed " + what);
}

// Wraps a finder: records every seam and (optionally) verifies that the energy map handed to
// the finder equals computeEnergy(gray) bit for bit - i.e. the incremental map is exact.
class Probe : public seam::SeamFinder {
public:
    Probe(std::unique_ptr<seam::SeamFinder> inner, seam::EnergyType type, bool verify)
        : inner_(std::move(inner)), type_(type), verify_(verify) {}
    seam::Seam findVerticalSeam(const seam::SeamInput& in) override {
        if (verify_) {
            const cv::Mat full = seam::computeEnergy(in.gray, type_);
            check(full.size() == in.energy.size(), "energy size");
            check(cv::countNonZero(full != in.energy) == 0, "energy map differs from full recompute");
            ++verified;
        }
        seam::Seam s = inner_->findVerticalSeam(in);
        seams.push_back(s);
        return s;
    }
    const char* name() const override { return "probe"; }
    std::vector<seam::Seam> seams;
    int verified = 0;
private:
    std::unique_ptr<seam::SeamFinder> inner_;
    seam::EnergyType type_;
    bool verify_;
};

cv::Mat randomImage(int rows, int cols, cv::RNG& rng, int mode) {
    cv::Mat img(rows, cols, CV_8UC3);
    rng.fill(img, cv::RNG::UNIFORM, 0, 256);
    if (mode == 1) {            // cheap column on the left border: seams hug column 0
        img.col(0).setTo(cv::Scalar(0, 0, 0));
        if (cols > 1) img.col(1).setTo(cv::Scalar(0, 0, 0));
    } else if (mode == 2) {     // cheap columns on the right border
        img.col(cols - 1).setTo(cv::Scalar(255, 255, 255));
        if (cols > 1) img.col(cols - 2).setTo(cv::Scalar(255, 255, 255));
    } else if (mode == 3) {     // low-contrast noise: many ties and tiny energies
        cv::add(img / 64, cv::Scalar::all(100), img);
    }
    return img;
}

struct Config {
    const char* name;
    seam::Algorithm algo;
    bool forward;
};

int cases = 0, steps = 0;

void compare(const cv::Mat& img, int count, seam::Orientation o, seam::EnergyType type,
             const Config& cfg) {
    seam::FinderOptions opts;
    opts.dp.forwardEnergy = cfg.forward;

    auto full = std::make_unique<Probe>(seam::makeSeamFinder(cfg.algo, opts), type, false);
    auto inc = std::make_unique<Probe>(seam::makeSeamFinder(cfg.algo, opts), type, true);
    Probe* fullRaw = full.get();
    Probe* incRaw = inc.get();
    seam::Carver fullCarver(std::move(full), type);
    seam::Carver incCarver(std::move(inc), type);
    incCarver.setIncrementalEnergy(true);

    const cv::Mat a = fullCarver.removeSeams(img, count, o);
    const cv::Mat b = incCarver.removeSeams(img, count, o);

    check(a.size() == b.size() && a.type() == b.type(), "output size");
    check(cv::countNonZero(a.reshape(1) != b.reshape(1)) == 0, "output image differs");
    check(fullRaw->seams == incRaw->seams, "seams differ");
    check(incRaw->verified == count, "energy verified every step");
    check(fullCarver.stats().totalSeamCost == incCarver.stats().totalSeamCost, "total cost");
    check(incCarver.stats().seamsRemoved == count, "seam count");
    ++cases;
    steps += count;
}

} // namespace

int main() {
    const Config configs[] = {
        {"dp", seam::Algorithm::DP, false},
        {"dp-forward", seam::Algorithm::DP, true},
        {"greedy", seam::Algorithm::Greedy, false},
    };
    const int sizes[][2] = {{2, 2}, {1, 5}, {5, 1}, {2, 7}, {7, 2}, {3, 3}, {4, 9}, {9, 4},
                            {16, 16}, {23, 11}, {11, 23}, {40, 31}};
    cv::RNG rng(12345);
    for (const auto type : {seam::EnergyType::Gradient, seam::EnergyType::Sobel}) {
        for (const auto& cfg : configs) {
            for (const auto& sz : sizes) {
                for (int mode = 0; mode < 4; ++mode) {
                    for (int rep = 0; rep < 2; ++rep) {
                        const cv::Mat img = randomImage(sz[0], sz[1], rng, mode);
                        for (const auto o : {seam::Orientation::Vertical,
                                             seam::Orientation::Horizontal}) {
                            const int dim = o == seam::Orientation::Vertical ? img.cols : img.rows;
                            // 0 seams, 1 seam, half, and the maximum (leaves 1 row/column).
                            for (const int count : {0, 1, dim / 2, dim - 1})
                                if (count >= 0 && count < dim)
                                    compare(img, count, o, type, cfg);
                        }
                    }
                }
            }
        }
    }

    // resize() goes through removeSeams twice (vertical then horizontal).
    {
        const cv::Mat img = randomImage(30, 40, rng, 0);
        seam::Carver a(seam::makeSeamFinder(seam::Algorithm::DP), seam::EnergyType::Sobel);
        seam::Carver b(seam::makeSeamFinder(seam::Algorithm::DP), seam::EnergyType::Sobel);
        b.setIncrementalEnergy(true);
        const cv::Mat ra = a.resize(img, 25, 20), rb = b.resize(img, 25, 20);
        check(ra.size() == rb.size() && cv::countNonZero(ra.reshape(1) != rb.reshape(1)) == 0,
              "resize output");
    }

    std::cout << "q5 energy: " << cases << " cases, " << steps
              << " seam removals, incremental == full recompute\n";
    return 0;
}
