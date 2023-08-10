﻿#ifndef GRAPH_TOPO_WAPPER_HPP
#define GRAPH_TOPO_WAPPER_HPP

#include "graph_topo.hpp"

// 为了方便随便弄个类型占位
using NodeInfo = int;
using EdgeInfo = int;
using EdgeKey = int;

// template<class NodeInfo, class EdgeInfo, class EdgeKey>
class GraphTopoBuilder {
    GraphTopo<NodeInfo, EdgeInfo> graph;

public:
    void addEdge(EdgeKey);
    void addNode(
        NodeInfo info,
        std::vector<EdgeKey> const &inputs,
        std::vector<EdgeKey> const &outputs);
    GraphTopo<NodeInfo, EdgeInfo> build() {
        auto ans = std::move(graph);

        // TODO

        return ans;
    }
};

#endif// GRAPH_TOPO_WAPPER_HPP