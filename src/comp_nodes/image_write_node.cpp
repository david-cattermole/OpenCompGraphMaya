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
 * Write an image from a file path.
 */

// Maya
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnEnumAttribute.h>
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
#include "image_write_node.h"
#include "node_utils.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

MTypeId ImageWriteNode::m_id(OCGM_IMAGE_WRITE_TYPE_ID);

// Input Attributes
MObject ImageWriteNode::m_in_stream_attr;
MObject ImageWriteNode::m_enable_attr;
MObject ImageWriteNode::m_file_path_attr;

// Output Attributes
MObject ImageWriteNode::m_out_stream_attr;

ImageWriteNode::ImageWriteNode()
        : m_ocg_node(ocg::Node(ocg::NodeType::kNull, 0)) {}

ImageWriteNode::~ImageWriteNode() {}

MString ImageWriteNode::nodeName() {
    return MString(OCGM_IMAGE_WRITE_TYPE_NAME);
}

MStatus ImageWriteNode::updateOcgNodes(
        MDataBlock &data,
        std::shared_ptr<ocg::Graph> &shared_graph,
        std::vector<ocg::Node> input_ocg_nodes,
        ocg::Node &output_ocg_node) {
    MStatus status = MS::kSuccess;
    auto log = log::get_logger();
    if (input_ocg_nodes.size() != 1) {
        return MS::kFailure;
    }

    bool exists = shared_graph->node_exists(m_ocg_node);
    if (!exists) {
        MString node_name = "write";
        auto node_hash = generate_unique_node_hash(
            m_node_uuid,
            node_name);
        m_ocg_node = shared_graph->create_node(
            ocg::NodeType::kWriteImage,
            node_hash);
    }
    auto input_ocg_node = input_ocg_nodes[0];
    shared_graph->connect(input_ocg_node, m_ocg_node, 0);

    if (m_ocg_node.get_id() != 0) {
        // Set the output node
        output_ocg_node = m_ocg_node;

        // Enable Attribute toggle
        bool enable = utils::get_attr_value_bool(data, m_enable_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "enable", static_cast<int32_t>(enable));

        // File Path Attribute
        MString file_path = utils::get_attr_value_string(data, m_file_path_attr);
        shared_graph->set_node_attr_str(
            m_ocg_node, "file_path", file_path.asChar());

        // // Crop-on-Write Attribute toggle
        // bool enable = utils::get_attr_value_bool(data, m_enable_attr);
        // shared_graph->set_node_attr_i32(
        //     m_ocg_node, "enable", static_cast<int32_t>(enable));
    }

    return status;
}

MStatus ImageWriteNode::compute(const MPlug &plug, MDataBlock &data) {
    MObjectArray in_attr_array;
    in_attr_array.append(m_in_stream_attr);
    return computeOcgStream(
        plug, data,
        in_attr_array,
        m_out_stream_attr);
}

void *ImageWriteNode::creator() {
    return (new ImageWriteNode());
}

MStatus ImageWriteNode::initialize() {
    MStatus status;
    MFnUnitAttribute    uAttr;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute   tAttr;
    MFnEnumAttribute    eAttr;

    // Create empty string data to be used as attribute default
    // (string) value.
    MFnStringData empty_string_data;
    MObject empty_string_data_obj = empty_string_data.create("");

    // File Path Attribute
    m_file_path_attr = tAttr.create(
            "filePath", "fp",
            MFnData::kString, empty_string_data_obj);
    CHECK_MSTATUS(tAttr.setStorable(true));
    CHECK_MSTATUS(tAttr.setUsedAsFilename(true));

    // Create Common Attributes
    CHECK_MSTATUS(create_enable_attribute(m_enable_attr));
    CHECK_MSTATUS(create_input_stream_attribute(m_in_stream_attr));
    CHECK_MSTATUS(create_output_stream_attribute(m_out_stream_attr));

    // Add Attributes
    CHECK_MSTATUS(addAttribute(m_enable_attr));
    CHECK_MSTATUS(addAttribute(m_file_path_attr));
    CHECK_MSTATUS(addAttribute(m_in_stream_attr));
    CHECK_MSTATUS(addAttribute(m_out_stream_attr));

    // Attribute Affects
    CHECK_MSTATUS(attributeAffects(m_enable_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_in_stream_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_file_path_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya