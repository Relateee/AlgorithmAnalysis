#pragma once

#include "seam/Types.h"

#include <limits>
#include <vector>

namespace seam {

// Directed flow network with residual edges, stored as an edge list + adjacency by index
// (edge e and its reverse are e and e ^ 1).
class FlowGraph {
public:
    using Cap = double;
    static constexpr Cap kInf = std::numeric_limits<Cap>::infinity();

    explicit FlowGraph(int nodeCount);

    int nodeCount() const { return static_cast<int>(adj_.size()); }

    // Adds u -> v with capacity `cap` (and reverse residual edge with capacity `revCap`).
    void addEdge(int u, int v, Cap cap, Cap revCap = 0);

    // Returns the max-flow value from s to t.
    Cap maxFlow(int s, int t, MaxFlowAlgorithm algo);

    // After maxFlow(): inSource[v] == true if v is reachable from s in the residual graph.
    std::vector<bool> minCutSourceSide(int s) const;

private:
    Cap edmondsKarp(int s, int t);
    Cap dinic(int s, int t);

    struct Edge {
        int to;
        Cap cap; // remaining residual capacity
    };
    std::vector<Edge>             edges_;
    std::vector<std::vector<int>> adj_; // adj_[u] = indices into edges_
};

} // namespace seam
