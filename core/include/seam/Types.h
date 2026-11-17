#pragma once

#include <vector>

namespace seam {

// A vertical seam: seam[row] = column of the pixel removed in that row.
// Horizontal seams are found on the transposed image, so they use the same type
// (seam[col] = row) - see Carver.
using Seam = std::vector<int>;

enum class Orientation { Vertical, Horizontal };

enum class EnergyType {
    Gradient, // |dI/dx| + |dI/dy| with simple central differences
    Sobel,    // |Gx| + |Gy| with 3x3 Sobel kernels
};

enum class Algorithm { DP, Greedy, GraphCut };

enum class MaxFlowAlgorithm { EdmondsKarp, Dinic };

inline const char* toString(Algorithm a) {
    switch (a) {
        case Algorithm::DP:       return "DP";
        case Algorithm::Greedy:   return "Greedy";
        case Algorithm::GraphCut: return "GraphCut";
    }
    return "?";
}

inline const char* toString(EnergyType e) {
    switch (e) {
        case EnergyType::Gradient: return "Gradient";
        case EnergyType::Sobel:    return "Sobel";
    }
    return "?";
}

} // namespace seam
