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
#include "graph_maya_data.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

const MTypeId GraphMayaData::m_id(OCGM_GRAPH_DATA_TYPE_ID);
const MString GraphMayaData::m_type_name(OCGM_GRAPH_DATA_TYPE_NAME);


void* GraphMayaData::creator() {
    return new GraphMayaData;
}

GraphMayaData::GraphMayaData() : m_graph(),
                                 m_ocg_node(ocg::NodeType::kNull, 0),
                                 MPxData() {}

GraphMayaData::~GraphMayaData() {}

std::shared_ptr<ocg::Graph> GraphMayaData::get_graph() const {
    return m_graph;
}

bool GraphMayaData::is_valid_graph() const {
    return static_cast<bool>(m_graph);
}

void GraphMayaData::set_graph(std::shared_ptr<ocg::Graph> value) {
    m_graph = value;
}

ocg::Node GraphMayaData::get_node() const {
    return m_ocg_node;
}

void GraphMayaData::set_node(ocg::Node value) {
    m_ocg_node = value;
}

void GraphMayaData::copy(const MPxData& other) {
    m_graph = static_cast<const GraphMayaData&>(other).m_graph;
    m_ocg_node = static_cast<const GraphMayaData&>(other).m_ocg_node;
}

MTypeId GraphMayaData::typeId() const {
    return GraphMayaData::m_id;
}

// Function required by base-class.
MString GraphMayaData::name() const {
    return GraphMayaData::m_type_name;
}

// This is static, so to be called by plug-in initialize functions.
MString GraphMayaData::typeName() {
    return MString(OCGM_GRAPH_DATA_TYPE_NAME);
}

MStatus GraphMayaData::readASCII(const MArgList& args,
                                 unsigned& lastParsedElement) {
    return MS::kSuccess;
}

MStatus GraphMayaData::writeASCII(ostream& out) {
    return MS::kSuccess;
}

MStatus GraphMayaData::readBinary(istream& in, unsigned) {
    return MS::kSuccess;
}

MStatus GraphMayaData::writeBinary(ostream& out) {
    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
