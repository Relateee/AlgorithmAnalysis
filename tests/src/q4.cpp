// Question 4: max-flow (Edmonds-Karp, Dinic), residual min cut, and the graph-cut seam finder.

#include <seam/Carver.h>
#include <seam/Energy.h>
#include <seam/MaxFlow.h>
#include <seam/SeamFinder.h>

#include <opencv2/core.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void check(bool condition) {
    if (!condition) throw std::runtime_error("Check failed");
}

constexpr seam::MaxFlowAlgorithm kFlows[] = {seam::MaxFlowAlgorithm::EdmondsKarp,
                                             seam::MaxFlowAlgorithm::Dinic};

// ---- max-flow -------------------------------------------------------------------------------

struct EdgeSpec {
    int    u, v;
    double cap;
};

seam::FlowGraph build(int n, const std::vector<EdgeSpec>& edges) {
    seam::FlowGraph g(n);
    for (const auto& e : edges) g.addEdge(e.u, e.v, e.cap);
    return g;
}

double cutCapacity(const std::vector<EdgeSpec>& edges, const std::vector<bool>& inS) {
    double sum = 0;
    for (const auto& e : edges)
        if (inS[e.u] && !inS[e.v]) sum += e.cap;
    return sum;
}

// CLRS Figure 26.1: max flow 23, minimum cut S = {s, v1, v2, v4}.
void clrsExample() {
    const std::vector<EdgeSpec> edges = {{0, 1, 16}, {0, 2, 13}, {2, 1, 4},  {1, 3, 12}, {3, 2, 9},
                                         {2, 4, 14}, {4, 3, 7},  {3, 5, 20}, {4, 5, 4}};
    for (const auto algo : kFlows) {
        seam::FlowGraph g = build(6, edges);
        check(g.maxFlow(0, 5, algo) == 23);
        const auto side = g.minCutSourceSide(0);
        check(side == std::vector<bool>({true, true, true, false, true, false}));
        check(cutCapacity(edges, side) == 23);
    }
}

// Random networks: both algorithms agree, and the flow equals the minimum over every s-t cut
// (brute force over all 2^(n-2) cuts) - the max-flow min-cut theorem checked directly.
void randomNetworksMatchBruteForceMinCut() {
    cv::RNG rng(42);
    for (int trial = 0; trial < 300; ++trial) {
        const int n = 2 + trial % 7; // 2..8 nodes, source 0, sink n-1
        std::vector<EdgeSpec> edges;
        const int m = rng.uniform(0, 3 * n);
        for (int i = 0; i < m; ++i) {
            const int u = rng.uniform(0, n), v = rng.uniform(0, n);
            if (u != v) edges.push_back({u, v, static_cast<double>(rng.uniform(0, 20))});
        }
        double best = std::numeric_limits<double>::infinity();
        for (int mask = 0; mask < (1 << (n - 2)); ++mask) {
            std::vector<bool> inS(n, false);
            inS[0] = true;
            for (int k = 0; k < n - 2; ++k) inS[k + 1] = (mask >> k) & 1;
            best = std::min(best, cutCapacity(edges, inS));
        }
        for (const auto algo : kFlows) {
            seam::FlowGraph g = build(n, edges);
            const double flow = g.maxFlow(0, n - 1, algo);
            check(std::abs(flow - best) < 1e-9);
            const auto side = g.minCutSourceSide(0);
            check(side[0] && !side[n - 1]);
            check(std::abs(cutCapacity(edges, side) - flow) < 1e-9); // residual BFS gives a min cut
        }
    }
}

void invalidFlowRequests() {
    seam::FlowGraph g(3);
    for (const auto algo : kFlows) {
        bool threw = false;
        try { g.maxFlow(1, 1, algo); } catch (const std::invalid_argument&) { threw = true; }
        check(threw);
    }
}

// ---- graph-cut seams ------------------------------------------------------------------------
// The brief joins the last column to the sink with infinite capacity, so a graph-cut seam can
// only use columns 0 .. cols-2. The right reference is therefore the best seam of the energy
// map without its last column.

cv::Mat withoutLastColumn(const cv::Mat& m) { return m.colRange(0, m.cols - 1).clone(); }

