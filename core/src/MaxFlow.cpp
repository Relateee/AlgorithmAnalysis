#include "seam/MaxFlow.h"

#include <algorithm>
#include <queue>
#include <stdexcept>

namespace seam {

namespace {

// Residual capacities at or below this are treated as saturated. Seam-carving capacities
// are sums of integer-valued intensities, so anything this small is floating-point rounding
// left over from subtracting the same bottleneck along a path.
constexpr FlowGraph::Cap kEps = 1e-9;

} // namespace

FlowGraph::FlowGraph(int nodeCount) : adj_(nodeCount) {}

void FlowGraph::addEdge(int u, int v, Cap cap, Cap revCap) {
    adj_[u].push_back(static_cast<int>(edges_.size()));
    edges_.push_back({v, cap});
    adj_[v].push_back(static_cast<int>(edges_.size()));
    edges_.push_back({u, revCap});
}

FlowGraph::Cap FlowGraph::maxFlow(int s, int t, MaxFlowAlgorithm algo) {
    if (s < 0 || t < 0 || s >= nodeCount() || t >= nodeCount() || s == t)
        throw std::invalid_argument("maxFlow needs two different nodes inside the graph.");
    switch (algo) {
        case MaxFlowAlgorithm::EdmondsKarp: return edmondsKarp(s, t);
        case MaxFlowAlgorithm::Dinic:       return dinic(s, t);
    }
    return 0;
}

// Ford-Fulkerson with BFS: always augment along a shortest path (fewest edges) in the
// residual graph. Each augmentation saturates at least one edge, and shortest-path
// distances never decrease, giving O(V * E) augmentations of O(E) each: O(V * E^2).
FlowGraph::Cap FlowGraph::edmondsKarp(int s, int t) {
    Cap              flow = 0;
    std::vector<int> parentEdge(adj_.size());
    for (;;) {
        // BFS from s; parentEdge[v] is the residual edge used to reach v (-1 = unvisited).
        std::fill(parentEdge.begin(), parentEdge.end(), -1);
        parentEdge[s] = -2;
        std::queue<int> queue;
        queue.push(s);
        while (!queue.empty() && parentEdge[t] == -1) {
            const int u = queue.front();
            queue.pop();
            for (const int e : adj_[u]) {
                const int v = edges_[e].to;
                if (parentEdge[v] == -1 && edges_[e].cap > kEps) {
                    parentEdge[v] = e;
                    queue.push(v);
                }
            }
        }
        if (parentEdge[t] == -1)
            return flow; // no augmenting path left: the flow is maximum

        // Bottleneck = smallest residual capacity on the path (edge e ^ 1 points back to u).
        Cap bottleneck = kInf;
        for (int v = t; v != s; v = edges_[parentEdge[v] ^ 1].to)
            bottleneck = std::min(bottleneck, edges_[parentEdge[v]].cap);
        if (bottleneck == kInf)
            return kInf; // an all-infinite path: no finite cut exists

        for (int v = t; v != s; v = edges_[parentEdge[v] ^ 1].to) {
            edges_[parentEdge[v]].cap -= bottleneck;
            edges_[parentEdge[v] ^ 1].cap += bottleneck;
        }
        flow += bottleneck;
    }
}

// Dinic: BFS builds a level graph (distance from s), then a blocking flow is pushed using
// only edges that go exactly one level deeper. Each phase strictly increases the s-t
// distance, so there are at most V phases: O(V^2 * E) overall, far faster in practice.
// The DFS is iterative because seam graphs have paths with thousands of nodes, which would
// overflow the default stack with recursion.
FlowGraph::Cap FlowGraph::dinic(int s, int t) {
    const int        n = nodeCount();
    Cap              flow = 0;
    std::vector<int> level(n);
    std::vector<int> next(n); // next[u] = index into adj_[u] of the next edge to try
    std::vector<int> path;    // edges of the current partial path from s

    const auto buildLevels = [&] {
        std::fill(level.begin(), level.end(), -1);
        level[s] = 0;
        std::queue<int> queue;
        queue.push(s);
        while (!queue.empty()) {
            const int u = queue.front();
            queue.pop();
            for (const int e : adj_[u]) {
                const int v = edges_[e].to;
                if (level[v] < 0 && edges_[e].cap > kEps) {
                    level[v] = level[u] + 1;
                    queue.push(v);
                }
            }
        }
        return level[t] >= 0;
    };

    while (buildLevels()) {
        std::fill(next.begin(), next.end(), 0);
        for (;;) {
            // Advance along level-graph edges until t is reached; retreat from dead ends.
            path.clear();
            int u = s;
            while (u != t) {
                bool advanced = false;
                for (; next[u] < static_cast<int>(adj_[u].size()); ++next[u]) {
                    const int e = adj_[u][next[u]];
                    const int v = edges_[e].to;
                    if (edges_[e].cap > kEps && level[v] == level[u] + 1) {
                        path.push_back(e);
                        u = v;
                        advanced = true;
                        break;
                    }
                }
                if (advanced)
                    continue;
                if (u == s)
                    break;    // s has no usable edges left: this phase is blocked
                level[u] = -1; // dead end: never enter u again in this phase
                const int e = path.back();
                path.pop_back();
                u = edges_[e ^ 1].to;
                ++next[u];
            }
            if (u != t)
                break;

            Cap bottleneck = kInf;
            for (const int e : path)
                bottleneck = std::min(bottleneck, edges_[e].cap);
            if (bottleneck == kInf)
                return kInf;
            for (const int e : path) {
                edges_[e].cap -= bottleneck;
                edges_[e ^ 1].cap += bottleneck;
            }
            flow += bottleneck;
        }
    }
    return flow;
}

std::vector<bool> FlowGraph::minCutSourceSide(int s) const {
    // Vertices still reachable from s through unsaturated residual edges form the source side
    // S of a minimum cut (max-flow min-cut theorem); every edge from S to the rest is saturated.
    std::vector<bool> inSource(adj_.size(), false);
    std::queue<int>   queue;
    inSource[s] = true;
    queue.push(s);
    while (!queue.empty()) {
        const int u = queue.front();
        queue.pop();
        for (const int e : adj_[u]) {
            const int v = edges_[e].to;
            if (!inSource[v] && edges_[e].cap > kEps) {
                inSource[v] = true;
                queue.push(v);
            }
        }
    }
    return inSource;
}

} // namespace seam
