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
 * Read an image from a file path.
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
#include "image_read_node.h"
#include "node_utils.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

MTypeId ImageReadNode::m_id(OCGM_IMAGE_READ_TYPE_ID);

// Input Attributes
MObject ImageReadNode::m_enable_attr;
MObject ImageReadNode::m_frame_start_attr;
MObject ImageReadNode::m_frame_end_attr;
MObject ImageReadNode::m_frame_after_attr;
MObject ImageReadNode::m_frame_before_attr;
MObject ImageReadNode::m_file_path_attr;

// Output Attributes
MObject ImageReadNode::m_out_stream_attr;

ImageReadNode::ImageReadNode()
        : m_ocg_node(ocg::Node(ocg::NodeType::kNull, 0)) {}

ImageReadNode::~ImageReadNode() {}

MString ImageReadNode::nodeName() {
    return MString(OCGM_IMAGE_READ_TYPE_NAME);
}

MStatus ImageReadNode::updateOcgNodes(
        MDataBlock &data,
        std::shared_ptr<ocg::Graph> &shared_graph,
        std::vector<ocg::Node> input_ocg_nodes,
        ocg::Node &output_ocg_node) {
    MStatus status = MS::kSuccess;
    if (input_ocg_nodes.size() != 0) {
        return MS::kFailure;
    }
    bool exists = shared_graph->node_exists(m_ocg_node);
    if (!exists) {
        MString node_name = "read";
        auto node_hash = generate_unique_node_hash(
            m_node_uuid,
            node_name);
        m_ocg_node = shared_graph->create_node(
            ocg::NodeType::kReadImage,
            node_hash);
    }

    if (m_ocg_node.get_id() != 0) {
        // Set the output node
        output_ocg_node = m_ocg_node;

        // Enable Attribute toggle
        bool enable = utils::get_attr_value_bool(data, m_enable_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "enable", static_cast<int32_t>(enable));

        // Start / End Frame Attribute
        auto start_frame = utils::get_attr_value_int(data, m_frame_start_attr);
        auto end_frame = utils::get_attr_value_int(data, m_frame_end_attr);
        shared_graph->set_node_attr_i32(m_ocg_node, "start_frame", start_frame);
        shared_graph->set_node_attr_i32(m_ocg_node, "end_frame", end_frame);

        // Before Frame Attribute
        int16_t before_frame =
            utils::get_attr_value_short(data, m_frame_before_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "before_frame", static_cast<int32_t>(before_frame));

        // After Frame Attribute
        int16_t after_frame =
            utils::get_attr_value_short(data, m_frame_after_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "after_frame", static_cast<int32_t>(after_frame));

        // File Path Attribute
        MString file_path = utils::get_attr_value_string(data, m_file_path_attr);
        shared_graph->set_node_attr_str(
            m_ocg_node, "file_path", file_path.asChar());
    }

    return status;
}

MStatus ImageReadNode::compute(const MPlug &plug, MDataBlock &data) {
    MObjectArray in_attr_array;
    return computeOcgStream(
        plug, data,
        in_attr_array,
        m_out_stream_attr);
}

void *ImageReadNode::creator() {
    return (new ImageReadNode());
}

MStatus ImageReadNode::initialize() {
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

    // Start Frame
    uint32_t frame_default = 0;
    m_frame_start_attr = nAttr.create(
        "startFrame", "sfr",
        MFnNumericData::kInt, frame_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(false));

    // End Frame
    m_frame_end_attr = nAttr.create(
        "endFrame", "efr",
        MFnNumericData::kInt, frame_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(false));

    // Before Frame
    m_frame_before_attr = eAttr.create("beforeFrame", "beffrm", 0);
    CHECK_MSTATUS(eAttr.addField("hold", 0));
    CHECK_MSTATUS(eAttr.addField("loop", 1));
    CHECK_MSTATUS(eAttr.addField("bounce", 2));
    CHECK_MSTATUS(eAttr.addField("black", 3));
    CHECK_MSTATUS(eAttr.addField("error", 4));
    CHECK_MSTATUS(eAttr.setStorable(true));

    // After Frame
    m_frame_after_attr = eAttr.create("afterFrame", "aftfrm", 0);
    CHECK_MSTATUS(eAttr.addField("hold", 0));
    CHECK_MSTATUS(eAttr.addField("loop", 1));
    CHECK_MSTATUS(eAttr.addField("bounce", 2));
    CHECK_MSTATUS(eAttr.addField("black", 3));
    CHECK_MSTATUS(eAttr.addField("error", 4));
    CHECK_MSTATUS(eAttr.setStorable(true));

    // Create Common Attributes
    CHECK_MSTATUS(create_enable_attribute(m_enable_attr));
    CHECK_MSTATUS(create_output_stream_attribute(m_out_stream_attr));

    // Add Attributes
    CHECK_MSTATUS(addAttribute(m_enable_attr));
    CHECK_MSTATUS(addAttribute(m_file_path_attr));
    CHECK_MSTATUS(addAttribute(m_frame_start_attr));
    CHECK_MSTATUS(addAttribute(m_frame_end_attr));
    CHECK_MSTATUS(addAttribute(m_frame_after_attr));
    CHECK_MSTATUS(addAttribute(m_frame_before_attr));
    CHECK_MSTATUS(addAttribute(m_out_stream_attr));

    // Attribute Affects
    CHECK_MSTATUS(attributeAffects(m_enable_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_frame_start_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_frame_end_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_file_path_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