// Independent reference: enumerate every connected seam (no DP, no graph).
double exhaustiveCost(const cv::Mat& energy) {
    double minimum = std::numeric_limits<double>::infinity();
    std::function<void(int, int, double)> visit = [&](int r, int previous, double sum) {
        if (r == energy.rows) {
            minimum = std::min(minimum, sum);
            return;
        }
        const int first = r == 0 ? 0 : std::max(0, previous - 1);
        const int last  = r == 0 ? energy.cols - 1 : std::min(energy.cols - 1, previous + 1);
        for (int c = first; c <= last; ++c)
            visit(r + 1, c, sum + energy.at<float>(r, c));
    };
    visit(0, 0, 0);
    return minimum;
}

seam::Seam graphCut(const cv::Mat& energy, const cv::Mat& gray, bool forward,
                    seam::MaxFlowAlgorithm algo) {
    seam::FinderOptions o;
    o.graphCut.forwardEnergy = forward;
    o.graphCut.maxFlow       = algo;
    return seam::makeSeamFinder(seam::Algorithm::GraphCut, o)->findVerticalSeam({energy, gray});
}

double dpCost(const cv::Mat& energy) {
    const cv::Mat gray = cv::Mat::zeros(energy.size(), CV_32F);
    return seam::seamCost(energy, seam::makeSeamFinder(seam::Algorithm::DP)->findVerticalSeam({energy, gray}));
}

bool avoidsLastColumn(const seam::Seam& s, int cols) {
    return std::all_of(s.begin(), s.end(), [cols](int c) { return c <= cols - 2; });
}

// Every size up to 6x6, both max-flow algorithms: the seam is valid, never uses the last
// column, and is as cheap as the exhaustive optimum (and DP) over the other columns.
void graphCutMatchesExhaustive() {
    for (int rows = 1; rows <= 6; ++rows) {
        for (int cols = 2; cols <= 6; ++cols) {
            for (unsigned seed = 0; seed < 6; ++seed) {
                cv::Mat energy(rows, cols, CV_32F);
                cv::RNG rng(seed * 97 + rows * 7 + cols);
                rng.fill(energy, cv::RNG::UNIFORM, 0, 40);
                const cv::Mat gray  = cv::Mat::zeros(energy.size(), CV_32F);
                const cv::Mat inner = withoutLastColumn(energy);
                const double  best  = exhaustiveCost(inner);
                check(std::abs(dpCost(inner) - best) < 1e-3);
                for (const auto algo : kFlows) {
                    const seam::Seam s = graphCut(energy, gray, false, algo);
                    check(seam::isValidSeam(s, rows, cols) && avoidsLastColumn(s, cols));
                    check(std::abs(seam::seamCost(energy, s) - best) < 1e-3);
                    check(seam::seamCost(energy, s) >= dpCost(energy) - 1e-3); // never beats DP
                }
            }
        }
    }
}

// Larger random maps (too big to enumerate): graph cut matches DP on the map without its last
// column.
void graphCutMatchesDPOnLargerMaps() {
    for (unsigned seed = 0; seed < 20; ++seed) {
        cv::Mat energy(25 + seed, 18 + seed % 7, CV_32F);
        cv::RNG rng(seed + 1000);
        rng.fill(energy, cv::RNG::UNIFORM, 0, 255);
        const cv::Mat gray = cv::Mat::zeros(energy.size(), CV_32F);
        const double  best = dpCost(withoutLastColumn(energy));
        for (const auto algo : kFlows)
            check(std::abs(seam::seamCost(energy, graphCut(energy, gray, false, algo)) - best) < 1e-2);
    }
}

// Forward option: the edge capacity becomes |I(r, c+1) - I(r, c-1)|, so the result must equal
// a backward-energy graph cut run on a map of those values.
void forwardEnergyUsesNeighbourDifference() {
    for (unsigned seed = 0; seed < 10; ++seed) {
        cv::Mat gray(12, 9, CV_32F);
        cv::RNG rng(seed + 50);
        rng.fill(gray, cv::RNG::UNIFORM, 0, 256);
        cv::Mat difference(gray.size(), CV_32F);
        for (int r = 0; r < gray.rows; ++r)
            for (int c = 0; c < gray.cols; ++c)
                difference.at<float>(r, c) = seam::forwardCosts(gray, r, c).up;
        const cv::Mat energy = cv::Mat::zeros(gray.size(), CV_32F); // ignored in forward mode
        for (const auto algo : kFlows)
            check(graphCut(energy, gray, true, algo) == graphCut(difference, gray, false, algo));
    }
}

