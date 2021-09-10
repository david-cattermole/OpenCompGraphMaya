/*
 * Copyright (C) 2021 David Cattermole.
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
 * Executes the OCG graph.
 */

#ifndef OPENCOMPGRAPHMAYA_GRAPH_EXECUTE_H
#define OPENCOMPGRAPHMAYA_GRAPH_EXECUTE_H

// STL
#include <memory>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include "graph_data.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace graph {

ocg::ExecuteStatus execute_ocg_graph_frames(
    ocg::Node stream_ocg_node,
    std::vector<double> execute_frames,
    std::shared_ptr<ocg::Graph> shared_graph,
    std::shared_ptr<ocg::Cache> shared_cache);

ocg::ExecuteStatus execute_ocg_graph(
    ocg::Node stream_ocg_node,
    double execute_frame,
    std::shared_ptr<ocg::Graph> shared_graph,
    std::shared_ptr<ocg::Cache> shared_cache);

} // namespace graph
} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_GRAPH_EXECUTE_H
