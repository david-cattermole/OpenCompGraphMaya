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
 * Holds OCG Stream Data in the Maya DG.
 */

// Maya
#include <maya/MStreamUtils.h>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include <opencompgraphmaya/node_type_ids.h>
#include "graph_data.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

const MTypeId GraphData::m_id(OCGM_GRAPH_DATA_TYPE_ID);
const MString GraphData::m_type_name(OCGM_GRAPH_DATA_TYPE_NAME);


void* GraphData::creator() {
    return new GraphData;
}

GraphData::GraphData() : m_graph(),
                         m_ocg_node(ocg::NodeType::kNull, 0),
                         MPxData() {}

GraphData::~GraphData() {}

std::shared_ptr<ocg::Graph> GraphData::get_graph() const {
    return m_graph;
}

bool GraphData::is_valid_graph() const {
    return static_cast<bool>(m_graph);
}

void GraphData::set_graph(std::shared_ptr<ocg::Graph> value) {
    m_graph = value;
}

ocg::Node GraphData::get_node() const {
    return m_ocg_node;
}

void GraphData::set_node(ocg::Node value) {
    m_ocg_node = value;
}

void GraphData::copy(const MPxData& other) {
    m_graph = static_cast<const GraphData&>(other).m_graph;
    m_ocg_node = static_cast<const GraphData&>(other).m_ocg_node;
}

MTypeId GraphData::typeId() const {
    return GraphData::m_id;
}

// Function required by base-class.
MString GraphData::name() const {
    return GraphData::m_type_name;
}

// This is static, so to be called by plug-in initialize functions.
MString GraphData::typeName() {
    return MString(OCGM_GRAPH_DATA_TYPE_NAME);
}

MStatus GraphData::readASCII(const MArgList& args,
                             unsigned& lastParsedElement) {
    return MS::kSuccess;
}

MStatus GraphData::writeASCII(ostream& out) {
    return MS::kSuccess;
}

MStatus GraphData::readBinary(istream& in, unsigned) {
    return MS::kSuccess;
}

MStatus GraphData::writeBinary(ostream& out) {
    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
