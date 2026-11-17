#include "seam/MaxFlow.h"

namespace seam {

FlowGraph::FlowGraph(int nodeCount) : adj_(nodeCount) {}

void FlowGraph::addEdge(int u, int v, Cap cap, Cap revCap) {
    adj_[u].push_back(static_cast<int>(edges_.size()));
    edges_.push_back({v, cap});
    adj_[v].push_back(static_cast<int>(edges_.size()));
    edges_.push_back({u, revCap});
}

FlowGraph::Cap FlowGraph::maxFlow(int s, int t, MaxFlowAlgorithm algo) {
    switch (algo) {
        case MaxFlowAlgorithm::EdmondsKarp: return edmondsKarp(s, t);
        case MaxFlowAlgorithm::Dinic:       return dinic(s, t);
    }
    return 0;
}

FlowGraph::Cap FlowGraph::edmondsKarp(int s, int t) {
    // TODO: repeat { BFS from s for a shortest augmenting path; push its bottleneck } until none.
    //       O(V * E^2).
    (void)s;
    (void)t;
    return 0;
}

FlowGraph::Cap FlowGraph::dinic(int s, int t) {
    // TODO: repeat { BFS to build level graph; DFS blocking flow with per-node edge pointers }.
    //       O(V^2 * E) in general, much faster in practice on grid graphs.
    (void)s;
    (void)t;
    return 0;
}

std::vector<bool> FlowGraph::minCutSourceSide(int s) const {
    std::vector<bool> inSource(adj_.size(), false);
    // TODO: BFS/DFS from s, following only edges with cap > 0.
    inSource[s] = true;
    return inSource;
}

} // namespace seam
