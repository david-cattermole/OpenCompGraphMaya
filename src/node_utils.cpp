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

// Maya
#include <maya/MObject.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MString.h>
#include <maya/MPlug.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnDependencyNode.h>

// STL
#include <sstream>  // stringstream
#include <cctype>   // toupper

// OCG
#include "opencompgraph.h"

// OCG Maya
#include "logger.h"
#include "graph_data.h"

#include "node_utils.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace utils {

bool get_attr_value_bool(MDataBlock &data_block, MObject &attr) {
    MStatus status;
    MDataHandle handle = data_block.inputValue(attr, &status);
    CHECK_MSTATUS(status);
    return handle.asBool();
}

int16_t get_attr_value_short(MDataBlock &data_block, MObject &attr) {
    MStatus status;
    MDataHandle handle = data_block.inputValue(attr, &status);
    CHECK_MSTATUS(status);
    return handle.asShort();
}

int32_t get_attr_value_int(MDataBlock &data_block, MObject &attr) {
    MStatus status;
    MDataHandle handle = data_block.inputValue(attr, &status);
    CHECK_MSTATUS(status);
    return handle.asInt();
}

float get_attr_value_float(MDataBlock &data_block, MObject &attr) {
    MStatus status;
    MDataHandle handle = data_block.inputValue(attr, &status);
    CHECK_MSTATUS(status);
    return handle.asFloat();
}

MString get_attr_value_string(MDataBlock &data_block, MObject &attr) {
    MStatus status;
    MDataHandle handle = data_block.inputValue(attr, &status);
    CHECK_MSTATUS(status);
    return handle.asString();
}

// Get the ocgStreamData type from the given plug.
MStatus
get_plug_ocg_stream_value(MPlug &plug,
                          std::shared_ptr<ocg::Graph> &graph,
                          ocg::Node &value) {
    MStatus status;
    auto log = log::get_logger();
    log->debug(
        "Reading plug: {}",
        plug.name().asChar());

    if (plug.isNull()) {
        log->error(
            "Plug is not valid: {}",
            plug.name().asChar());
        status.perror("Plug is not valid.");
        status = MS::kFailure;
        return status;
    }

    MObject new_object = plug.asMObject(&status);
    if (new_object.isNull() || (status != MS::kSuccess)) {
        log->warn("Input stream is not valid - maybe connect a node?");
        value = ocg::Node(ocg::NodeType::kNull, 0);
        status = MS::kSuccess;
        return status;
    }

    // Convert Maya controlled data into the OCG custom MPxData class.
    // We are ensured this is valid from Maya. The MObject is a smart
    // pointer and we check the object is valid before-hand too.
    MFnPluginData fn_plugin_data(new_object);
    GraphData *input_stream_data =
        static_cast<GraphData *>(fn_plugin_data.data(&status));
    CHECK_MSTATUS(status);
    if (input_stream_data == nullptr) {
        log->error("Input stream data is not valid.");
        status = MS::kFailure;
        status.perror("Input stream data is not valid.");
        return status;
    }

    value = input_stream_data->get_node();
    log->debug("input node id: {}", value.get_id());

    return status;
}

MString generate_unique_hash_string() {
    // Generate hash and convert to string.
    auto unique_hash_number =
        ocg::internal::generate_random_id();
    std::stringstream string_stream;
    string_stream
        << std::setfill('0')
        << std::setw(sizeof(unique_hash_number) * 2)
        << std::hex
        << unique_hash_number;
    auto unique_hash_string = string_stream.str();
    std::transform(
        unique_hash_string.begin(),
        unique_hash_string.end(),
        unique_hash_string.begin(),
        [](unsigned char c){ return std::toupper(c); }
    );
    MString unique_hash_mstring(unique_hash_string.c_str());
    return unique_hash_mstring;
}

MStatus find_unique_node_hash_plug(MFnDependencyNode &fn_depend_node, MPlug &plug) {
    MString hash_attr_name("uniqueNodeHash");
    plug = fn_depend_node.findPlug(hash_attr_name, true);
    return MS::kSuccess;
}

MStatus create_empty_unique_node_hash_attr(MFnDependencyNode &fn_depend_node) {
    MStatus status = MS::kSuccess;

    MPlug hash_plug;
    status = find_unique_node_hash_plug(fn_depend_node, hash_plug);
    CHECK_MSTATUS(status);

    // Get a unique random hash that will be created and set for the
    // node just after construction, and set it in an attribute.
    if (hash_plug.isNull()) {
        // Create Attribute
        MFnStringData fn_string_data;
        MObject str_attr_object = fn_string_data.create("");
        MFnTypedAttribute attr;
        attr.setHidden(true);
        MString hash_attr_name("uniqueNodeHash");
        MObject attr_obj = attr.create(
            hash_attr_name, hash_attr_name,
            MFnData::kString, str_attr_object);
        fn_depend_node.addAttribute(
            attr_obj, MFnDependencyNode::kLocalDynamicAttr);
        hash_plug = fn_depend_node.findPlug(attr_obj, true);
    }

    return status;
}

MStatus set_new_unique_node_hash_attr(MFnDependencyNode &fn_depend_node) {
    MStatus status = MS::kSuccess;

    MPlug hash_plug;
    status = find_unique_node_hash_plug(fn_depend_node, hash_plug);
    CHECK_MSTATUS(status);

    if (hash_plug.isNull()) {return MS::kFailure;}

    hash_plug.setLocked(false);
    MString unique_hash_mstring = generate_unique_hash_string();
    hash_plug.setValue(unique_hash_mstring);
    hash_plug.setLocked(true);

    return status;
}
} // namespace utils
} // namespace open_comp_graph_maya
