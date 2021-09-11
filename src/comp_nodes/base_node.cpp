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
#include <sstream>  // stringstream
#include <cmath>
#include <cctype>   // toupper

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
        : m_node_uuid() {}

BaseNode::~BaseNode() {}

MStatus BaseNode::joinOcgNodes(
        std::shared_ptr<ocg::Graph> &shared_graph,
        ocg::Node &input_ocg_node,
        ocg::Node &output_ocg_node,
        uint8_t input_num) {
    bool input_node_exists = shared_graph->node_exists(input_ocg_node);
    if (input_node_exists) {
        shared_graph->connect(input_ocg_node, output_ocg_node, input_num);
    } else {
        shared_graph->disconnect_input(output_ocg_node, input_num);
    }
    return MS::kSuccess;
}

MStatus BaseNode::updateOcgNodes(
        MDataBlock &data,
        std::shared_ptr<ocg::Graph> &shared_graph,
        std::vector<ocg::Node> input_ocg_nodes,
        ocg::Node &output_ocg_node) {
    return MS::kFailure;
}

MStatus BaseNode::computeOcgStream(const MPlug &plug, MDataBlock &data,
                                   MObjectArray &in_stream_attr_array,
                                   MObject &out_stream_attr) {
    auto log = log::get_logger();
    MStatus status = MS::kUnknownParameter;

    const MUuid empty_uuid = MUuid();
    if (m_node_uuid == empty_uuid) {
        // No OCG hash has been created yet, this node is not ready
        // to be computed.
        return status;
    }

    if (plug == out_stream_attr) {
        // Create initial plug-in data structure. We don't need to
        // 'new' the data type directly.
        MFnPluginData fn_plugin_data;
        MTypeId data_type_id(OCGM_GRAPH_DATA_TYPE_ID);
        fn_plugin_data.create(data_type_id, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        // Get Input Stream
        auto input_ocg_nodes = std::vector<ocg::Node>();
        input_ocg_nodes.reserve(in_stream_attr_array.length());
        for (uint32_t i = 0; i < in_stream_attr_array.length(); ++i) {
            auto in_stream_attr = in_stream_attr_array[i];
            MDataHandle in_stream_handle = data.inputValue(
                in_stream_attr, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            ocg::Node input_ocg_node = ocg::Node(ocg::NodeType::kNull, 0);
            GraphData* input_stream_data =
                static_cast<GraphData*>(in_stream_handle.asPluginData());
            if (input_stream_data != nullptr) {
                input_ocg_node = input_stream_data->get_node();
            } else {
                log->warn(
                    "Input stream is not valid - maybe connect a node? input={}",
                    i);
            }
            input_ocg_nodes.push_back(input_ocg_node);
        }

        // Output Stream
        MDataHandle out_stream_handle = data.outputValue(out_stream_attr);
        GraphData* new_data =
            static_cast<GraphData*>(fn_plugin_data.data(&status));
        CHECK_MSTATUS_AND_RETURN_IT(status);

        // Update OCG nodes.
        auto shared_graph = get_shared_graph();
        auto output_ocg_node = ocg::Node(ocg::NodeType::kNull, 0);
        if (shared_graph) {
            // Initialise the OCG Graph, and update OCG node values.
            // The output node is modified via an output variable. The
            // output node is the 'last' node in the graph, and is
            // connected to downstream nodes.
            status = updateOcgNodes(
                data,
                shared_graph,
                input_ocg_nodes,
                output_ocg_node);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
        log->debug(
            "BaseNode: Graph as string:\n{}",
            shared_graph->data_debug_string());
        new_data->set_node(output_ocg_node);
        out_stream_handle.setMPxData(new_data);
        out_stream_handle.setClean();
        status = MS::kSuccess;
    }

    return status;
}


// Called after the node is created.
void BaseNode::postConstructor() {
    MObject this_node = BaseNode::thisMObject();
    MStatus status = MS::kSuccess;

    MFnDependencyNode fn_depend_node(this_node, &status);
    CHECK_MSTATUS(status);

    // Get Node UUID
    m_node_uuid = fn_depend_node.uuid();

    // Get a unique random hash that will be created and set for the
    // node just after construction, and set it in an attribute.
    MString hash_attr_name("uniqueNodeHash");
    MPlug hash_plug = fn_depend_node.findPlug(hash_attr_name, true);
    if (hash_plug.isNull())
    {
        // Create Attribute
        MFnStringData fn_string_data;
        MObject str_attr_object = fn_string_data.create("");
        MFnTypedAttribute attr;
        attr.setHidden(true);
        MObject attr_obj = attr.create(
            hash_attr_name, hash_attr_name,
            MFnData::kString, str_attr_object);
        fn_depend_node.addAttribute(
            attr_obj, MFnDependencyNode::kLocalDynamicAttr);
        hash_plug = fn_depend_node.findPlug(attr_obj, true);

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

        hash_plug.setValue(unique_hash_mstring);
        hash_plug.setLocked(true);
    }

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
    MString suffix = "";
    return BaseNode::create_input_stream_attribute(attr, suffix);
}


MStatus BaseNode::create_input_stream_attribute(MObject &attr, const MString &suffix) {
    MStatus status = MS::kFailure;
    MFnTypedAttribute tAttr;
    MTypeId stream_data_type_id(OCGM_GRAPH_DATA_TYPE_ID);

    MString long_name = "inStream";
    MString short_name = "istm";
    long_name += suffix;
    short_name += suffix;
    attr = tAttr.create(
        long_name, short_name,
        stream_data_type_id);
    CHECK_MSTATUS(tAttr.setStorable(false));
    CHECK_MSTATUS(tAttr.setKeyable(false));
    CHECK_MSTATUS(tAttr.setReadable(true));
    CHECK_MSTATUS(tAttr.setWritable(true));
    CHECK_MSTATUS(tAttr.setDisconnectBehavior(MFnAttribute::kReset));
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
    CHECK_MSTATUS(tAttr.setDisconnectBehavior(MFnAttribute::kReset));
    return MS::kSuccess;
}

// Generate a hash that is seeded by the current node UUID,
// providing a technique for creating consistent OCG node hashes
// for each instance of Maya node.
uint64_t BaseNode::generate_unique_node_hash(MUuid &node_uuid, MString &node_name) {
    MString uuid_string = node_uuid.asString();
    uuid_string += node_name;

    // Generate a 64-bit hash id from the 128-bit UUID string plus
    // the node name suffix.
    const char *uuid_char = uuid_string.asChar();
    auto node_hash =
        ocg::internal::generate_id_from_name(uuid_char);
    return node_hash;
}

} // namespace open_comp_graph_maya
