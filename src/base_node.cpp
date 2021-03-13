/*
 * Copyright (C) 2020, 2021 David Cattermole.
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
 * Base class node for all OCG nodes.
 */

// Maya
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MUuid.h>

// STL
#include <cstring>
#include <cmath>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include <opencompgraphmaya/node_type_ids.h>
#include "logger.h"
#include "graph_data.h"
#include "base_node.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

BaseNode::BaseNode()
        : m_ocg_node_hash(0) {}

BaseNode::~BaseNode() {}

// Called after the node is created.
void BaseNode::postConstructor() {
    // Get the size
    MObject this_node = BaseNode::thisMObject();

    // Get Node UUID
    MStatus status = MS::kSuccess;
    MFnDependencyNode fn_depend_node(this_node, &status);
    CHECK_MSTATUS(status);
    MUuid uuid = fn_depend_node.uuid();
    MString uuid_string = uuid.asString();
    const char *uuid_char = uuid_string.asChar();

    // Generate a 64-bit hash id from the 128-bit UUID.
    BaseNode::m_ocg_node_hash =
        ocg::internal::generate_id_from_name(uuid_char);
};


MStatus BaseNode::create_enable_attribute(MObject &attr) {
    MStatus status = MS::kFailure;
    MFnNumericAttribute nAttr;

    attr = nAttr.create(
        "enable", "enb",
        MFnNumericData::kBoolean, true);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    return MS::kSuccess;
}

MStatus BaseNode::create_input_stream_attribute(MObject &attr) {
    MStatus status = MS::kFailure;
    MFnTypedAttribute tAttr;
    MTypeId stream_data_type_id(OCGM_GRAPH_DATA_TYPE_ID);

    attr = tAttr.create(
        "inStream", "istm",
        stream_data_type_id);
    CHECK_MSTATUS(tAttr.setStorable(false));
    CHECK_MSTATUS(tAttr.setKeyable(false));
    CHECK_MSTATUS(tAttr.setReadable(true));
    CHECK_MSTATUS(tAttr.setWritable(true));
    return MS::kSuccess;
}

MStatus BaseNode::create_output_stream_attribute(MObject &attr) {
    MStatus status = MS::kFailure;
    MFnTypedAttribute tAttr;
    MTypeId stream_data_type_id(OCGM_GRAPH_DATA_TYPE_ID);

    attr = tAttr.create(
        "outStream", "ostm",
        stream_data_type_id);
    CHECK_MSTATUS(tAttr.setStorable(false));
    CHECK_MSTATUS(tAttr.setKeyable(false));
    CHECK_MSTATUS(tAttr.setReadable(true));
    CHECK_MSTATUS(tAttr.setWritable(false));
    return MS::kSuccess;
}


} // namespace open_comp_graph_maya
