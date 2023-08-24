﻿#include "transformation/conv.h"

using namespace refactor::common;
using namespace refactor::graph;

namespace refactor::transformation {
    using GraphTopo_ = GraphTopo<Cell<NodeInfo>, Cell<EdgeInfo>>;
    using Searcher_ = GraphTopoSearcher<Cell<NodeInfo>, Cell<EdgeInfo>>;

    ConvTransformer::ConvTransformer(graph::GraphMut &&graph)
        : _graph(std::forward<GraphMut>(graph)),
          _convs{} {
        for (auto const &node : _graph.topo().nodes()) {
            auto &info = node.info().value;
            if (info.isSubgraph() || info.operator_().opType != OpType::Conv) {
                continue;
            }

            auto const &kernel = node.inputs()[1].info().value.tensor();
            auto const &dilations = info.operator_().attributes["dilations"].ints();
            if (kernel.shape.size() == 4 && kernel.shape[2] == 1 && kernel.shape[3] == 1) {
                _convs[node] = Rule::_1x1;
            } else if (std::any_of(dilations.begin(), dilations.end(), [](auto x) { return x > 1; })) {
                _convs[node] = Rule::Dilation;
            } else {
                continue;
            }

            GraphTopo_ g;
            std::vector<decltype(g)::EdgeRef> inputs;
            for (auto const &edge : node.inputs()) {
                inputs.push_back(g.addEdge(edge.info()));
            }
            g.markOutput(g.addNode(std::move(info), std::move(inputs), {{}})[0]);
            auto subgraph = std::make_shared<GraphMut>(std::move(g));
            subgraph->fillEdgeInfo();
            info = Subgraph{std::move(subgraph)};
        }
    }

    void ConvTransformer::reduce() {
        GraphTopo_ newTopo;
        std::map<Searcher_::Edge, GraphTopo_::EdgeRef> old2new;
        // 克隆全图输入
        for (auto const &input : _graph.topo().globalInputs()) {
            old2new.insert({input, newTopo.addEdge({std::move(input.info().value)})});
        }
        // 克隆节点
        for (auto const &node : _graph.topo().nodes()) {
            if (node.info().value.isOperator()) {// 克隆算子
                // 对齐算子输入
                auto const &inputs = node.inputs();
                std::vector<GraphTopo_::EdgeRef> newInputs(inputs.size());
                std::transform(inputs.begin(), inputs.end(), newInputs.begin(),
                               [&old2new](auto const &input) { return old2new[input]; });
                // 导出算子输出信息
                auto const &outputs = node.outputs();
                std::vector<Cell<EdgeInfo>> newOutputs;
                newOutputs.reserve(outputs.size());
                std::transform(outputs.begin(), outputs.end(), std::back_inserter(newOutputs),
                               [](auto const &output) { return std::move(output.info().value); });
                // 添加算子
                auto newNode = newTopo.addNode(std::move(node.info().value), std::move(newInputs), std::move(newOutputs));
                // 对齐算子输出
                for (auto i = 0; i < outputs.size(); ++i) { old2new.insert({outputs[i], newNode[i]}); }

            } else if (node.info().value.isSubgraph()) {// 展平子图
                auto subgraph = std::move(node.info().value.subgraph().graph);
                std::map<Searcher_::Edge, GraphTopo_::EdgeRef> inner2new;

                {// 对齐子图输入
                    auto const &inputs = node.inputs();
                    auto innerInputs = subgraph->topo().globalInputs();
                    for (size_t i = 0; i < inputs.size(); ++i) {
                        inner2new[innerInputs[i]] = old2new[inputs[i]];
                    }
                }
                for (auto const &node_ : subgraph->topo().nodes()) {
                    // 克隆子图算子
                    // 子图算子的输出不会被用在子图之外，所以不需要对齐到新图

                    // 对齐算子输入
                    auto const &inputs_ = node_.inputs();
                    std::vector<GraphTopo_::EdgeRef> newInputs(inputs_.size());
                    std::transform(inputs_.begin(), inputs_.end(), newInputs.begin(),
                                   [&inner2new](auto const &input) { return inner2new[input]; });
                    // 导出算子输出信息
                    auto const &outputs = node_.outputs();
                    std::vector<Cell<EdgeInfo>> newOutputs;
                    newOutputs.reserve(outputs.size());
                    std::transform(outputs.begin(), outputs.end(), std::back_inserter(newOutputs),
                                   [](auto const &output) { return std::move(output.info().value); });
                    // 添加算子
                    auto newNode = newTopo.addNode(std::move(node.info().value), std::move(newInputs), std::move(newOutputs));
                    // 对齐算子输出
                    for (auto i = 0; i < outputs.size(); ++i) { inner2new.insert({outputs[i], newNode[i]}); }
                }
                {// 对齐子图输出
                    auto outputs = node.outputs();
                    auto innerOutputs = subgraph->topo().globalOutputs();
                    for (size_t i = 0; i < outputs.size(); ++i) {
                        old2new[outputs[i]] = inner2new[innerOutputs[i]];
                    }
                }

            } else {
                RUNTIME_ERROR("Unreachable");
            }
        }
        {// 标记全图输出
            auto outputs = _graph.topo().globalOutputs();
            std::vector<GraphTopo_::EdgeRef> newOutputs(outputs.size());
            std::transform(outputs.begin(), outputs.end(), newOutputs.begin(),
                           [&old2new](auto const &output) { return old2new[output]; });
            newTopo.markOutput(newOutputs);
        }
    }

}// namespace refactor::transformation
