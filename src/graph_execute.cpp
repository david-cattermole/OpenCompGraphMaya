/*
 * Copyright (C) 2020 David Cattermole.
 *
 * This file is part of OpenCompGraphMaya.
 *
 * OpenCompGraphMaya is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * OpenCompGraphMaya is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with OpenCompGraphMaya.  If not, see <https://www.gnu.org/licenses/>.
 * ====================================================================
 *
 * Executes the OCG Graph.
 */

// OCG
#include "opencompgraph.h"

// OCG Maya
#include "graph_data.h"
#include "logger.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace graph {


// Trigger an OCG Graph evaluation and return the computed data.
ocg::ExecuteStatus execute_ocg_graph_frames(
        ocg::Node stream_ocg_node,
        std::vector<double> execute_frames,
        std::shared_ptr <ocg::Graph> shared_graph,
        std::shared_ptr <ocg::Cache> shared_cache) {
    auto log = log::get_logger();

    bool exists = shared_graph->node_exists(stream_ocg_node);
    log->debug(
        "input node id={} node type={} exists={}",
        stream_ocg_node.get_id(),
        static_cast<uint64_t>(stream_ocg_node.get_node_type()),
        exists);

    for (auto f : execute_frames) {
        log->debug("execute_frames={}", f);
    }

    auto exec_status = shared_graph->execute(
        stream_ocg_node, execute_frames, shared_cache);
    log->debug(
        "execute status={}",
        static_cast<uint64_t>(exec_status));

    auto input_node_status = shared_graph->node_status(stream_ocg_node);
    log->debug(
        "input node status={}",
        static_cast<uint64_t>(input_node_status));
    log->debug(
        "Graph as string:\n{}",
        shared_graph->data_debug_string());
    log->debug(
        "Cache as string:\n{}",
        shared_cache->data_debug_string());

    if (exec_status != ocg::ExecuteStatus::kSuccess) {
        log->error("Failed to execute OCG node network!");
    }
    return exec_status;
}


// Trigger an OCG Graph evaluation and return the computed data.
ocg::ExecuteStatus execute_ocg_graph(
        ocg::Node stream_ocg_node,
        double execute_frame,
        std::shared_ptr <ocg::Graph> shared_graph,
        std::shared_ptr <ocg::Cache> shared_cache) {
    auto log = log::get_logger();

    std::vector <double> execute_frames;
    execute_frames.push_back(execute_frame);

    return execute_ocg_graph_frames(
        stream_ocg_node,
        execute_frames,
        shared_graph,
        shared_cache);
}


} // namespace graph
} // namespace open_comp_graph_maya