// As in the brief, the last column is wired to the sink and can never be removed, even when
// it is the cheapest path. The first column can be.
void lastColumnIsNeverRemoved() {
    cv::Mat energy(8, 6, CV_32F, cv::Scalar(50));
    energy.col(5).setTo(0);
    const cv::Mat gray = cv::Mat::zeros(energy.size(), CV_32F);
    for (const auto algo : kFlows) {
        const seam::Seam s = graphCut(energy, gray, false, algo);
        check(avoidsLastColumn(s, 6) && seam::seamCost(energy, s) == 8 * 50.0);
    }
    energy.setTo(50);
    energy.col(0).setTo(0);
    for (const auto algo : kFlows)
        check(graphCut(energy, gray, false, algo) == seam::Seam(8, 0));
}

// A zero-cost diagonal: the seam must follow it, so the infinite diagonal edges
// do allow (and only allow) one-column steps.
void followsDiagonalPath() {
    cv::Mat energy(6, 7, CV_32F, cv::Scalar(100));
    for (int r = 0; r < 6; ++r) energy.at<float>(r, 5 - r) = 0;
    const cv::Mat gray = cv::Mat::zeros(energy.size(), CV_32F);
    for (const auto algo : kFlows)
        check(graphCut(energy, gray, false, algo) == seam::Seam({5, 4, 3, 2, 1, 0}));
}

// A zero-cost path that jumps two columns is NOT a valid seam; graph cut must not take it.
void rejectsDisconnectedCheapPath() {
    cv::Mat energy(4, 6, CV_32F, cv::Scalar(10));
    energy.at<float>(0, 0) = 0;
    energy.at<float>(1, 2) = 0;
    energy.at<float>(2, 4) = 0;
    energy.at<float>(3, 2) = 0;
    const cv::Mat gray = cv::Mat::zeros(energy.size(), CV_32F);
    for (const auto algo : kFlows) {
        const seam::Seam s = graphCut(energy, gray, false, algo);
        check(seam::isValidSeam(s, 4, 6));
        check(std::abs(seam::seamCost(energy, s) - exhaustiveCost(withoutLastColumn(energy))) < 1e-6);
    }
}

// Through the Carver: both orientations and a resize on an image, never cheaper than DP.
void carverWithGraphCut() {
    cv::Mat image(24, 30, CV_8UC3);
    cv::RNG rng(5);
    rng.fill(image, cv::RNG::UNIFORM, 0, 256);
    const cv::Mat before = image.clone();
    for (const auto algo : kFlows) {
        for (auto type : {seam::EnergyType::Gradient, seam::EnergyType::Sobel}) {
            seam::FinderOptions o;
            o.graphCut.maxFlow = algo;
            seam::Carver gc(seam::makeSeamFinder(seam::Algorithm::GraphCut, o), type);
            seam::Carver ref(seam::makeSeamFinder(seam::Algorithm::DP), type);
            for (auto orient : {seam::Orientation::Vertical, seam::Orientation::Horizontal}) {
                gc.resetStats();
                ref.resetStats();
                const seam::Seam s = gc.findSeam(image, orient);
                ref.findSeam(image, orient);
                check(avoidsLastColumn(s, orient == seam::Orientation::Vertical ? image.cols : image.rows));
                check(gc.stats().totalSeamCost >= ref.stats().totalSeamCost - 1e-3);
            }
            const cv::Mat out = gc.resize(image, 26, 21);
            check(out.cols == 26 && out.rows == 21 && out.type() == image.type());
            check(cv::norm(image, before, cv::NORM_INF) == 0);
        }
    }
}

} // namespace

int main() {
    int failed = 0;
    for (const auto& [name, test] : std::initializer_list<std::pair<const char*, void (*)()>>{
             {"max-flow: CLRS example, both algorithms, min cut side", clrsExample},
             {"max-flow = brute-force min cut on 300 random networks", randomNetworksMatchBruteForceMinCut},
             {"max-flow rejects source == sink", invalidFlowRequests},
             {"graph cut = exhaustive optimum without the last column (all sizes to 6x6)", graphCutMatchesExhaustive},
             {"graph cut = DP without the last column on larger maps", graphCutMatchesDPOnLargerMaps},
             {"forward option uses |I(r,c+1) - I(r,c-1)|", forwardEnergyUsesNeighbourDifference},
             {"last column never removed, first column can be", lastColumnIsNeverRemoved},
             {"follows a diagonal zero-cost seam", followsDiagonalPath},
             {"ignores a cheap but disconnected path", rejectsDisconnectedCheapPath},
             {"carver: both orientations, both energies, resize", carverWithGraphCut}}) {
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
