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
 * Node utilities.
 */

#ifndef OPENCOMPGRAPHMAYA_NODE_UTILS_H
#define OPENCOMPGRAPHMAYA_NODE_UTILS_H

// Maya
#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MDataBlock.h>
#include <maya/MString.h>

// OCG
#include "opencompgraph.h"

namespace open_comp_graph_maya {
namespace utils {

bool get_attr_value_bool(MDataBlock &data_block, MObject &attr);

int16_t get_attr_value_short(MDataBlock &data_block, MObject &attr);

int32_t get_attr_value_int(MDataBlock &data_block, MObject &attr);

float get_attr_value_float(MDataBlock &data_block, MObject &attr);

MString get_attr_value_string(MDataBlock &data_block, MObject &attr);

MStatus
get_plug_ocg_stream_value(MPlug &plug,
                          std::shared_ptr<open_comp_graph::Graph> &graph,
                          open_comp_graph::Node &value);

MStatus
create_empty_unique_node_hash_attr(MFnDependencyNode &fn_depend_node);

MStatus
set_new_unique_node_hash_attr(MFnDependencyNode &fn_depend_node);

MStatus
join_ocg_nodes(
    std::shared_ptr<ocg::Graph> &shared_graph,
    ocg::Node &input_ocg_node,
    ocg::Node &output_ocg_node,
    uint8_t input_num);

uint64_t
generate_unique_node_hash(MUuid &node_uuid, MString &node_name);

} // namespace utils
} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_NODE_UTILS_H
