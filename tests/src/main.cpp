// Minimal self-contained tests (no framework needed). Several FAIL until the TODOs are done -
// that's intended: they tell you when each piece works.

#include <seam/Carver.h>
#include <seam/Energy.h>
#include <seam/MaxFlow.h>
#include <seam/SeamFinder.h>

#include <opencv2/core.hpp>

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                                          \
    do {                                                                                     \
        if (!(cond)) {                                                                       \
            std::cerr << "    CHECK failed: " #cond " (" __FILE__ ":" << __LINE__ << ")\n";  \
            ++g_failures;                                                                    \
        }                                                                                    \
    } while (0)

cv::Mat randomEnergy(int rows, int cols, unsigned seed) {
    cv::Mat  e(rows, cols, CV_32F);
    cv::RNG  rng(seed);
    rng.fill(e, cv::RNG::UNIFORM, 0.0, 100.0);
    return e;
}

seam::Seam find(seam::Algorithm a, const cv::Mat& energy) {
    const cv::Mat gray = cv::Mat::zeros(energy.size(), CV_32F);
    return seam::makeSeamFinder(a)->findVerticalSeam({energy, gray});
}

// ---- tests ---------------------------------------------------------------------------------

void allFindersReturnValidSeams() {
    const cv::Mat e = randomEnergy(30, 20, 1);
    for (auto a : {seam::Algorithm::DP, seam::Algorithm::Greedy, seam::Algorithm::GraphCut})
        CHECK(seam::isValidSeam(find(a, e), e.rows, e.cols));
}

void dpFindsObviousCheapPath() {
    // Energy 100 everywhere except a zero-energy diagonal path; DP must find cost 0.
    cv::Mat e(5, 5, CV_32F, cv::Scalar(100));
    const int path[5] = {4, 3, 2, 1, 0};
    for (int r = 0; r < 5; ++r) e.at<float>(r, path[r]) = 0;
    CHECK(seam::seamCost(e, find(seam::Algorithm::DP, e)) == 0.0);
}

void dpNeverWorseThanGreedy() {
    for (unsigned seed = 0; seed < 20; ++seed) {
        const cv::Mat e = randomEnergy(40, 30, seed);
        CHECK(seam::seamCost(e, find(seam::Algorithm::DP, e)) <=
              seam::seamCost(e, find(seam::Algorithm::Greedy, e)) + 1e-6);
    }
}

void graphCutMatchesDP() {
    // Graph cut with plain energy must find a seam of the same (optimal) cost as DP.
    seam::FinderOptions o;
    o.graphCut.forwardEnergy = false;
    for (unsigned seed = 0; seed < 5; ++seed) {
        const cv::Mat e    = randomEnergy(15, 12, seed);
        const cv::Mat gray = cv::Mat::zeros(e.size(), CV_32F);
        const auto    gc   = seam::makeSeamFinder(seam::Algorithm::GraphCut, o)
                              ->findVerticalSeam({e, gray});
        CHECK(std::abs(seam::seamCost(e, gc) -
                       seam::seamCost(e, find(seam::Algorithm::DP, e))) < 1e-3);
    }
}

void maxFlowClassicExample() {
    // CLRS Figure 26.1 network: max flow = 23.
    for (auto algo : {seam::MaxFlowAlgorithm::EdmondsKarp, seam::MaxFlowAlgorithm::Dinic}) {
        seam::FlowGraph g(6);
        g.addEdge(0, 1, 16); g.addEdge(0, 2, 13); g.addEdge(2, 1, 4);
        g.addEdge(1, 3, 12); g.addEdge(3, 2, 9);  g.addEdge(2, 4, 14);
        g.addEdge(4, 3, 7);  g.addEdge(3, 5, 20); g.addEdge(4, 5, 4);
        CHECK(g.maxFlow(0, 5, algo) == 23);
    }
}

void removeSeamShrinksImage() {
    cv::Mat img(10, 8, CV_8UC3, cv::Scalar(10, 20, 30));
    const seam::Seam s(10, 3);
    const cv::Mat out = seam::Carver::removeVerticalSeam(img, s);
    CHECK(out.cols == 7 && out.rows == 10);
}

void energyIsNonZeroOnEdges() {
    cv::Mat gray(10, 10, CV_32F, cv::Scalar(0));
    gray.colRange(5, 10).setTo(255); // vertical edge at column 5
    for (auto t : {seam::EnergyType::Gradient, seam::EnergyType::Sobel}) {
        const cv::Mat e = seam::computeEnergy(gray, t);
        CHECK(e.at<float>(5, 5) > 0 && e.at<float>(5, 1) == 0);
    }
}

} // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"allFindersReturnValidSeams", allFindersReturnValidSeams},
        {"energyIsNonZeroOnEdges",     energyIsNonZeroOnEdges},
        {"removeSeamShrinksImage",     removeSeamShrinksImage},
        {"dpFindsObviousCheapPath",    dpFindsObviousCheapPath},
        {"dpNeverWorseThanGreedy",     dpNeverWorseThanGreedy},
        {"maxFlowClassicExample",      maxFlowClassicExample},
        {"graphCutMatchesDP",          graphCutMatchesDP},
    };

    int failedTests = 0;
    for (const auto& [name, fn] : tests) {
        const int before = g_failures;
        fn();
        const bool ok = g_failures == before;
        failedTests += ok ? 0 : 1;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << "\n";
    }
    const int total = static_cast<int>(tests.size());
    std::cout << (total - failedTests) << "/" << total << " tests passed\n";
    return failedTests == 0 ? 0 : 1;
}
