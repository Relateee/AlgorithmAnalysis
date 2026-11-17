#include "seam/SeamFinder.h"

#include "seam/DPSeamFinder.h"
#include "seam/GraphCutSeamFinder.h"
#include "seam/GreedySeamFinder.h"

#include <cstdlib>

namespace seam {

std::unique_ptr<SeamFinder> makeSeamFinder(Algorithm algo, const FinderOptions& opts) {
    switch (algo) {
        case Algorithm::DP:       return std::make_unique<DPSeamFinder>(opts.dp);
        case Algorithm::Greedy:   return std::make_unique<GreedySeamFinder>(opts.greedy);
        case Algorithm::GraphCut: return std::make_unique<GraphCutSeamFinder>(opts.graphCut);
    }
    return nullptr;
}

bool isValidSeam(const Seam& seam, int rows, int cols) {
    if (static_cast<int>(seam.size()) != rows)
        return false;
    for (int r = 0; r < rows; ++r) {
        if (seam[r] < 0 || seam[r] >= cols)
            return false;
        if (r > 0 && std::abs(seam[r] - seam[r - 1]) > 1)
            return false;
    }
    return true;
}

} // namespace seam
